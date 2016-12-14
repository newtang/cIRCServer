/**
  * http://beej.us/guide/bgnet/output/html/singlepage/bgnet.html#getaddrinfo
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
} user_t;

user_t users[256];

char* parseCommand(char *str, char *args){
    int i =0;
    
    while(str[i] != '\0'){
        if(str[i] == ' '){
            str[i] = '\0';
            return &str[i+1];
        }        
        ++i;
    }
}

bool validUser(user_t *user){
    return user->pass != '\0' && user->name != '\0' &&  user->userstr != '\0';
}

void write_to_user(int user_fd, char* str){
    send(user_fd, str, strlen(str), 0);
}

void handleCommand(char *comm, char *args, int user_fd){

    if(!users[user_fd].valid){

        if(strcmp(comm, "PASS") == 0){
            users[user_fd].pass = (char *) malloc(strlen(args));
            strcpy(users[user_fd].pass, args);

        }
        else if(strcmp(comm, "NICK") == 0){
            users[user_fd].name = args;
        }
        else if(strcmp(comm, "USER") == 0){
            users[user_fd].userstr = args;
        }

        if(validUser(&users[user_fd])){
            users[user_fd].valid = true;
            write_to_user(user_fd, "You're valid!");
            return;
        }
    }
    else{

    }



}

int main (){

	int status;
	struct addrinfo hints;
	struct addrinfo *res;  // will point to the results
	char remoteIP[INET6_ADDRSTRLEN];

	struct sockaddr_storage remoteaddr; // client address

	fd_set master;    // master file descriptor list
    fd_set read_fds;  // temp file descriptor list for select()
    int fdmax;        // maximum file descriptor number

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

	int listener = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
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
                	 }
                	 else {

                        printf("buf: %s", buf);
                        
                        //parseCommand puts a null terminator between the command and arguments
                        char *commandArgs = parseCommand(buf, commandArgs);

                        //we received a message!
                        printf("command: %s\n", buf);
                        printf("command Args: %s\n", commandArgs); 


                        handleCommand(buf, commandArgs, i);


                	 	for(int j=0; j<=fdmax; ++j){
                	 		if (FD_ISSET(j, &master)){
	                	 		if(j != listener && j != i){
	                	 			send(j, buf, nbytes, 0);
	                	 		}
	                	 	}
                	 	}
                	}
                }
            }
        }
    }

	return 0;	
}





