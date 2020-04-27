/* C library for useful C functions
Author: Joshua Eckels
Include with #include <eckelsjd.h> */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

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

    // check allocation
    if (args == NULL) {
        perror("parse_args");
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
        args[i+1] = "\0";

        // iterate
        i++;
        ptr = strtok(NULL, delim);
    }

    return args;
}

int num_args(char** args) {
    int num = 0;
    for (int i = 0; strcmp(args[i],"\0") != 0;i++) {
        num++;
    }
    return num;
}

void free_args(char** args) {
    for (int i = 0; strcmp(args[i],"\0") != 0; i++) {
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
    fclose(fd);
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
        perror("hasFile");
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

