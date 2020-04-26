#include <string.h>
#include <stdio.h>

int length(void* array, int type) {
     int count = 0;
     if (type == 0) {
         int* int_array = (int*) array;
         for (int i = 0; i < 10; i++) {
             if (int_array[i] != 0) { count++;}
         }
     }
     else if (type == 1) {
         double* float_array = (double*) array;
         for (int i = 0; i < 10; i++) {
             if (float_array[i] != 0) { count++;}
         }
     }
     return count;
}

int main() {
	int array[10];
	double float_array[10];
	memset(array,0,sizeof(array));
	memset(float_array,0,sizeof(float_array));
	for (int i = 0; i < sizeof(array)/sizeof(array[0]);i++) {
		array[i] = (i+1) % 3;
		float_array[i] = i + 0.2*i + 1.37;
		printf("Float: %f Int: %d\n",float_array[i], array[i]);
	}
	printf("Float length: %d Int length: %d\n",length(&float_array,1),length(&array,0));
}
