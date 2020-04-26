#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <netdb.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <arpa/inet.h>

#define PORT "3490"
#define MAXDATASIZE 1024
#define DEFAULT_DIR "./received_files"

// reads entire file into a buffer
// returns number of bytes in file
int readAll(char *path, char **buf) {
    FILE *fd;
    int numbytes;

    fd = fopen(path, "r");
    if (fd == NULL) {
        perror("Failed to open file.");
        exit(1);
    }

    // get number of bytes
    fseek(fd, 0L, SEEK_END);
    numbytes = ftell(fd);

    // go back to beginning of file
    fseek(fd, 0L, SEEK_SET);

    *buf = malloc(sizeof(char) * numbytes);
    memset(*buf,0,sizeof(char) * numbytes);

    // copy all text into buffer
    fread(*buf, sizeof(char),numbytes,fd);
    fclose(fd);
    return numbytes;
}

// determines if a valid file path
// saves just filename if possible
int isValidPath(char *path, char **filename) {
    FILE *fd = fopen(path, "r");
    if (fd == NULL) {
        return 0;
    }
    fclose(fd);

    // extract filename from path
    char *path_cpy = malloc(strlen(path)+1);
    memset(path_cpy,0,strlen(path)+1);
    strcpy(path_cpy,path);

    char delim[] = "/";
    char *token_ptr = strtok(path_cpy, delim);
    char *save_ptr = token_ptr;

    while (token_ptr != NULL) {
        save_ptr = token_ptr;
        token_ptr = strtok(NULL, delim);
    }

    *filename = malloc(strlen(save_ptr)+1);
    memset(*filename,0,strlen(save_ptr)+1);
    strcpy(*filename,save_ptr);

    free(path_cpy);
    return 1;
}

// counts number of args
int num_args(char** args) {
    int num = 0;
    for (int i = 0; strcmp(args[i],"\0") != 0;i++) {
        num++;
    }
    return num;
}

// frees command line args
void free_args(char** args) {
    for (int i = 0; strcmp(args[i],"\0") != 0; i++) {
       free(args[i]);
    }
   free(args);
}

// this function returns an allocated array 
// of args passed in as a command line string
char** parse_args(char* cmdline, char** args) {
    char delim[] = " ";
    args = malloc(sizeof(char*));

    // check allocation
    if (args == NULL) {
        perror("Unsuccessful allocation\n");
        exit(1);
    }

    args[0] = "\0"; // signal the end of the array
    char *ptr = strtok(cmdline, delim); // break up cmdline into tokens
    int i = 0;

    // break up cmdline into tokens using dynamic memory reallocation
    while (ptr != NULL) {
        args = realloc(args, (i+2)*sizeof(char*));

        // check allocation
        if (args == NULL) {
            perror("Unsuccessful allocation\n");
            exit(1);
        }

        // allocate space for new token
        char* token = malloc(strlen(ptr)+1);
        if (token == NULL) {
            perror("Unsuccessful allocation\n");
            exit(1);
        }
        token = strcpy(token,ptr);

        // add token to args list
        args[i] = token;
        args[i+1] = "\0";

        // iterate
        i++;
        ptr = strtok(NULL, delim);
    }

    return args;
}

// this function dynamically allocates space from user input
// returns a pointer to the line read from stdin
char* getaline() {
    int ch; // getchar() returns an int
    
    char* line = malloc(sizeof(char));
    
    // check allocation
    if (line == NULL) {
        perror("Unsuccessful allocation");
        exit(1);
    }   

    line[0] = '\0';

    // keep reading until we get a newline from user
    for(int i = 0; ((ch = getchar()) != '\n') && (ch != EOF); i++) {
        // reallocate the line
        line = realloc(line, (i+2)*sizeof(char));

        // check allocation
        if (line == NULL) {
            perror("unsuccessful reallocation");
            exit(1);
        }

        line[i] = (char) ch; 
        line[i+1] = '\0';
    }   

    return line;
}

void *get_in_addr(struct sockaddr *sa) {
    if (sa->sa_family == AF_INET) {
        return &(((struct sockaddr_in*)sa)->sin_addr);
    }
    return &(((struct sockaddr_in6*)sa)->sin6_addr);
}

int setup_client(char* hostname,char* port) {
    int status;
    int sockfd;
    struct addrinfo hints;
    struct addrinfo *servinfo;
    struct addrinfo *p;

    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM; // TCP stream

    // getaddrinfo()
    if ((status = getaddrinfo(hostname,port,&hints,&servinfo)) != 0) {
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(status));
        exit(1);
    }

    // loop through all results and connect to first we can
    for (p = servinfo; p != NULL; p = p->ai_next) {
        // socket
        if ((sockfd = socket(p->ai_family, p->ai_socktype,
                        p->ai_protocol)) == -1) {
            perror("client: socket");
            continue;
        }

        // connect
        if (connect(sockfd, p->ai_addr, p->ai_addrlen) == -1) {
            close(sockfd);
            perror("client: connect");
            continue;
        }

        break;
    }

    if (p == NULL) {
        fprintf(stderr, "client: failed to connect\n");
        exit(1);
    }

    // string buffer to get IP address of server
    char s[INET6_ADDRSTRLEN];
    inet_ntop(p->ai_family, get_in_addr((struct sockaddr *)p->ai_addr),
            s, sizeof s);
    printf("client: connecting to %s on port %s\n", s, port);

    // free addrinfo structure
    freeaddrinfo(servinfo);
    return sockfd;
}

/* Main client function.
   Sets up a client socket and connects to server
   passed in command line as argument.
*/
int main(int argc, char** argv) {
    // get hostname from command line
    if (argc != 3) {
        fprintf(stderr,"usage: ./client <hostname> <port number>\n");
        exit(1);
    }
    char* hostname = argv[1];
    char* port = (strcmp(argv[2],"0")) ? argv[2]:PORT;

    // setup client socket
    int sockfd = setup_client(hostname,port);

    // start CLI for user client program
    char* line;
    char** args;
    while (1) {
        // get a line from user
        printf("client>");
        line = getaline();

        // parse client command line arguments
        args = parse_args(line,args);

        if (strcmp(args[0], ".") == 0) { break; }

        if (num_args(args) != 2) {
            printf("Usage: <cmd> <filename>\n");
            free_args(args);
            free(line);
            continue;
        }

        // handle iWant command
        if (strcmp(args[0], "iWant") == 0) {
            // send the name of the command to the server first
            printf("Requesting command \"%s\" from server...\n",args[0]);
            int bytes_sent = send(sockfd,args[0],strlen(args[0])+1,0);
            if (bytes_sent == -1) {
                perror("client: send");
                exit(1);
            } else if (bytes_sent != strlen(args[0])+1) {
                // handle incorrect number of bytes sent
                printf("Msg length: %ld. Bytes sent: %d.\n",strlen(args[0])+1,bytes_sent);
                exit(1);
            }

            // confirm connection from server
            char buf[MAXDATASIZE+2];
            int bytes_recv = recv(sockfd, buf, MAXDATASIZE+1,0);
            if (bytes_recv == -1) {
                perror("client: recv");
                exit(1);
            }
            buf[bytes_recv] = '\0';
            printf("Server confirmed connection: %s\n",buf);

            // send the filename to the server (with null-terminator)
            // client should format as "file.txt"
            // server will handle searching in its directories for this file
            bytes_sent = send(sockfd,args[1],strlen(args[1])+1,0);
            if (bytes_sent == -1) {
                perror("client: send");
                exit(1);
            } else if (bytes_sent != strlen(args[1])+1) {
                // handle incorrect number of bytes sent
                printf("Msg length: %ld. Bytes sent: %d.\n",strlen(args[1])+1,bytes_sent);
                exit(1);
            }

            // wait for a success or failure message from server
            memset(buf,0,MAXDATASIZE+2);
            bytes_recv = recv(sockfd, buf, MAXDATASIZE+1,0);
            if (bytes_recv == -1) {
                perror("client: recv");
                exit(1);
            }
            buf[bytes_recv] = '\0';
            printf("Locating file on server: %s\n",buf);

            // if file was found on server
            if (strcmp(buf,"Success") == 0) {
                // ask for directory from user (With error checking and defaults)
                // constructs the fullpath string from user input
                printf("Save file to path: ");
                char *fullpath;
                char *filename; // won't do anything with this
                char *path = getaline();
                if (isValidPath(path,&filename)) {
                    int size = strlen(path) + strlen(args[1]) + 2;
                    fullpath = malloc(size);
                    memset(fullpath,0,size);
                    strcpy(fullpath,path);
                    fullpath[strlen(path)] = '/';
                    char *ptr = fullpath + strlen(path) + 1;
                    strcpy(ptr, args[1]);
                    free(filename);
                } else {
                    printf("Invalid path. Using default...\n");
                    int size = strlen(DEFAULT_DIR) + strlen(args[1]) + 2;
                    fullpath = malloc(size);
                    memset(fullpath,0,size);
                    strcpy(fullpath,DEFAULT_DIR);
                    fullpath[strlen(DEFAULT_DIR)] = '/';
                    char *ptr = fullpath + strlen(DEFAULT_DIR) + 1;
                    strcpy(ptr, args[1]);
                }
                free(path);

                // send message to server requesting file transfer to begin
                bytes_sent = send(sockfd,"Start",strlen("Start"),0);
                if (bytes_sent == -1) {
                    perror("client: send");
                    exit(1);
                } else if (bytes_sent != strlen("Start")) {
                    // handle incorrect number of bytes sent
                    printf("Msg length: %ld. Bytes sent: %d.\n",strlen("Start"),bytes_sent);
                    exit(1);
                }

                // receive file from server
                // assume file size is below 1024 for now
                memset(buf,0,MAXDATASIZE+2);
                bytes_recv = recv(sockfd, buf, MAXDATASIZE+1,0);
                if (bytes_recv == -1) {
                    perror("client: recv");
                    exit(1);
                }
                buf[bytes_recv] = '\0';
                printf("Received file from server.\n");

                // save file to disk
                printf("Writing to disk...\n");
                FILE *fd = fopen(fullpath, "w");
                if (fd == NULL) {
                    printf("Failed to open file\n");
                    exit(1);
                }
                if (fputs(buf, fd) < 0) {
                    perror("Failed during file write");
                    exit(1);
                }
                fclose(fd);
                free(fullpath);
                printf("Write success.\n");
                // continue
            }

            // Else could not find file on server
            else {
                printf("File \"%s\" not found on server.\n",args[1]);
                free_args(args);
                free(line);
                continue;
            }

        // Handle uTake command
        } else if (strcmp(args[0], "uTake") == 0) {
            // send the name of the command to the server first
            printf("Requesting command \"%s\" from server...\n",args[0]);
            int bytes_sent = send(sockfd,args[0],strlen(args[0])+1,0);
            if (bytes_sent == -1) {
                perror("client: send");
                exit(1);
            } else if (bytes_sent != strlen(args[0])+1) {
                // handle incorrect number of bytes sent
                printf("Msg length: %ld. Bytes sent: %d.\n",strlen(args[0])+1,bytes_sent);
                exit(1);
            }

            // confirm connection from server
            char buf[MAXDATASIZE+2];
            int bytes_recv = recv(sockfd, buf, MAXDATASIZE+1,0);
            if (bytes_recv == -1) {
                perror("client: recv");
                exit(1);
            }
            buf[bytes_recv] = '\0';
            printf("Server confirmed connection: %s\n",buf);

            // check if arg[1] is a file on client system
            char *filename;
            if (isValidPath(args[1],&filename)) {
                // tell server message is on its way
                bytes_sent = send(sockfd,"Start",strlen("Start"),0);
                if (bytes_sent == -1) {
                    perror("client: send");
                    exit(1);
                } else if (bytes_sent != strlen("Start")) {
                    // handle incorrect number of bytes sent
                    printf("Msg length: %ld. Bytes sent: %d.\n",strlen("Start"),bytes_sent);
                    exit(1);
                }
            } else {
                // tell server no message is coming
                bytes_sent = send(sockfd,"No file",strlen("No file"),0);
                if (bytes_sent == -1) {
                    perror("client: send");
                    exit(1);
                } else if (bytes_sent != strlen("No file")) {
                    // handle incorrect number of bytes sent
                    printf("Msg length: %ld. Bytes sent: %d.\n",strlen("No file"),bytes_sent);
                    exit(1);
                }

                printf("File \"%s\" not found.\n",args[1]);
                free_args(args);
                free(line);
                continue;
            }

            // handshaking
            memset(buf,0,MAXDATASIZE+2);
            bytes_recv = recv(sockfd, buf, MAXDATASIZE+1,0);
            if (bytes_recv == -1) {
                perror("client: recv");
                exit(1);
            }
            buf[bytes_recv] = '\0';

            // send file name to server
            bytes_sent = send(sockfd,filename,strlen(filename)+1,0);
            if (bytes_sent == -1) {
                perror("client: send");
                exit(1);
            } else if (bytes_sent != strlen(filename)+1) {
                // handle incorrect number of bytes sent
                printf("Msg length: %ld. Bytes sent: %d.\n",strlen(filename)+1,bytes_sent);
                exit(1);
            }

            // handshaking
            memset(buf,0,MAXDATASIZE+2);
            bytes_recv = recv(sockfd, buf, MAXDATASIZE+1,0);
            if (bytes_recv == -1) {
                perror("client: recv");
                exit(1);
            }
            buf[bytes_recv] = '\0';

            // send file to server
            char *file_buf;
            int filesize = readAll(args[1],&file_buf);
            
            bytes_sent = send(sockfd,file_buf,filesize,0);
            if (bytes_sent == -1) {
                perror("client: send");
                exit(1);
            } else if (bytes_sent != filesize) {
                // handle incorrect number of bytes sent
                printf("Msg length: %d. Bytes sent: %d.\n",filesize,bytes_sent);
                exit(1);
            }
            
            printf("File \"%s\" sent to server.\n",filename);
            free(filename);
            free(file_buf);
            // continue

        } 
        
        // handle incorrect CLI input from user
        else {
            printf("Command not recognized. Try again...\n");
            free_args(args);
            free(line);
            continue;
        }

        free_args(args);
        free(line);
    }

    // Clean up
    free_args(args);
    free(line);
    printf("Closing client socket...\n");
    close(sockfd);
}
