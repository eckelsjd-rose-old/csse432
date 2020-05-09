/* SSH server
Date: 5/9/20
Author: Joshua Eckels
Class: CSSE432 - networks
Notes:
9. IP independence
10. Use forking to listen on multiple ports
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

// used for cleaning up zombies
void sigchld_handler(int s) {
    int saved_errno = errno;
    while(waitpid(-1,NULL,WNOHANG) > 0);
    errno = saved_errno;
}

int main(int argc, char** argv) {
    // handle user input
    if (argc != 2) {
        printf("Incorrect usage: ./server <port number>\n");
        exit(1);
    }
    char* port = argv[1];
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

        // do some kind of user authentication for ssh server

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
            int filesize;
            char **args;
            char buf[MAXDATASIZE+1]; // server queuing buffer
            char *stmp = "stmp.txt"; // server temp file
            char *file_buf;          // server temp file buffer
            while (1) {
                // get a command line from client
                if ( (bytes_recv = qrecv(client_fd, buf, MAXDATASIZE,0)) == 0) { break; }
                printf("Command received from client: \"%s\"\n",buf);
                args = parse_args(buf);

                // handle custom commands

                // user wants file from server
                if (strcmp(args[0],"scp") == 0) {
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
                            printf("Sent bytes: %d\n",filesize);

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

                // take file from client
                else if (strcmp(args[0],"ssend") == 0) {
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

                // change directory
                else if (strcmp(args[0],"cd")==0) {
                    if (chdir(args[1])==-1) {
                        int len = strlen(strerror(errno)) + 2;
                        char msg[len];
                        sprintf(msg,"%s\n",strerror(errno));
                        msg[len-1] = '\0';
                        bytes_sent = qsend(client_fd,msg,len,0);
                        //continue
                    } else {
                        bytes_sent = qsend(client_fd,"\0",1,0);
                    }
                }

                // execute generic linux command
                else {
                    int link[2]; // pipe to get output from exec
                    int exec_pid;  // fork to execute command
                    if (pipe(link) == -1) {
                        perror("Pipe failed");
                        exit(EXIT_FAILURE);
                    }
                    if ((exec_pid = fork()) == -1) {
                        perror("Fork failed");
                        exit(EXIT_FAILURE);
                    }
                    if (exec_pid == 0) {
                        // link pipe to stdout
                        dup2(link[1],STDOUT_FILENO);
                        close(link[0]);
                        close(link[1]);
                        execvp(args[0],args);
                        // send error message to stdout to send to client
                        printf("%s\n",strerror(errno));
                        exit(EXIT_SUCCESS);
                    } else {
                        // read everything that was sent to stdout
                        close(link[1]);
                        char cbuf;
                        char *line = malloc(sizeof(char));
                        line[0] = '\0';
                        int i = 0;
                        while (read(link[0],&cbuf,sizeof(char)) > 0) {
                            line = realloc(line, (i+2)*sizeof(char));
                            line[i] = (char) cbuf;
                            line[i+1] = '\0';
                            i++;
                        }
                        bytes_sent = qsend(client_fd,line,strlen(line)+1,0);
                        free(line);
                        close(link[0]);
                    }
                }
                free_args(args); // cleanup
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
