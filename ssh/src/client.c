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

#include <eckelsjd.h>

#define MAXDATASIZE 1024
#define DEFAULT_DIR "../client_files"

int main(int argc, char** argv) {
    // get hostname from command line
    if (argc != 3) {
        fprintf(stderr,"usage: ./client <hostname> <port number>\n");
        exit(1);
    }
    char* hostname = argv[1];
    char* port = argv[2];

    // do some kind of user authentication for ssh server

    // setup client socket
    int sockfd = setup_client(hostname,port);

    // start CLI for user client program
    char* line;     // read input 
    char** args;
    int bytes_recv;
    int bytes_sent;
    int filesize;
    char buf[MAXDATASIZE+1];    // client queuing buffer
    char *ctmp = "ctmp.txt";    // client temp file
    char *file_buf;             // client temp file buffer

    while (1) {
        // get a line from user
        printf("client>");
        line = getaline();

        // parse client command line arguments
        args = parse_args(line);

        if (strcmp(args[0], "quit") == 0) { break; }

        // handle custom commands

        // get file from ssh server
        if (strcmp(args[0], "scp") == 0) {
            // send the name of the command to the server first
            printf("Requesting command \"%s\" from server...\n",args[0]);
            bytes_sent = qsend(sockfd,args[0],strlen(args[0])+1,0);

            // confirm connection from server
            if ( (bytes_recv = qrecv(sockfd, buf, MAXDATASIZE,0)) == 0) { break; }
            printf("Server confirmed connection: %s\n",buf);

            // send the filename to the server; format as "file.txt"
            bytes_sent = qsend(sockfd,args[1],strlen(args[1])+1,0);

            // wait for a success or failure message from server
            if ( (bytes_recv = qrecv(sockfd, buf, MAXDATASIZE,0)) == 0) { break; }
            printf("Locating file on server: %s\n",buf);

            // if file was found on server
            if (strcmp(buf,"Success") == 0) {
                // ask for directory from user
                printf("Save file to path: ");
                char *fullpath;
                char *path = getaline();
                if (isValidPath(path)) {
                    // convert to fullpath
                    fullpath = getPath(path,args[1]);
                } else {
                    printf("Invalid path. Using default...\n");
                    fullpath = getPath(DEFAULT_DIR,args[1]);
                }
                free(path);
                printf("Fullpath: %s\n",fullpath);

                // send message to server requesting file transfer to begin
                bytes_sent = qsend(sockfd,"Start",strlen("Start")+1,0);

                // save file to disk
                printf("Writing to disk...\n");
                int total_bytes = qrecv_big(sockfd, fullpath, buf, MAXDATASIZE);
                free(fullpath);
                printf("Write success. Total bytes: %d\n",total_bytes);
                // continue
            }

            // Else could not find file on server
            else {
                printf("File \"%s\" not found on server.\n",args[1]);
                free_args(args);
                free(line);
                continue;
            }

        // send file to server
        } else if (strcmp(args[0], "ssend") == 0) {
            // send the name of the command to the server first
            printf("Requesting command \"%s\" from server...\n",args[0]);
            bytes_sent = qsend(sockfd,args[0],strlen(args[0])+1,0);

            // confirm connection from server
            if ( (bytes_recv = qrecv(sockfd, buf, MAXDATASIZE,0)) == 0) { break; }
            printf("Server confirmed connection: %s\n",buf);

            // check if arg[1] is a file on client system
            if (isValidPath(args[1])) {
                // tell server message is on its way
                bytes_sent = qsend(sockfd,"Start",strlen("Start")+1,0);
            } else {
                // tell server no message is coming
                bytes_sent = qsend(sockfd,"No file",strlen("No file")+1,0);

                printf("File \"%s\" not found.\n",args[1]);
                free_args(args);
                free(line);
                continue;
            }

            // handshaking
            if ( (bytes_recv = qrecv(sockfd, buf, MAXDATASIZE,0)) == 0) { break; }

            // send file name to server
            char *filename = getFilename(args[1]);
            bytes_sent = qsend(sockfd,filename,strlen(filename)+1,0);

            // handshaking
            if ( (bytes_recv = qrecv(sockfd, buf, MAXDATASIZE,0)) == 0) { break; }

            // send file to server
            filesize = readAll(args[1],&file_buf);
            
            bytes_sent = qsend(sockfd,file_buf,filesize,0);
            
            printf("File \"%s\" sent to server.\n",filename);
            free(filename);
            free(file_buf);
            // continue
        } 
        
        // handle generic linux command
        else {
            bytes_sent = qsend(sockfd,line,strlen(line)+1,0);
            bytes_recv = qrecv_big(sockfd,ctmp,buf,MAXDATASIZE);
            filesize = readAll(ctmp,&file_buf);
            printf("%s",file_buf);
            free(file_buf);
            remove(ctmp);
        }

        free_args(args);
        free(line);
        continue;
    }

    // Clean up
    free_args(args);
    free(line);
    printf("Closing ssh connection...\n");
    close(sockfd);
}
