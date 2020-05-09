/* File transfer server
Date: 4/27/20
Author: Joshua Eckels
Class: CSSE432 - networks
Notes:
2. Rewrite in Java,C++,Python
6. Understand exit codes and sighandlers
7. Understand forking with sockets
9. IP independence
10. Use forking to listen on multiple ports
11. Use exec to allow ssh client->server
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
#include <sys/select.h>
#include <sys/time.h>

#include <eckelsjd.h>

#define PORT "3490"
#define MAXDATASIZE 1024
#define DEFAULT_DIR "../server_files"

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
            perror("Fork failed");
            exit(1);
        } else if (pid == 0) { // child will handle the client
            close(sockfd); // child doesn't need server listener anymore

            // main server<->client loop
            int bytes_recv;
            int bytes_sent;
            char buf[MAXDATASIZE+1];

            // receive message from client
            // might need to fix to receive more bytes
            if ( (bytes_recv = qrecv(client_fd,buf,MAXDATASIZE,0)) == 0) {break; }
            
            // check for correct HTTP message
            printf("%s\n",buf);

            // if not, form and send error message, then exit

            // if correctly formatted, parse message for
            // requested host, port, and URL
            char *hostname = "www.google.com";
            char *remote_port = "80";

            // form an HTTP message to send to remote host
            char *msg = "GET / HTTP/1.0\r\nHost: www.google.com\r\nConnection: close\r\n\r\n";

            // create client connection with host and receive one HTTP response
            int remote_fd = setup_client(hostname,remote_port);

            bytes_sent = qsend(remote_fd,msg,strlen(msg),0);

            char *path = "response.txt";
            bytes_recv = qrecv_big(remote_fd,path,buf,MAXDATASIZE);
            close(remote_fd);
            printf("bytes_recv:%d\n",bytes_recv);

            // send this HTTP response to client as is
            char *response;
            int filesize = readAll(path,&response);
            printf("filesize:%d\n",filesize);
            bytes_sent = qsend(client_fd,response,filesize,0);
            printf("bytes_sent:%d\n",bytes_sent);
            free(response);

            // close client
            close(client_fd);
            
            // exiting will kill the child process
            exit(0);

        } else { // parent will start accepting more clients
            close(client_fd); //parent doesn't need client socket
        }
    }

    return 0;
}
