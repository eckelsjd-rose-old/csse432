/* Header for libeckelsjd.so C library
Author: Joshua Eckels */

/* User input functions */

/* Parse a command line into a list of arguments
Parameters:
    char *cmdline : a string containing command line input
Returns:
    char **args   : array of cmd line arguments
    Dynamically allocates memory to char **args to hold user input
    Last element in char **args is "\0"
User:
    Responsible for freeing args with a call to free_args
*/
char** parse_args(char *cmdline);

/* Frees allocated memory from a call to parse_args */
void free_args(char **args);

/* Counts number of arguments in args array */
int num_args(char **args);

/* Reads a line of input from user on stdin
Parameters:
    None
Returns:
    char *line : a string containing all user input
    Dynamically allocates memory to char *line to hold user input
User:
    Responsible for freeing char *line when finished
*/
char* getaline();


/* File I/O functions */

/* Reads an entire file into a buffer
Parameters:
    char *path : path to file (absolute or relative)
    char **buf : empty pointer to a buffer
Returns:
    int retval : number of bytes in file; -1 on fail
    Allocates memory to *buf to hold file data
User:
    Responsible for passing in empty char **buf and 
    freeing *buf when finished
*/
int readAll(char *path, char **buf);

/* Determines if a path is valid (absolute or relative)
Parameters:
    char *path      : path to file (absolute or relative)
Returns:
    int retval      : 0 if invalid path; 1 if valid path
*/
int isValidPath(char *path);

/* Gets the trimmed filename from a valid path. 
   User must free the returned char *filename */
char *getFilename(char *path);

/* Gets the full path for a given file in a directory
Parameters:
    char *dir      : starting directory to begin search
    char *filename : trimmed name of file (example.txt)
Returns:
    char *path     : string containing path to the file
    Allocates memory to path to hold the directory path of the file
User:
    Responsible for freeing char *path
*/
char* getPath(char *dir, char *filename);


/* Socket functions */
/*
   send
   recv
   recv_big
   setup_client
   setup_server
*/
