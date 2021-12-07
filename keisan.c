#include <stdio.h>
#define UDP_PORT 5001

#define WIDTH 1280
#define HEIGHT 720
#define COLOR_DEPTH 3

#define false 0
#define true 1

#define SIZE_OF_DATA        1450
#define SIZE_OF_ID          2
#define SIZE_OF_datasize    4
#define SIZE_OF_HEADER      SIZE_OF_ID + SIZE_OF_datasize
#define SIZE_OF_FRAME       (WIDTH * HEIGHT * COLOR_DEPTH)
#define END_OF_ID (SIZE_OF_FRAME/(SIZE_OF_DATA-(SIZE_OF_HEADER)))+1


void main(){
    int a = (1280*720*3/(1450-6))+1;
    int b = END_OF_ID;
    printf("%d\n",b);
}