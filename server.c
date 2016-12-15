/**
  * http://beej.us/guide/bgnet/output/html/singlepage/bgnet.html#getaddrinfo
  */

/**
    Improvements:
    - Get closer to protocol for receiving messages
    - the file descriptor should probably be a property of user.
    - Handle invalid commands args better
    - Something like the factory & policy pattern for handling commands
    - Handle room creation
    - Handle being in multiple rooms
    - string utils
    - clean up users when they leave.
    - make file
    - get closer to handling real client
*/

#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <stdlib.h>
#include "stringutils.h"


//client join a server
//client join a room

// get sockaddr, IPv4 or IPv6:
void *get_in_addr(struct sockaddr *sa)
{
    if (sa->sa_family == AF_INET) {
        return &(((struct sockaddr_in*)sa)->sin_addr);
    }

    return &(((struct sockaddr_in6*)sa)->sin6_addr);
}

typedef struct {
    char *pass;
    char *name;
    char *userstr;
    bool valid;
    char *room;
} user_t;

user_t users[256];

int listener;
fd_set master;    // master file descriptor list
fd_set read_fds;  // temp file descriptor list for select()
int fdmax;        // maximum file descriptor number



bool validUser(user_t *user){
    return user->pass != '\0' && user->name != '\0' &&  user->userstr != '\0';
}

void write_to_user(int user_fd, char* str){
    send(user_fd, str, strlen(str), 0);
}

char* buildMessage(char *name, char *message){
    char *dest = (char *) malloc(strlen(name) + strlen(message) + 2 );
    snprintf(dest, 100, "%s%s%s\n", name, "> ", message);
    return dest;
}

void write_to_room(int writer_fd, char* room, char* msg){

    printf("write to room |%s|%s|\n", room, users[writer_fd].room);

    if(strcmp(users[writer_fd].room, room) == 0){
        for(int i=0; i<=fdmax; ++i){
            if (FD_ISSET(i, &master)){
                if(i != listener && strcmp(users[i].room, room) == 0){
                    write_to_user(i, msg);
                } 
            }
        }
    }
    else{
        write_to_user(writer_fd, "You can't write to a room you're not in.\n");
    }
}

void handleCommand(char *comm, char *args, int user_fd){

    if(!users[user_fd].valid){

        if(strcmp(comm, "PASS") == 0){
            users[user_fd].pass = (char *) malloc(strlen(args));
            strcpy(users[user_fd].pass, args);
        }
        else if(strcmp(comm, "NICK") == 0){
            users[user_fd].name = (char *) malloc(strlen(args));
            strcpy(users[user_fd].name, args);
        }
        else if(strcmp(comm, "USER") == 0){
            users[user_fd].userstr = (char *) malloc(strlen(args));
            strcpy(users[user_fd].userstr, args);
        }
        else{
            write_to_user(user_fd, "Not authenticated yet. Invalid command.");
        }

        if(validUser(&users[user_fd])){
            users[user_fd].valid = true;
            write_to_user(user_fd, "You're authenticated!\n");
            return;
        }
    }
    else{
        if(strcmp(comm, "JOIN") == 0){
            users[user_fd].room = (char *) malloc(strlen(args));
            strcpy(users[user_fd].room, args);

            char *prefix = "You have joined: ";
            char *dest = (char *) malloc(strlen(args) + strlen(prefix) +1);

            snprintf(dest, 100, "%s%s\n", prefix, args);
            write_to_user(user_fd, dest);
            //write_to_user(user_fd, strcat("You have joined a room ", args));
        }
        else if(strcmp(comm, "PRIVMSG") == 0){
            char* msg_args = parseCommand(args);
            //target (room) is the args
            //the message is the msg_args

            printf("target %s\n", args);
            printf("msg_args %s\n", msg_args);

            //assuming target is a room.
            char* msg = buildMessage(users[user_fd].name, msg_args);
            printf("msg %s\n", msg);
            write_to_room(user_fd, args, msg);
            free(msg);

        }
    }
}



int main (){

	int status;
	struct addrinfo hints;
	struct addrinfo *res;  // will point to the results
	char remoteIP[INET6_ADDRSTRLEN];

	struct sockaddr_storage remoteaddr; // client address

    FD_ZERO(&master);    // clear the master and temp sets
    FD_ZERO(&read_fds);

	memset(&hints, 0, sizeof hints); // make sure the struct is empty
	hints.ai_family = AF_UNSPEC;     // don't care IPv4 or IPv6
	hints.ai_socktype = SOCK_STREAM; // TCP stream sockets
	hints.ai_flags = AI_PASSIVE;     // fill in my IP for me

	if ( (status = getaddrinfo(NULL, "5555", &hints, &res)) != 0) {
	    fprintf(stderr, "getaddrinfo error: %s\n", gai_strerror(status));
	    return 1;
	}

	listener = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
    printf("listener %d\n", listener);

    int yes=1;
    // lose the pesky "Address already in use" error message
    if (setsockopt(listener,SOL_SOCKET,SO_REUSEADDR,&yes,sizeof yes) == -1) {
        perror("setsockopt");
        exit(1);
    } 


	// bind it to the port we passed in to getaddrinfo():
	bind(listener, res->ai_addr, res->ai_addrlen);

	freeaddrinfo(res);

	int queueSize = 20;

	if( (status = listen(listener, queueSize)) != 0){
		fprintf(stderr, "listen error: %s\n", gai_strerror(status));
	    return 1;
	}

	fdmax = listener;
	// add the listener to the master set
    FD_SET(listener, &master);


	// now accept an incoming connection:
	struct sockaddr_storage their_addr;
    socklen_t addr_size = sizeof their_addr;
    while(1){
    	read_fds = master; // copy it
    	printf("Awaiting connection or input\n");
    	if (select(fdmax+1, &read_fds, NULL, NULL, NULL) == -1) {
            perror("select");
            exit(4);
        }

        for(int i = 0; i <= fdmax; i++) {

            if (FD_ISSET(i, &read_fds)) { // we got one!!
                if (i == listener) {
                	int new_fd = accept(listener, (struct sockaddr *)&their_addr, &addr_size);
                	if (new_fd != -1){
                		FD_SET(new_fd, &master);
                		if (new_fd > fdmax) {    // keep track of the max
                            fdmax = new_fd;
                        }
                        printf("selectserver: new connection from %s on "
                            "socket %d\n",
                            inet_ntop(their_addr.ss_family,
                                get_in_addr((struct sockaddr*)&their_addr),
                                remoteIP, INET6_ADDRSTRLEN),
                            new_fd);

                        user_t user;
                        user.valid = false;
                        user.pass = 0;
                        user.name = 0;
                        user.userstr = 0;
                        users[new_fd] = user;

                	}
                	else{
                		perror("accept new connection");
                	}
                }
                else {
                	 int nbytes;
                	 char buf[256];

                	 if ((nbytes = recv(i, buf, sizeof buf, 0)) <= 0) {
                	 	FD_CLR(i, &master);
                        //clear from user table
                	 }
                	 else {

                        printf("buf: %s", buf);
                        
                        //parseCommand puts a null terminator between the command and arguments
                        char *commandArgs = parseCommand(buf);

                        //we received a message!
                        printf("command: %s\n", buf);
                        printf("command Args: %s\n", commandArgs); 
                        handleCommand(buf, commandArgs, i);
                        memset(&buf[0], 0, sizeof(buf));
                	}
                }
            }
        }
    }

	return 0;	
}





