/* C library for useful C functions
Author: Joshua Eckels
Include with #include <eckelsjd.h> */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <unistd.h>
#include <ctype.h>
#include <netdb.h>
#include <sys/time.h>
#include <sys/select.h>
#include <dirent.h>

char* getaline() {
    int ch; // getchar() returns an int
    
    char* line = malloc(sizeof(char));
    
    // check allocation
    if (line == NULL) {
        perror("getaline");
        exit(1);
    }   

    line[0] = '\0';

    // keep reading until we get a newline from user
    for(int i = 0; ((ch = getchar()) != '\n') && (ch != EOF); i++) {
        // reallocate the line
        line = realloc(line, (i+2)*sizeof(char));

        // check allocation
        if (line == NULL) {
            perror("getaline");
            exit(1);
        }

        line[i] = (char) ch; 
        line[i+1] = '\0';
    }   

    return line;
}

char** parse_args(char* cmdline) {
    char delim[] = " ";
    char **args = malloc(sizeof(char*));
    char *str_cpy = malloc(strlen(cmdline)+1);
    memset(str_cpy,0,strlen(cmdline)+1);
    strcpy(str_cpy,cmdline);

    // check allocation
    if (args == NULL) {
        perror("parse_args");
        exit(1);
    }

    args[0] = NULL; // signal the end of the array
    char *ptr = strtok(str_cpy, delim); // break up cmdline into tokens
    int i = 0;

    // break up cmdline into tokens using dynamic memory reallocation
    while (ptr != NULL) {
        args = realloc(args, (i+2)*sizeof(char*));

        // check allocation
        if (args == NULL) {
            perror("parse_args");
            exit(1);
        }

        // allocate space for new token
        char* token = malloc(strlen(ptr)+1);
        if (token == NULL) {
            perror("parse_args");
            exit(1);
        }
        token = strcpy(token,ptr);

        // add token to args list
        args[i] = token;
        args[i+1] = NULL;

        // iterate
        i++;
        ptr = strtok(NULL, delim);
    }
    free(str_cpy);
    return args;
}

int num_args(char** args) {
    int num = 0;
    for (int i = 0; args[i] != NULL; i++) {
        num++;
    }
    return num;
}

void free_args(char** args) {
    for (int i = 0; args[i] != NULL; i++) {
       free(args[i]);
    }
   free(args);
}

int readAll(char *path, char **buf) {
    FILE *fd;
    int numbytes;
    fd = fopen(path, "r");
    if (fd == NULL) {
        perror("readAll");
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
    //fclose(fd);
    return numbytes;
}

int isValidPath(char *path) {
    FILE *fd = fopen(path, "r");
    if (fd == NULL) {
        return 0;
    }
    fclose(fd);
    return 1;
}

int isDirectory(char *path) {
    DIR *directory = opendir(path);
    if (directory != NULL) {
        closedir(directory);
        return 1;
    }
    return 0;
}

char *getFilename(char *path) {
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

    char *filename = malloc(strlen(save_ptr)+1);
    memset(filename,0,strlen(save_ptr)+1);
    strcpy(filename,save_ptr);

    free(path_cpy);
    return filename;
}

char* getPath(char *dir, char *filename) {
    int size = strlen(dir) + strlen(filename) + 2;
    char *path = malloc( sizeof(char) * size);

    // check allocation
    if (path == NULL) {
        perror("getPath");
        exit(1);
    }   

    memset(path,0,size);
    strcpy(path,dir);
    char *ptr = path + strlen(path);
    ptr[0] = '/';
    ptr = ptr + 1;
    strcpy(ptr,filename);

    return path;
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
    inet_ntop(p->ai_family, &(((struct sockaddr_in *)p->ai_addr)->sin_addr),
            s, sizeof s);
    printf("client: connecting to %s on port %s\n", s, port);

    // free addrinfo structure
    freeaddrinfo(servinfo);
    return sockfd;
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
    int backlog = 10;
    if (listen(sockfd, backlog) == -1) {
        perror("server: listen");
        exit(1);
    }

    return sockfd;
}

int qrecv(int sockfd, char *buf, int size, int flags) {
    memset(buf,0,size+1);
    int bytes_recv = recv(sockfd,buf,size,flags);
    if (bytes_recv == -1) {
        perror("recv");
        exit(1);
    } else if (bytes_recv == 0) {
        return 0; // signal when client disconnects
    }
    buf[bytes_recv] = '\0';
    return bytes_recv;
}

int qsend(int sockfd, char *buf, int size, int flags) {
    int bytes_sent = send(sockfd,buf,size,flags);
    if (bytes_sent == -1) {
        perror("send");
        exit(1);
    } else if (bytes_sent != size) {
        // handle incorrect number of bytes sent
        fprintf(stderr,"Wrong number of bytes sent");
        exit(1);
    }
    return bytes_sent;
}

int qrecv_big(int sockfd, char *path, char *buf, int datasize,struct timeval *timeout) {
    FILE *fd = fopen(path, "a");
    if (fd == NULL) {
        perror("qrecv_big");
        exit(1);
    }

    // block until data is visible on socket

    // receive arbitrarily large file data
    // client must send full file at once (one send() call)
    int total_bytes = 0;
//    struct timeval start;
//   gettimeofday(&start,NULL);
//    printf("qrecv start: %lu.%lu\n",start.tv_sec,start.tv_usec);
    while (1) {
        fd_set rfds;
        struct timeval tv;
        int retval;
        tv.tv_sec = timeout->tv_sec;
        tv.tv_usec = timeout->tv_usec;
        FD_ZERO(&rfds);
        FD_SET(sockfd,&rfds);
        retval = select(sockfd+1,&rfds,NULL,NULL,&tv);
        if (retval == -1) {
            perror("select()");
            exit(1);
        } else if (retval) {
            // continue through loop
        } else {
            // done reading
//            struct timeval end;
//            gettimeofday(&end,NULL);
//            printf("qrecv end: %lu.%lu\n",end.tv_sec,end.tv_usec);
            break;
        }

        // will only receive if recv will be non-blocking
        int bytes_recv;
        if ( (bytes_recv = qrecv(sockfd, buf, datasize, 0)) == 0) { break; }
        total_bytes += bytes_recv;

        if (fwrite(buf,sizeof(char),bytes_recv,fd) != bytes_recv) {
            perror("qrecv_big");
            exit(1);
        }
    }
    fclose(fd);
    return total_bytes;
}

