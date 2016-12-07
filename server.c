#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <netinet/in.h>

// get sockaddr, IPv4 or IPv6:
void *get_in_addr(struct sockaddr *sa)
{
    if (sa->sa_family == AF_INET) {
        return &(((struct sockaddr_in*)sa)->sin_addr);
    }

    return &(((struct sockaddr_in6*)sa)->sin6_addr);
}

int main (){

	int status;
	struct addrinfo hints;
	struct addrinfo *res;  // will point to the results
	char s[INET6_ADDRSTRLEN];

	memset(&hints, 0, sizeof hints); // make sure the struct is empty
	hints.ai_family = AF_UNSPEC;     // don't care IPv4 or IPv6
	hints.ai_socktype = SOCK_STREAM; // TCP stream sockets
	hints.ai_flags = AI_PASSIVE;     // fill in my IP for me

	if ( (status = getaddrinfo(NULL, "5555", &hints, &res)) != 0) {
	    fprintf(stderr, "getaddrinfo error: %s\n", gai_strerror(status));
	    return 1;
	}

	int sockfd = socket(res->ai_family, res->ai_socktype, res->ai_protocol);


	// bind it to the port we passed in to getaddrinfo():
	bind(sockfd, res->ai_addr, res->ai_addrlen);

	freeaddrinfo(res);

	int queueSize = 20;

	if( (status = listen(sockfd, queueSize)) != 0){
		fprintf(stderr, "listen error: %s\n", gai_strerror(status));
	    return 1;
	}

	// now accept an incoming connection:
	struct sockaddr_storage their_addr;
    socklen_t addr_size = sizeof their_addr;
    while(1){

	    int new_fd = accept(sockfd, (struct sockaddr *)&their_addr, &addr_size);
	    if (new_fd == -1) {
            perror("accept");
            continue;
        }

        inet_ntop(their_addr.ss_family,
        	get_in_addr((struct sockaddr *)&their_addr), s, sizeof s);
        printf("server: got connection from %s\n", s);


        /**
          * This forks the program. It's essentially duplicated. Weirdly, this method essentially returns twice, one 
          to its self, and one to it's child.  The child gets a return of '0' (so it enters this if statement)
          and the original gets the pid of the child, so it loops again, and blocks on accept!

        **/
	    if (!fork()) { // this is the child process

            close(sockfd); // child doesn't need the listener

            char *msg = "Hello There!";
			int len = strlen(msg);

            if (send(new_fd, msg, len, 0) == -1)
                perror("send");
            
            int bufLen = 200;
            char buf[bufLen];
            while(recv(new_fd, buf, bufLen, 0)){
            	printf("%s", buf);
            	memset(buf, 0, bufLen);
            }

            close(new_fd);
            return 0;
        }

        close(new_fd);  // parent doesn't need this
	}

	return 0;	
}