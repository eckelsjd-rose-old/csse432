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

// this function dynamically allocates space from user input
// returns a pointer to the line read from stdin
char* getaline(char *line) {
    int ch; // getchar() returns an int
    
    line = malloc(sizeof(char));
    
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

    // prompt for user input and send to server
    char* line = NULL;
    while (1) {
        // get a line from user
        printf("client>");
        line = getaline(line);

        // exit on "."
        if (strcmp(line, ".") == 0) { break; }

        // send the string to the server (with null-terminator)
        int bytes_sent = send(sockfd,line,strlen(line)+1,0);
        if (bytes_sent == -1) {
            perror("client: send");
            exit(1);
        } else if (bytes_sent != strlen(line)+1) {
            // handle incorrect number of bytes sent
            printf("Msg length: %ld. Bytes sent: %d.\n",strlen(line)+1,bytes_sent);
            exit(1);
        }

        // wait for reply from server
        char buf[MAXDATASIZE+2];
        int bytes_recv = recv(sockfd, buf, MAXDATASIZE+1,0);
        if (bytes_recv == -1) {
            perror("client: recv");
            exit(1);
        }
        buf[bytes_recv] = '\0';

        printf("client received: %s\n",buf);

        free(line);
    }

    free(line);
    printf("Closing client socket...\n");
    close(sockfd);
}
