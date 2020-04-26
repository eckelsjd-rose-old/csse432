/* Basic echo server in C
Date: 4/11/20
Author: Joshua Eckels
Class: CSSE432 - networks
Notes:
1. Implement a chat service using pthreads
2. Rewrite in Java,C++,Python
3. Handle incorrect user input to command line
4. Handle incorrect client input
5. Handle buffer sizes better
6. Understand exit codes and sighandlers
7. Understand forking with sockets
8. Handle incorrect bytes sent/received
9. IP independence
10. Connect to Pi
*/

#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <netinet/in.h>
#include <sys/wait.h>
#include <arpa/inet.h>
#include <ctype.h>

#define PORT "3490"
#define BACKLOG 10
#define MAXDATASIZE 1024

// get sockaddr, IPv4 or IPv6
void *get_in_addr(struct sockaddr *sa) {
    // IPv4
    if (sa->sa_family == AF_INET) {
        return &(((struct sockaddr_in*)sa)->sin_addr);
    }
    // IPv6
    return &(((struct sockaddr_in6*)sa)->sin6_addr);
}

// used for cleaning up zombies; I assume this gets called
// when a process exits with status code 0
void sigchld_handler(int s) {
    int saved_errno = errno;
    while(waitpid(-1,NULL,WNOHANG) > 0);
    errno = saved_errno;
}

int setup_server(char* port) {
    // setup the server
    int status;
    int sockfd;
    struct addrinfo hints;              // fill out a few fields by hand
    struct addrinfo *servinfo;          // result of getaddrinfo()
    struct addrinfo *p;                 // temporary pointer to a struct addrinfo

    // input some known information to hints
    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_UNSPEC;        // AF_INET, AF_INET6, AF_UNSPEC
    hints.ai_socktype = SOCK_STREAM;    // TCP stream sockets
    hints.ai_flags = AI_PASSIVE;        // fill in the host IP automatically

    // getaddrinfo does DNS lookup and fills out the struct addrinfo *servinfo
    // getaddrinfo("IP","port",hints,result);
    // replace NULL with an IP address if you want something other than the local host
    if ((status = getaddrinfo(NULL, port, &hints, &servinfo)) != 0) {
        fprintf(stderr, "getaddrinfo error: %s\n", gai_strerror(status));
        exit(1);
    }

    // servinfo points to a linked list of 1 or more struct addrinfos
    // loop through this list and find the first acceptable address struct
    int yes = 1;
    for(p = servinfo; p != NULL; p = p->ai_next) {
        // socket
        if ((sockfd = socket(p->ai_family, p->ai_socktype,
                        p->ai_protocol)) == -1) {
            perror("server: socket");
            continue;
        }

        // lose the "Address already in use" error message
        if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &yes,
                    sizeof yes) == -1) {
            perror("server: setsockopt");
            exit(1);
        }

        // bind
        if (bind(sockfd,p->ai_addr, p->ai_addrlen) == -1) {
            close(sockfd);
            perror("server: bind");
            continue;
        }

        break;
    }

    if (p == NULL) {
        fprintf(stderr, "server: failed to bind\n");
        exit(1);
    } 

    freeaddrinfo(servinfo); // free linked-list

    // listen
    if (listen(sockfd, BACKLOG) == -1) {
        perror("server: listen");
        exit(1);
    }

    return sockfd;
}

/* Main function starts a server
   Continuously accepts new clients
*/
int main(int argc, char** argv) {
    // handle user input
    if (argc != 2) {
        printf("Incorrect usage: ./server <port number>\n");
        exit(1);
    }
    char* port = (strcmp(argv[1],"0")) ? argv[1]:PORT; // default

    // setup server
    int sockfd = setup_server(port);

    // clean up zombie processes on signal
    struct sigaction sa;
    sa.sa_handler = sigchld_handler;
    sa.sa_flags = SA_RESTART;
    if (sigaction(SIGCHLD, &sa, NULL) == -1) {
        perror("sigaction");
        exit(1);
    }

    printf("server: waiting for connections on port %s...\n",port);

    // buffer for client address
    char s[INET6_ADDRSTRLEN];

    // main accept() loop
    while(1) {
        // accept a client
        struct sockaddr_storage client_addr;
        socklen_t addr_size;
        addr_size = sizeof client_addr;
        int client_fd = accept(sockfd, (struct sockaddr *)&client_addr, &addr_size);
        if (client_fd == -1) {
            perror("server: accept");
            continue;
        }

        // retrieve and print client IP address (network to presentation)
        inet_ntop(client_addr.ss_family, get_in_addr((struct sockaddr *)&client_addr),
                s, sizeof s);
        printf("server: got connection from %s\n",s);

        // spawn child to handle this client
        int pid = fork();
        if (pid < 0) {
            perror("Fork failed.\n");
            exit(1);
        } else if (pid == 0) { // child will handle the client
            close(sockfd); // child doesn't need server listener anymore

            while (1) {
                // recv from client
                char buf[MAXDATASIZE+2];
                int bytes_recv = recv(client_fd, buf, MAXDATASIZE+1,0);
                if (bytes_recv == -1) {
                    perror("server: recv");
                    exit(1);
                } else if (bytes_recv == 0) {
                    break; // break when client disconnects
                }

                buf[bytes_recv] = '\0';
                
                // print received message
                printf("server received: %s\n",buf);

                // capitalize the message
                char buf2[MAXDATASIZE];
                int i = 0;
                while (buf[i]) {
                    buf2[i] = toupper(buf[i]);
                    i++;
                }
                buf2[i] = '\0';

                // send the new message back to client
                int bytes_sent = send(client_fd,buf2,strlen(buf2)+1,0);
                if (bytes_sent == -1) {
                    perror("server: send");
                    exit(1);
                } else if (bytes_sent != strlen(buf2)+1) {
                    // handle incorrect number of bytes sent
                    printf("Msg length: %ld. Bytes sent: %d.\n",strlen(buf2)+1,bytes_sent);
                    exit(1);
                }
            }

            // close client socket when they disconnect
            close(client_fd);
            
            // exiting will kill the child process
            exit(0);

        } else { // parent will start accepting more clients
            close(client_fd); //parent doesn't need client socket
        }
    }

    return 0;
}
