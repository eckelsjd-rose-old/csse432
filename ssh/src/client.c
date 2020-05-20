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
#include <sys/time.h>

#include <eckelsjd.h>

#define MAXDATASIZE 1024
#define MAXFILESIZE 52428800 // 50 MiB
#define LOGIN_SUCCESS 1
#define LOGIN_FUSR 2
#define LOGIN_FPW 3
#define SUCCESS "Success"
#define FAILURE "Failure"

int main(int argc, char** argv) {
    // get hostname from command line
    if (argc != 4) {
        fprintf(stderr,"usage: ./ssh_client user@hostname -p <port number>\n");
        exit(1);
    }

    // separate user and hostname
    char *cpy = malloc(strlen(argv[1])+1);
    memset(cpy,0,strlen(argv[1])+1);
    strcpy(cpy,argv[1]);
    char *hostname = strtok(cpy,"@");
    char *user = hostname;
    hostname = strtok(NULL,"@");
    char* port = argv[3];

    // setup client socket
    int sockfd = setup_client(hostname,port);

    char* line;                 // read input 
    char** args;                // cmdline args
    int bytes_recv;
    int bytes_sent;
    int filesize;
    char buf[MAXDATASIZE+1];    // client queuing buffer
    char *ctmp = "ctmp.txt";    // client temp file
    char *file_buf;             // client temp file buffer
    struct timeval timeout;

    // user authentication
    printf("Password: ");
    line = getaline();
    memset(buf,0,MAXDATASIZE);
    char *ptr = buf;
    strncpy(ptr,user,strlen(user));
    ptr += strlen(user);
    ptr[0] = ' ';
    ptr += 1;
    strcpy(ptr,line);
    bytes_sent = qsend(sockfd,buf,strlen(buf)+1,0);
    memset(buf,0,MAXDATASIZE);
    bytes_recv = qrecv(sockfd,buf,MAXDATASIZE,0);
    int login = atoi(buf);
    int flag = 0;
    switch (login) {
        case LOGIN_SUCCESS :
            printf("Login Successful\n");
            break;
        case LOGIN_FUSR :
            printf("Login failed: Invalid user \"%s\"\n", user);
            flag = 1;
            break;
        case LOGIN_FPW :
            printf("Login failed: Invalid password \"%s\"\n",line);
            flag = 1;
            break;
        default:
            printf("Invalid\n");
            exit(1);
    }
    memset(line,0,strlen(line)+1);
    free(line);

    // invalid login
    if (flag) {
        close(sockfd);
        exit(1);
    }

    // valid login
    // construct user@hostname prompt
    int len = strlen(user) + strlen(hostname) + 5;
    char prompt[len];
    ptr = prompt;
    strncpy(ptr,user,strlen(user));
    ptr += strlen(user);
    ptr[0] = '@';
    ptr += 1;
    strncpy(ptr,hostname,strlen(hostname));
    ptr += strlen(hostname);
    ptr[0] = ' ';
    ptr[1] = ':';
    ptr[2] = ' ';
    ptr[3] = '\0';

    // start CLI for user client program
    while (1) {
        // get working directory from server
//        struct timeval start, end;
//        gettimeofday(&start,NULL);
//        printf("Start: %lu.%lu\n",start.tv_sec,start.tv_usec);
        usleep(100000); // aid in small race conditions when client continues
//        gettimeofday(&end,NULL);
//       printf("End: %lu.%lu\n",end.tv_sec,end.tv_usec);
        bytes_sent = qsend(sockfd,"pwd",strlen("pwd")+1,0);
        bytes_recv = qrecv(sockfd,buf,MAXDATASIZE,0);
        buf[strlen(buf)-1] = '\0'; //trim newline

        // print prompt and working directory
        printf("\033[0;32m"); // green
        printf("%s",prompt);
        printf("\033[0;36m"); // bold cyan
        printf("%s $ ",buf);
        printf("\033[0m"); // default

        // get a line from user
        line = getaline();
        if (strlen(line)==0) {
            bytes_sent = qsend(sockfd,FAILURE,strlen(FAILURE)+1,0);
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
                bytes_sent = qsend(sockfd,FAILURE,strlen(FAILURE)+1,0);
                free_args(args);
                free(line);
                continue;
            }

            if (!isDirectory(args[2])) {
                printf("\"%s\" is not a valid directory.\n",args[2]);
                bytes_sent = qsend(sockfd,FAILURE,strlen(FAILURE)+1,0);
                free_args(args);
                free(line);
                continue;
            }

            // send command to server
            bytes_sent = qsend(sockfd,line,strlen(line)+1,0);

            // wait for a success or failure message from server
            if ( (bytes_recv = qrecv(sockfd, buf, MAXDATASIZE,0)) == 0) { break; }

            // if file was found on server
            if (strcmp(buf,SUCCESS) == 0) {
                // send message to server requesting file transfer to begin
                bytes_sent = qsend(sockfd,SUCCESS,strlen(SUCCESS)+1,0);

                // save file to disk
                bytes_recv = qrecv(sockfd,buf,MAXDATASIZE,0); // get filesize
                filesize = atoi(buf);
                if (filesize > MAXFILESIZE) {
                    printf("File size > 50MiB. Transfer canceled\n");
                    bytes_sent = qsend(sockfd,FAILURE,strlen(FAILURE)+1,0);
                    free_args(args);
                    free(line);
                    continue;
                }
                char *filename = getFilename(args[1]);
                char *fullpath = getPath(args[2],filename);
                bytes_sent = qsend(sockfd,SUCCESS,strlen(SUCCESS)+1,0);

                timeout.tv_sec = 1;
                timeout.tv_usec = 0;
                int total_bytes = qrecv_big(sockfd, fullpath, buf, MAXDATASIZE,&timeout);
                // timeout
                if (total_bytes != filesize) {
                    bytes_sent = qsend(sockfd,FAILURE,strlen(FAILURE)+1,0);
                    remove(fullpath);
                    free(filename);
                    free(fullpath);
                    printf("File transfer timed-out. Closing your connection...\n");
                    break;
                }
                bytes_sent = qsend(sockfd,SUCCESS,strlen(SUCCESS)+1,0);
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
                bytes_sent = qsend(sockfd,FAILURE,strlen(FAILURE)+1,0);
                free_args(args);
                free(line);
                continue;
            }

            if (isDirectory(args[1]) || !isValidPath(args[1])) {
                printf("\"%s\" is not a valid file to send.\n",args[1]);
                bytes_sent = qsend(sockfd,FAILURE,strlen(FAILURE)+1,0);
                free_args(args);
                free(line);
                continue;
            }

            // send command to server
            bytes_sent = qsend(sockfd,line,strlen(line)+1,0);

            // confirm connection from server
            if ( (bytes_recv = qrecv(sockfd, buf, MAXDATASIZE,0)) == 0) { break; }

            if (strcmp(buf,SUCCESS) == 0) {
                // send file to server
                filesize = readAll(args[1],&file_buf);
                memset(buf,0,MAXDATASIZE);
                sprintf(buf,"%d",filesize);
                bytes_sent = qsend(sockfd,buf,strlen(buf)+1,0); // send filesize
                bytes_recv = qrecv(sockfd,buf,MAXDATASIZE,0);

                if (strcmp(buf,FAILURE)==0) {
                    free_args(args); // file was too big
                    free(line);
                    free(file_buf);
                    continue;
                }
                bytes_sent = qsend(sockfd,file_buf,filesize,0);
                printf("Sent bytes: %d\n",filesize);
                bytes_recv = qrecv(sockfd,buf,MAXDATASIZE,0);
                if (strcmp(buf,FAILURE)==0) {
                    free(file_buf); // server timed-out; close-connection
                    printf("File transfer timed-out. Closing your connection...\n");
                    break;
                } else {
                    // server successfully received file
                    free(file_buf);
                    // continue
                }

            } else {
                printf("\"%s\" directory not found on server.\n",args[2]);
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

            bytes_recv = qrecv(sockfd,buf,MAXDATASIZE,0); // handshaking
            bytes_sent = qsend(sockfd,SUCCESS,strlen(SUCCESS)+1,0);

            timeout.tv_sec = 0;
            timeout.tv_usec = 100000;
            // assume no file overflow or timeout issues here
            bytes_recv = qrecv_big(sockfd,ctmp,buf,MAXDATASIZE,&timeout);
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
