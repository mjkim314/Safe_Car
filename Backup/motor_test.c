#include "header.h"
#include "motor.h"
#include "PWM.h"

int main(int argc, char *argv[]){
	initMotor();
	// while(1){
	changeDutyCycle(0, 200);
	sleep(2);
	changeDutyCycle(0, 300);
	sleep(2);
	changeDutyCycle(0, 400);
	sleep(2);
	// 	stopMotor();
	// 	sleep(2);
	// 	// changeDutyCycle(150, 400);
	// 	// sleep(2);
	// 	// stopMotor();
	// 	// sleep(2);
	// 	// changeDutyCycle(-150, 400);
	// 	// sleep(2);
	// 	// stopMotor();
	// 	// sleep(2);
	// 	// goBackward();
	// 	// sleep(2);
	// 	// stopMotor();
	// 	// sleep(2);
	// 	// changeDutyCycle(-200, 400);
	// 	// sleep(2);
	// 	// stopMotor();
	// 	// sleep(2);
	// }
	// for(int i = 150; i < 400; i++){
	// 	changeDutyCycle(0, i);
	// 	printf("%d", i);
	// 	usleep(90000);
	// }
	// sleep(1);
	// for(int i = 400; i > 150; i--){
	// 	changeDutyCycle(0, i);
	// 	usleep(90000);
	// }
	stopMotor();
}
