# To compile both programs, list both executable names here.
all: paint palindrome

# Creates the executable "paint" using the flag "-o".
paint: paint.o
	gcc paint.o -o paint -g -lm -Wall

# Creates the object file "paint.o" using the flag "-c".
paint.o: paint.c
	gcc paint.c -c -g -lm -Wall

# Add the statements here to compile "palindrome.c" and create "palindrome". 

palindrome: palindrome.o
	gcc palindrome.o -o palindrome -g -Wall -lm

palindrome.o: palindrome.c
	gcc palindrome.c -c -g -lm -Wall

# Delete object files and executatbles to start afresh.
# Be sure to delete the "palindrome" executable also.
# Delete these files BEFORE committing your work. 
clean:
	rm -f *.o paint palindrome
