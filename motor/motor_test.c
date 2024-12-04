#include "header.h"
#include "motor.h"
#include "PWM.h"

int main(int argc, char *argv[]){
	initMotor();
	while(1){
		changeDutyCycle(0, 400);
		sleep(2);
		changeDutyCycle(150, 400);
		sleep(2);
		changeDutyCycle(-150, 400);
		sleep(2);
		stopMotor();
		sleep(2);
		// changeDutyCycle(150, 400);
		// sleep(2);
		// stopMotor();
		// sleep(2);
		// changeDutyCycle(-150, 400);
		// sleep(2);
		// stopMotor();
		// sleep(2);
		// goBackward();
		// sleep(2);
		// stopMotor();
		// sleep(2);
		// changeDutyCycle(-200, 400);
		// sleep(2);
		// stopMotor();
		// sleep(2);
	}
}
