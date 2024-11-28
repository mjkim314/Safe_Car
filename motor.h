#ifndef __MOTOR__
#define __MOTOR__

#include "header.h"
#include "PWM.h"

#define ENA 1 // ENA 핀
#define IN1 0  // IN1 핀
#define IN2 2  // IN2 핀
#define ENB 26  // ENB 핀
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

void goForward(int spd) {
    digitalWrite(IN1, LOW);
    digitalWrite(IN2, HIGH);
    digitalWrite(IN3, HIGH);
    digitalWrite(IN4, LOW);

    PWMWriteDutyCycle(ENA, spd * 1000);
    PWMWriteDutyCycle(ENB, spd * 1000);
}

void goBackward(int spd) {
    digitalWrite(IN1, HIGH);
    digitalWrite(IN2, LOW);
    digitalWrite(IN3, LOW);
    digitalWrite(IN4, HIGH);

    PWMWriteDutyCycle(ENA, spd * 1000);
    PWMWriteDutyCycle(ENB, spd * 1000);
}

void turnLeft(int spd) {
    digitalWrite(IN1, HIGH);
    digitalWrite(IN2, LOW);
    digitalWrite(IN3, HIGH);
    digitalWrite(IN4, LOW);

    PWMWriteDutyCycle(ENA, spd * 1000);
    PWMWriteDutyCycle(ENB, spd * 1000);
}

    void turnRight(int spd) {
    digitalWrite(IN1, LOW);
    digitalWrite(IN2, HIGH);
    digitalWrite(IN3, LOW);
    digitalWrite(IN4, HIGH);

    PWMWriteDutyCycle(ENA, spd * 1000);
    PWMWriteDutyCycle(ENB, spd * 1000);
}

#endif
