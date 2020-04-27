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
            while (1) {
                // get a command from client
                if ( (bytes_recv = qrecv(client_fd, buf, MAXDATASIZE,0)) == 0) { break; }
                printf("Command received from client: \"%s\"\n",buf);

                // handle iWant command
                if (strcmp(buf,"iWant") == 0) {
                    // confirm connection with client
                    bytes_sent = qsend(client_fd,buf,strlen(buf)+1,0);

                    // get the filename from client
                    if ( (bytes_recv = qrecv(client_fd, buf, MAXDATASIZE,0)) == 0) { break; }
                    printf("Client requesting file: \"%s\"\n",buf);

                    // check if server has the file
                    char *filepath = getPath(DEFAULT_DIR,buf);
                    if (isValidPath(filepath)) {
                        printf("Filepath: %s\n",filepath);
                        // send success to client
                        bytes_sent = qsend(client_fd,"Success",strlen("Success")+1,0);

                        // wait for client to request transfer to begin
                        if ( (bytes_recv = qrecv(client_fd, buf, MAXDATASIZE,0)) == 0) { break; }

                        // start file transfer
                        if (strcmp(buf,"Start") == 0) {
                            char *file_buf;
                            int filesize = readAll(filepath, &file_buf);

                            bytes_sent = qsend(client_fd,file_buf,filesize,0);
                            printf("File transfer to client completed.\n");

                            free(file_buf);
                            free(filepath);
                            continue;

                        } else {
                            // something went wrong
                            printf("Server failed in transfer. Closing client...\n");
                            break;
                        }

                    } else {
                        // file not found on server
                        bytes_sent = qsend(client_fd,"Failure",strlen("Failure")+1,0);
                        free(filepath);
                        printf("File: \"%s\" not found.\n",buf);
                        continue;
                    }
                }

                // handle uTake command
                else if (strcmp(buf,"uTake") == 0) {
                    // confirm connection with client
                    bytes_sent = qsend(client_fd,buf,strlen(buf)+1,0);

                    // receive directions from client
                    if ( (bytes_recv = qrecv(client_fd, buf, MAXDATASIZE,0)) == 0) { break; }

                    // client starts file transfer
                    if (strcmp(buf,"Start") == 0) {
                        // handshaking
                        bytes_sent = qsend(client_fd,"Handshaking",strlen("Handshaking")+1,0);

                        // receive filename from client
                        char filename[MAXDATASIZE+1];
                        bytes_recv = qrecv(client_fd, filename, MAXDATASIZE,0);
                        printf("Server received file \"%s\" from client.\n",filename);

                        // handshaking
                        bytes_sent = qsend(client_fd,"Handshaking",strlen("Handshaking")+1,0);

                        // convert filename to path
                        char *path = getPath(DEFAULT_DIR,filename);
                        printf("Path: %s\n",path);

                        // write to disk
                        printf("Writing to disk...\n");
                        int total_bytes = qrecv_big(client_fd, path, buf, MAXDATASIZE);
                        free(path);
                        printf("Write success. Total bytes: %d\n",total_bytes);
                        continue;
                    }

                    else if (strcmp(buf,"No file") == 0) {
                        // client didn't have the file
                        printf("No file received from client.\n");
                        continue;
                    }

                    else {
                        // Something went wrong
                        fprintf(stderr,"Error in file transfer. Closing client...\n");
                        break;
                    }

                }

                // invalid command
                else {
                    // command checking should be handled in client
                    fprintf(stderr,"Invalid command. Closing client...\n");
                    break;
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
