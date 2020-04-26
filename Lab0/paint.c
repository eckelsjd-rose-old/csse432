/* File: paint.c 
   Author: 
   
*/
#define CAN_COVERAGE 200
#define MAX_LENGTH 50

#include <stdio.h>
#include <math.h>
#include <stdlib.h>

/*Optional functions, uncomment the next two lines
 * if you want to create these functions after main: */
//float readDimension(const char* name);
//float calcArea(float width, float height, float depth);

int main(int argc, char *argv[]){
	char length[MAX_LENGTH];
	char width[MAX_LENGTH];
	char height[MAX_LENGTH];

	// get dimensions
	printf("Length>");
	scanf("%s",length);
	printf("Width>");
	scanf("%s",width);
	printf("Height>");
	scanf("%s",height);

	// convert to floats
	float l_int = strtof(length,NULL);
	float w_int = strtof(width,NULL);
	float h_int = strtof(height,NULL);

	// calculate surface area
	float sa = 2*l_int*w_int + 2*l_int*h_int + 2*w_int*h_int;
	
	// determine number of paint cans needed
	int num_cans = (int) ceil(sa/CAN_COVERAGE);
	
	printf("Total Area: %.3f\n",sa);
	printf("Number of cans required: %d\n",num_cans);
}
