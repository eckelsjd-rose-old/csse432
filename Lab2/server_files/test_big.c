#include <stdio.h>
#include <string.h>
#include <stdlib.h>

int test_malloc(char*name,char**buf) {
    int sum = strlen(name);
    *buf = malloc(sizeof(char) * strlen(name) + 1);
    strcpy(*buf,name);
    int i = 0;
    *buf[i] = 'h';
    return sum;
}

int isValidPath(char *path) {
    FILE *fptr = fopen(path, "r");
    if (fptr == NULL) {
        return 0;
    }
    fclose(fptr);
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

void free_args(char** args) {
    for (int i = 0; (strcmp(args[i],"\0") != 0); i++) {
        free(args[i]);
    }
    free(args);
}

char** parse_args(char* cmdline, char** args) {
    args = malloc(sizeof(char*));
    if (args == NULL) {
        perror("Unsuccessful allocation");
        exit(1);
    }
    args[0] = "\0";
    char delim[] = " ";
    char *ptr = strtok(cmdline, delim);
    int i = 0;
    while (ptr != NULL) {
        args = realloc(args, (i+2)*sizeof(char*));

        if (args == NULL) {
            perror("Unsuccessful allocation");
            exit(1);
        }

        char* new_word = malloc(strlen(ptr)+1);
        new_word = strcpy(new_word,ptr);

        args[i] = new_word;
        args[i+1] = "\0";

        i++;
        ptr = strtok(NULL, delim);
    }

    return args;
}

int main(int argc, char** argv) {
    //char cmd[] = "one two three fourfive six 123lkj";
    char cmd[] = "\n";
    char** args;
    args = parse_args(cmd,args);
    /*
    char *strings[5];
    strings[0] = "zero";
    strings[1] = "one";
    strings[2] = "two";
    strings[3] = "three";
    strings[4] = "\0";
    */
    printf("num: %d\n",num_args(args));
    for (int i = 0; (strcmp(args[i],"\0") != 0); i++) {
        printf("%s\n",args[i]);
    }
    free_args(args);

    // check file path
    char *path = "/";
    printf("File path \"%s\" exists: %d\n",path,isValidPath(path));
    printf("Start length: %ld\n",strlen("start"));
    printf("size of char: %ld\n",sizeof(char));

    // check buffer
    char *filename;
    int length = test_malloc(argv[0],&filename);
    printf("Filename: %s Length: %d\n",filename,length);
    free(filename);
}
