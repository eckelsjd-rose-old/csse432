/* SSH server
Date: 5/9/20
Author: Joshua Eckels
Class: CSSE432 - networks
Notes:
9. IP independence
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
#define MAXFILESIZE 52428800 // 50 MiB
#define KNOWN_HOSTS "../users.txt"
#define LOGIN_SUCCESS 1 // successful login
#define LOGIN_FUSR 2    // failed user
#define LOGIN_FPW 3     // failed password
#define SUCCESS "Success"
#define FAILURE "Failure"

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

// validate user login against users.txt
int validLogin(char *user, char *password) {
    FILE *fd = fopen(KNOWN_HOSTS,"r");
    if (fd == NULL) {
        perror("fopen");
        exit(1);
    }
    char buf[MAXDATASIZE];
    memset(buf,0,MAXDATASIZE);
    while ( fgets(buf,MAXDATASIZE,fd) != NULL ) {
        buf[strlen(buf)-1] = '\0'; // trim newline
        char *curr_user = strtok(buf," ");
        if (strcmp(user,curr_user) == 0) {
            char *curr_pw = strtok(NULL," ");
            if (strcmp(password,curr_pw) == 0) {
                fclose(fd);
                return LOGIN_SUCCESS;
            } else {
                fclose(fd);
                return LOGIN_FPW;
            }
        }
        memset(buf,0,MAXDATASIZE);
    }
    return LOGIN_FUSR;
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

            int bytes_recv;
            int bytes_sent;
            int filesize;
            int i;
            char **args;
            char buf[MAXDATASIZE+1]; // server queuing buffer
            char *stmp = "stmp.txt"; // server temp file
            char *file_buf;          // server temp file buffer
            struct timeval timeout;

            // user authentication
            if ( (bytes_recv = qrecv(client_fd, buf, MAXDATASIZE,0)) == 0) { break;}
            args = parse_args(buf);
            int login = validLogin(args[0],args[1]);
            sprintf(buf,"%d",login);
            bytes_sent = qsend(client_fd,buf,strlen(buf)+1,0);
            int flag = 0;
            switch (login) {
                case LOGIN_SUCCESS :
                    break;
                case LOGIN_FUSR :
                    flag = 1;
                    break;
                case LOGIN_FPW :
                    flag = 1;
                    break;
                default :
                    printf("Invalid\n");
                    exit(1);
            }
            free_args(args);

            // main server<->client loop
            while (!flag) {
                int pwd_pid;
                int pwd_pipe[2];
                char pwd_cbuf;
                char *pwd_out;
                // send current working directory to user
                bytes_recv = qrecv(client_fd,buf,MAXDATASIZE,0);
                if (pipe(pwd_pipe) == -1) {
                    perror("Pipe failed");
                    exit(EXIT_FAILURE);
                }
                if ((pwd_pid = fork()) < 0) {
                    perror("server fork");
                    exit(EXIT_FAILURE);
                } else if (pwd_pid == 0) {
                    // link to stdout
                    dup2(pwd_pipe[1],STDOUT_FILENO);
                    dup2(pwd_pipe[1],STDERR_FILENO);
                    close(pwd_pipe[0]);
                    execlp(buf,buf,NULL);
                    printf("\"%s\" failed: %s\n",buf,strerror(errno));
                    exit(EXIT_SUCCESS);
                } else {
                    waitpid(pwd_pid,NULL,0);
                    pwd_out = malloc(sizeof(char));
                    pwd_out[0] ='\0';
                    i = 0;

                    close(pwd_pipe[1]);
                    while (read(pwd_pipe[0],&pwd_cbuf,sizeof(char)) > 0) {
                        pwd_out = realloc(pwd_out, (i+2)*sizeof(char));
                        pwd_out[i] = (char) pwd_cbuf;
                        pwd_out[i+1] = '\0';
                        i++;
                    }
                    bytes_sent = qsend(client_fd,pwd_out,strlen(pwd_out)+1,0);
                    free(pwd_out);
                    close(pwd_pipe[0]);
                }

                // get a command line from client
                // command limited to MAXDATASIZE bytes
                if ( (bytes_recv = qrecv(client_fd, buf, MAXDATASIZE,0)) == 0) { break; }
                if (strcmp(buf,FAILURE) == 0) {
                    continue; // if client enters a blank line or fails on custom command
                }
                printf("Command received from client: \"%s\"\n",buf);
                args = parse_args(buf);

                // handle custom commands

                // user wants file from server
                if (strcmp(args[0],"scp") == 0) {
                    // check if server has the file
                    if (isValidPath(args[1])) {
                        // send success to client
                        bytes_sent = qsend(client_fd,SUCCESS,strlen(SUCCESS)+1,0);

                        // wait for client to request transfer to begin
                        if ( (bytes_recv = qrecv(client_fd, buf, MAXDATASIZE,0)) == 0) { break; }

                        // start file transfer
                        if (strcmp(buf,SUCCESS) == 0) {
                            filesize = readAll(args[1], &file_buf);
                            memset(buf,0,MAXDATASIZE);
                            sprintf(buf,"%d",filesize); // send filesize
                            bytes_sent = qsend(client_fd,buf,strlen(buf)+1,0); 
                            bytes_recv = qrecv(client_fd,buf,MAXDATASIZE,0);

                            if (strcmp(buf,FAILURE)==0) {
                                free_args(args); // file was too big
                                free(file_buf);
                                continue;
                            }
                            bytes_sent = qsend(client_fd,file_buf,filesize,0);
                            printf("Sent bytes: %d\n",filesize);
                            bytes_recv = qrecv(client_fd,buf,MAXDATASIZE,0);
                            if (strcmp(buf,FAILURE)==0) {
                                free(file_buf); // client timed-out; close connection
                                free_args(args);
                                break;
                            } else {
                                free(file_buf);
                                // continue
                            }

                        } else {
                            // something went wrong
                            printf("Server failed in transfer. Closing client...\n");
                            break;
                        }

                    } else {
                        // file not found on server
                        bytes_sent = qsend(client_fd,FAILURE,strlen(FAILURE)+1,0);
                        // continue
                    }
                }

                // take file from client
                else if (strcmp(args[0],"ssend") == 0) {
                    // check for valid directory
                    if (isDirectory(args[2])) {
                        bytes_sent = qsend(client_fd,SUCCESS,strlen(SUCCESS)+1,0);

                        bytes_recv = qrecv(client_fd,buf,MAXDATASIZE,0); // get filesize
                        filesize = atoi(buf);
                        if (filesize > MAXFILESIZE) {
                            bytes_sent = qsend(client_fd,FAILURE,strlen(FAILURE)+1,0);
                            free_args(args);
                            continue; // file size too big to send
                        }
                        char *filename = getFilename(args[1]);
                        char *fullpath = getPath(args[2],filename);
                        bytes_sent = qsend(client_fd,SUCCESS,strlen(SUCCESS)+1,0);

                        // save file to disk
                        timeout.tv_sec = 1;
                        timeout.tv_usec = 0;
                        int total_bytes = qrecv_big(client_fd, fullpath, buf, MAXDATASIZE,&timeout);
                        // timeout
                        if (total_bytes != filesize) {
                            bytes_sent = qsend(client_fd,FAILURE,strlen(FAILURE)+1,0);
                            remove(fullpath);
                            free(filename);
                            free(fullpath);
                            free_args(args); 
                            break; // close connection on time-out
                        }
                        bytes_sent = qsend(client_fd,SUCCESS,strlen(SUCCESS)+1,0);
                        printf("Write success. Total bytes: %d\n",total_bytes);
                        free(filename);
                        free(fullpath);
                        //continue

                    } else {
                        // invalid directory
                        bytes_sent = qsend(client_fd,FAILURE,strlen(FAILURE)+1,0);
                        //continue
                    }
                }

                // change directory
                else if (strcmp(args[0],"cd")==0) {
                    if (chdir(args[1])==-1) {
                        int len = strlen(strerror(errno)) + strlen(args[0]) + 11;
                        char msg[len];
                        sprintf(msg,"%s failed: %s\n",args[0],strerror(errno));
                        msg[len-1] = '\0';
                        
                        // handshaking
                        bytes_sent = qsend(client_fd,SUCCESS,strlen(SUCCESS)+1,0);
                        bytes_recv = qrecv(client_fd,buf,MAXDATASIZE,0);

                        bytes_sent = qsend(client_fd,msg,len,0);
                        //continue
                    } else {
                        // handshaking 
                        bytes_sent = qsend(client_fd,SUCCESS,strlen(SUCCESS)+1,0);
                        bytes_recv = qrecv(client_fd,buf,MAXDATASIZE,0);

                        bytes_sent = qsend(client_fd,"\0",1,0);
                    }
                }

                // execute generic linux command
                // can only handle text-based commands
                // graphical programs or other types of data streams not supported
                else {
                    int link[2]; // pipe to get stdout output from exec
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
                        dup2(link[1],STDERR_FILENO);
                        close(link[0]);
                        execvp(args[0],args);
                        // send error message to stdout to send to client
                        printf("\"%s\" failed: %s\n",args[0],strerror(errno));
                        exit(EXIT_SUCCESS);
                    } else {
                        //int status;
                        //waitpid(exec_pid,&status,0);

                        char cbuf;
                        char *line = malloc(sizeof(char));
                        line[0] = '\0';
                        i = 0;

                        close(link[1]);
                        while (read(link[0],&cbuf,sizeof(char)) > 0) {
                            line = realloc(line, (i+2)*sizeof(char));
                            line[i] = (char) cbuf;
                            line[i+1] = '\0';
                            i++;
                        }

                        // handshaking
                        bytes_sent = qsend(client_fd,SUCCESS,strlen(SUCCESS)+1,0);
                        bytes_recv = qrecv(client_fd,buf,MAXDATASIZE,0);

                        // assume no file overflow or timeout issues here; just small commands
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
