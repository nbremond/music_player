#ifndef __WIRING__
#define __WIRING__
#include <wiringPi.h>

const int _LOW = LOW;
const int _HIGH = HIGH;

const int _OUTPUT = OUTPUT;
const int _INPUT = INPUT;

int _wiringSetup();
void _pinMode(int a, int b);
void _digitalWrite(int, int);
int _digitalRead(int);


#endif
