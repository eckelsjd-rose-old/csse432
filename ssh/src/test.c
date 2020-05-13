#include <stdio.h>
#include <sys/wait.h>
#include <signal.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <dirent.h>


int main(int argc, char **argv) {
    DIR *fd = opendir(argv[1]);
    if (fd != NULL) {
        printf("is directory\n");
        closedir(fd);
        return 1;
    }
    printf("Not\n");
    return 0;
}
