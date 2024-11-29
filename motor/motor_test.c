#include "header.h"
#include "motor.h"
#include "PWM.h"

int main(int argc, char *argv[]){
	initMotor();
	goForward(1000);
	sleep(5);
	stopMotor();
}
