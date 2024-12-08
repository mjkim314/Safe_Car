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

#define MAX_INPUT 400    // x, y 값의 최대 절댓값
#define PWM_SCALE 500000  // PWM 값 스케일

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
    sleep(3);
}

void changeDutyCycle(int x, int y) {
    // 방향 설정 (전진/후진)
    if (y < 0) {  // backward
        digitalWrite(IN1, LOW);
        digitalWrite(IN2, HIGH);
        digitalWrite(IN3, LOW);
        digitalWrite(IN4, HIGH);
    } else {  // forward
        digitalWrite(IN1, HIGH);
        digitalWrite(IN2, LOW);
        digitalWrite(IN3, HIGH);
        digitalWrite(IN4, LOW);
    }

    // y == 0일 때 듀티 사이클 0으로 설정
    if ( y > -100 && y < 100 ) {
        stopMotor();
        //printf("Y == 0, no movement\n");
        return;
    }
    int dutyA = 0, dutyB = 0;
    // 기본 속도는 y에 비례
    int baseDuty = abs(y) * 22000;  

    // x > 0 -> 우회전, x < 0 -> 좌회전
    if (x < -100) {  // 우회전
        // 우회전 정도에 따라 속도 세분화
        dutyA = baseDuty;   // 왼쪽 모터는 속도 감소
        dutyB = baseDuty * 0.4; // 오른쪽 모터는 최대 속도 유지
    } else if (x > 100) {  // 좌회전
        // 좌회전 정도에 따라 속도 세분화
        dutyA = baseDuty * 0.4;  // 왼쪽 모터는 최대 속도 유지
        dutyB = baseDuty;  // 오른쪽 모터는 속도 감소
    } else {  // 직진
        dutyA = baseDuty;
        dutyB = baseDuty;
    }
    // PWM 신호 설정
    PWMWriteDutyCycle(ENA, abs(dutyA));
    PWMWriteDutyCycle(ENB, abs(dutyB)+330000); //모터 하드웨어 차이로 인한 조정

    // 디버그 출력
    printf("X:%d ## Y:%d ## DutyA:%d ## DutyB:%d\n", x, y, dutyA / 10000, dutyB / 10000);
}



/*
void changeDutyCycle(int x, int y) {
    // 방향 설정 (전진/후진)
    if (y < 0) {  // backward
        digitalWrite(IN1, LOW);
        digitalWrite(IN2, HIGH);
        digitalWrite(IN3, HIGH);
        digitalWrite(IN4, LOW);
    } else {  // forward
        digitalWrite(IN1, HIGH);
        digitalWrite(IN2, LOW);
        digitalWrite(IN3, LOW);
        digitalWrite(IN4, HIGH);
    }
    
        int dutyA, dutyB;

    
    // 벡터 정규화
    float normX = (float)x / MAX_INPUT;  // x의 정규화 값 [-1, 1]
    float normY = (float)y / MAX_INPUT;  // y의 정규화 값 [-1, 1]
    float magnitude = sqrt(normX * normX + normY * normY);

    // 각도 계산 (0~360도)
    float angle = atan2(normY, normX);  // [-π, π]
    if (angle < 0) angle += 2 * M_PI;  // [0, 2π]

    // 모터 속도 계산
    if (magnitude < 0.1) {  // 움직임이 거의 없는 경우
        dutyA = 0;
        dutyB = 0;
    } else if (angle < M_PI / 8 || angle >= 15 * M_PI / 8) {  // 0° ~ 22.5° 또는 337.5° ~ 360°
        dutyA = PWM_SCALE * magnitude;
        dutyB = PWM_SCALE * magnitude;  // 직진
    } else if (angle < 3 * M_PI / 8) {  // 22.5° ~ 67.5° (약간 우회전)
        dutyA = PWM_SCALE * magnitude * 0.8;
        dutyB = PWM_SCALE * magnitude;  // 오른쪽 속도 유지
    } else if (angle < 5 * M_PI / 8) {  // 67.5° ~ 112.5° (큰 우회전)
        dutyA = PWM_SCALE * magnitude * 0.5;
        dutyB = PWM_SCALE * magnitude;  // 오른쪽 모터 우선
    } else if (angle < 7 * M_PI / 8) {  // 112.5° ~ 157.5° (오른쪽 회전)
        dutyA = PWM_SCALE * magnitude * 0.3;
        dutyB = PWM_SCALE * magnitude;  // 강하게 회전
    } else if (angle < 9 * M_PI / 8) {  // 157.5° ~ 202.5° (왼쪽 회전)
        dutyA = PWM_SCALE * magnitude;
        dutyB = PWM_SCALE * magnitude * 0.3;  // 강하게 회전
    } else if (angle < 11 * M_PI / 8) {  // 202.5° ~ 247.5° (큰 좌회전)
        dutyA = PWM_SCALE * magnitude;
        dutyB = PWM_SCALE * magnitude * 0.5;  // 왼쪽 모터 우선
    } else if (angle < 13 * M_PI / 8) {  // 247.5° ~ 292.5° (약간 좌회전)
        dutyA = PWM_SCALE * magnitude;
        dutyB = PWM_SCALE * magnitude * 0.8;  // 왼쪽 속도 유지
    } else {  // 292.5° ~ 337.5° (직진)
        dutyA = PWM_SCALE * magnitude;
        dutyB = PWM_SCALE * magnitude;
    }
    if (y ==0){
        dutyA = 0; 
        dutyB = 0;

    }
    // PWM 신호 설정
    PWMWriteDutyCycle(ENA, abs(dutyA));
    PWMWriteDutyCycle(ENB, abs(dutyB));

    // 디버그 출력
    printf("X:%d Y:%d DutyA:%d DutyB:%d Angle:%.2f°\n", x, y, dutyA, dutyB, angle * 180 / M_PI);
}*/


#endif
