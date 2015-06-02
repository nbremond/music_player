#include "wiring.hpp"
int _wiringSetup(){return wiringPiSetup();}
void _pinMode(int a, int b){pinMode(a, b);}
void _digitalWrite(int a, int b){digitalWrite(a, b);}
int _digitalRead(int a){return digitalRead(a);}
