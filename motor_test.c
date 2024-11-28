#include "header.h"
#include "motor.h"
#include "PWM.h"

int main(int argc, char *argv[]){
	initMotor();
	while(1){
		goForward(1000);
	}
}
