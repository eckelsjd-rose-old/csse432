#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

int main(int argc, char **argv) {
    execlp("echo","echo","hello>text.txt",NULL);
}