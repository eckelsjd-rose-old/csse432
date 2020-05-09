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

#define PORT "3490"
#define MAXDATASIZE 1024
#define DEFAULT_DIR "../client_files"

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
    int bytes_recv;
    int bytes_sent;
    char buf[MAXDATASIZE+1];
    while (1) {
        // get a line from user
        printf("client>");
        line = getaline();

        // parse client command line arguments
        args = parse_args(line);

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
                continue;
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
            char *file_buf;
            int filesize = readAll(args[1],&file_buf);
            
            bytes_sent = qsend(sockfd,file_buf,filesize,0);
            
            printf("File \"%s\" sent to server.\n",filename);
            free(filename);
            free(file_buf);
            continue;
        } 
        
        else if (strcmp(args[0],"GET")==0) {
            bytes_sent = qsend(sockfd,"hello",strlen("hello"),0);
            char *path = "client.txt";
            bytes_recv = qrecv_big(sockfd,path,buf,MAXDATASIZE);
            printf("bytes_recv:%d\n",bytes_recv);
            continue;
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
        continue;
    }

    // Clean up
    free_args(args);
    free(line);
    printf("Closing client socket...\n");
    close(sockfd);
}
