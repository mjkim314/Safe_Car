#ifndef __MOTOR__
#define __MOTOR__

#include "header.h"
#include "PWM.h"
#include "math.h"

#define ENA 0 // ENA 핀 23
#define IN1 0  // IN1 핀
#define IN2 2  // IN2 핀
#define ENB 1  // ENB 핀 26
#define IN3 4 // IN3 핀
#define IN4 5  // IN4 핀

#define PWM_RANGE 1024  // PWM 범위

//motor 초기화
void initMotor() {
    wiringPiSetup();  // GPIO 핀 번호 사용

    pinMode(IN1, OUTPUT);
    pinMode(IN2, OUTPUT);

    pinMode(IN3, OUTPUT);
    pinMode(IN4, OUTPUT);

    PWMinit(ENA);
    PWMinit(ENB);
}

//stop
void stopMotor() {
    digitalWrite(IN1, LOW);
    digitalWrite(IN2, LOW);
    digitalWrite(IN3, LOW);
    digitalWrite(IN4, LOW);
    
    PWMWriteDutyCycle(ENA, 0);
    PWMWriteDutyCycle(ENB, 0);
}

void slowStop(int lastspd) {
    int spd = abs(lastspd);
    for(int i = spd; i > 0; i--){
        PWMWriteDutyCycle(ENA, i);
        PWMWriteDutyCycle(ENB, i);
    }
    stopMotor();
    sleep(7);
}

void emerBrake(int y){
	for (int i = 0; i < 3; i++){
		PWMWriteDutyCycle(ENA, 200);
		PWMWriteDutyCycle(ENB, 200);
		sleep(0.5);
		PWMWriteDutyCycle(ENA, 400);
		PWMWriteDutyCycle(ENB, 400);
		sleep(1);
	}
}

void goForward(int spd) {
    digitalWrite(IN1, LOW);
    digitalWrite(IN2, HIGH);
    digitalWrite(IN3, LOW);
    digitalWrite(IN4, HIGH);

    PWMWriteDutyCycle(ENA, spd * 10000);
    PWMWriteDutyCycle(ENB, spd * 10000);
}

void goBackward() {
    digitalWrite(IN1, HIGH);
    digitalWrite(IN2, LOW);
    digitalWrite(IN3, HIGH);
    digitalWrite(IN4, LOW);

    PWMWriteDutyCycle(ENA, 800 * 10000);
    PWMWriteDutyCycle(ENB, 400 * 10000);
}

void changeDutyCycle(int x, int y, int prey){
	if(y == 0){
		stopMotor();
	}
	else if (y > 0){ //foward
		if(y > 0 && prey <= 0){
       			 digitalWrite(IN1, LOW);
       			 digitalWrite(IN2, HIGH);
       			 digitalWrite(IN3, LOW);
        		 digitalWrite(IN4, HIGH);
		}
        if (x == 0){//그냥 전진
            PWMWriteDutyCycle(ENA, 400 * 20000);
	        PWMWriteDutyCycle(ENB, 400 * 20000);
        }else if(x < 0){//x값 따라
            PWMWriteDutyCycle(ENA, 200 * 20000);
	        PWMWriteDutyCycle(ENB, 400 * 20000);
        }else if(x > 0){//x값 따라
            PWMWriteDutyCycle(ENA, 400 * 20000);
	        PWMWriteDutyCycle(ENB, 200 * 20000);
        }
    }
    else{ //backward
	    if(y < 0 && prey >= 0){
		    	digitalWrite(IN1, HIGH);
        		digitalWrite(IN2, LOW);
        		digitalWrite(IN3, HIGH);
        		digitalWrite(IN4, LOW);
		}
        if (x == 0){//그냥 후진
            PWMWriteDutyCycle(ENA, 400 * 20000);
	        PWMWriteDutyCycle(ENB, 400 * 20000);
        }else if(x < 0){//x값 따라
            PWMWriteDutyCycle(ENA, 200 * 20000);
	        PWMWriteDutyCycle(ENB, 400 * 20000);
        }else if(x > 0){//x값 따라
            PWMWriteDutyCycle(ENA, 400 * 20000);
	        PWMWriteDutyCycle(ENB, 200 * 20000);
        }
    }
    printf("### X:%d ### Y:%d ### PreY:%d \n", x, y, prey);
}
#endif
