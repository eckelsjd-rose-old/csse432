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
        if (strlen(line)==0) {
            free(line);
            continue;
        }

        // parse client command line arguments
        args = parse_args(line);

        if (strcmp(args[0], "quit") == 0) { break; }

        // handle custom commands

        // get file from ssh server
        if (strcmp(args[0], "scp") == 0) {
            // check args
            if (num_args(args) != 3) {
                printf("Usage: scp <server_path_to_file> <host_directory>\n");
                free_args(args);
                free(line);
                continue;
            }

            if (!isDirectory(args[2])) {
                printf("\"%s\" is not a valid directory.\n",args[2]);
                free_args(args);
                free(line);
                continue;
            }

            // send command to server
            bytes_sent = qsend(sockfd,line,strlen(line)+1,0);

            // wait for a success or failure message from server
            if ( (bytes_recv = qrecv(sockfd, buf, MAXDATASIZE,0)) == 0) { break; }

            // if file was found on server
            if (strcmp(buf,"Success") == 0) {
                // send message to server requesting file transfer to begin
                bytes_sent = qsend(sockfd,"Start",strlen("Start")+1,0);

                char *filename = getFilename(args[1]);
                char *fullpath = getPath(args[2],filename);
                // save file to disk
                int total_bytes = qrecv_big(sockfd, fullpath, buf, MAXDATASIZE);
                printf("Write success. Total bytes: %d\n",total_bytes);
                free(filename);
                free(fullpath);
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
            // check args
            if (num_args(args) != 3) {
                printf("Usage: ssend <host_path_to_file> <server_directory>\n");
                free_args(args);
                free(line);
                continue;
            }

            if (isDirectory(args[1]) || !isValidPath(args[1])) {
                printf("\"%s\" is not a valid file to send.\n",args[1]);
                free_args(args);
                free(line);
                continue;
            }

            // send command to server
            bytes_sent = qsend(sockfd,line,strlen(line)+1,0);

            // confirm connection from server
            if ( (bytes_recv = qrecv(sockfd, buf, MAXDATASIZE,0)) == 0) { break; }

            if (strcmp(buf,"Success") == 0) {
                // send file to server
                filesize = readAll(args[1],&file_buf);
                bytes_sent = qsend(sockfd,file_buf,filesize,0);
                printf("Sent bytes: %d\n",filesize);
                free(file_buf);

            } else {
                printf("\"%s\" not found on server.\n",args[2]);
                free_args(args);
                free(line);
                continue;
            }
            // continue
        } 
        
        // handle generic linux command
        // only can handle text response from server
        // any kind of graphical program is off the charts (vim, gedit, zsh, etc.)
        // also will not receive anything if server is too slow to respond (qrecv_big "bug")
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
