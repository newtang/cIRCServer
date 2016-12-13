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
    char pass[10];
    char name[10];
    char userstr[50];
} user_t;

user_t users[10];

//char* parseCommand(char *str, char *args){
char *parseCommand(char *str){
    char *command;

    int i =0;
    int len = strlen(str);

    while(i<len){
        if(str[i] == ' '){
            break;
        }        
        ++i;
    }

    command=(char*)malloc(sizeof(char) * (i+1));
    
    strncpy(command, str, i+1);

    //strncpy(args, str+i, len);

    return (char *)command;

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

                        char commandArgs[256];
                        //char* comm = parseCommand(&buf, &commandArgs);
                        char *comm;
                        comm = parseCommand(*buf);


                        printf("command: %s", comm);


                	 	//we received a message!
                	 	printf("buf: %s", buf);
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

bool validUser(user_t *user){
    return strlen(user->pass) && strlen (user->name) && strlen (user->userstr);
}

