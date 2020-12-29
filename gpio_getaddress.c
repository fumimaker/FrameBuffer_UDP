#include <stdio.h>
#include <stdlib.h>
#include <wiringPi.h>
#define  LED0 26
int main (void){
    wiringPiSetupGpio();
    pinMode(LED0, OUTPUT);

    for(;;){
        digitalWrite(LED0,HIGH);
        //delay(1);
        digitalWrite(LED0, LOW);
        //delay(1);
    }
    return EXIT_SUCCESS;
}