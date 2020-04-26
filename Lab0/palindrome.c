/* File: palindrome.c 
   Author: Joshua Eckels
   Date:   3/28/20
   Note:   All non-alpha characters are disregarded. So "!!!!" is
		   not a palindrome. Palindromes are case insensitive. So
		   "AnNa" is a palindrome. 
   
*/
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h> //allows to use "bool" as a boolean type
#include <ctype.h>
#include <string.h>

/*Optional functions, uncomment the next two lines
 * if you want to create these functions after main: */
//bool readLine(char** line, size_t* size, size_t* length);

/* 
  * NOTE that I used char** for the line above... this is a pointer to
  * a char pointer.  I used this because of the availability of
  * a newer function getline which takes 3 arguments (you should look it
  * up) and the first argument is a char**.  You can create a char*, say
  * called var, and to make it a char** just use &var when calling this
  * function.  If this is too confusing, you can use fgets instead.  Feel
  * free to change the function prototypes as you need them.
  * Also, note the use of size_t as a type.  You can look this up, but
  * essentially, this is just a special type of int to track sizes of
  * things like strings...
*/

// recursive helper function
bool palindrome_helper(const char* line, int start, int end) {

	// if start reached the end or vice versa, then we had a string
	// of non-alpha characters only; don't count these as palindromes
	if (start >= strlen(line) || end <= 0) { return false; }

	// skip non-alpha characters
	if (!isalpha(line[start])) {
		return palindrome_helper(line, start + 1, end);
	}
	if (!isalpha(line[end])) {
		return palindrome_helper(line, start, end - 1);
	}

	// base cases
	if (start >= end) {
		return true;
	}
	// ignore case
	if (tolower(line[start]) != tolower(line[end])) {
		return false;
	}
	
	return palindrome_helper(line,start+1, end-1);
}

// recursively determine if a line is a palindrome
bool isPalindrome(const char* line, size_t len) {
	int start = 0;
	int end = len - 1;
	return palindrome_helper(line,start,end);
}

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

int main(int argc, char *argv[]){
	
	char *line = NULL;

	while (true) {
		// get a line from user
		printf("word>");
		line = getaline(line);
	
		// exit on "."
		if (strcmp(line, ".") == 0) { break; }

		// find palindrome
		if (isPalindrome(line, strlen(line))) {
			printf("\"%s\" is a palindrome\n",line);
		} else {
			printf("\"%s\" is not a palindrome\n",line);
		}
		
		free(line);
	}

	free(line);
}
