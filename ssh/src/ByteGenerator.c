#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>

int main(int argc, char* argv[]) {

		int numBytes = atoi(argv[1]);
		int numPerSequence = 1;
		if(argc > 2) {
			numPerSequence = atoi(argv[2]);
		}

		int firstAsciiLetter = 97; //ASCII value of letter 'a'
		int lastAsciiLetter = 122; //ASCII value of letter 'z'
		int curCount = 0;
		int letterIndex = 0;
		int rounds = 0;
		int prevA = 0;
		for (int i = 0; i < numBytes; i++) {
			char tempChar;
			if(curCount >= numPerSequence) {
				letterIndex++;
				curCount = 0;
			}
			tempChar = (char) ((letterIndex%(lastAsciiLetter-firstAsciiLetter+1))+firstAsciiLetter);

			if(tempChar == 'a' && prevA == 0) {
				rounds++;
				printf("%d", rounds);
				char s[25];
        sprintf(s, "%d", rounds);
				i += strlen(s);
				prevA = 1;
			} else if(tempChar != 'a') {
				prevA = 0;
			}
			printf("%c", tempChar);

			curCount++;
		}

    printf("\n\n");
}
