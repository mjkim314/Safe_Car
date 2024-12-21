#ifndef __MOTOR__
#define __MOTOR__

#include "header.h"
#include "PWM.h"

#define ENA 18 // ENA 핀
#define IN1 17  // IN1 핀
#define IN2 27  // IN2 핀
#define ENB 13  // ENB 핀
#define IN3 23 // IN3 핀
#define IN4 24  // IN4 핀

#define MAX_INPUT 400    // x, y 값의 최대 절댓값
#define PWM_SCALE 500000  // PWM 값 스케일

#define PWM_RANGE 1024  // PWM 범위

//motor 초기화
void initMotor() {
    wiringPiSetupGpio();  // GPIO 핀 번호 사용
    pwmSetMode(PWM_MODE_MS); //WiringPi 내장 PWM 사용

    pinMode(IN1, OUTPUT);
    pinMode(IN2, OUTPUT);

    pinMode(IN3, OUTPUT);
    pinMode(IN4, OUTPUT);

    //PWMinit(ENA);
    //PWMinit(ENB);
    pinMode(ENA, PWM_OUTPUT);
    pinMode(ENB, PWM_OUTPUT); //핀 지정
}

//모터 정지 함수
void stopMotor() {
    digitalWrite(IN1, LOW);
    digitalWrite(IN2, LOW);
    digitalWrite(IN3, LOW);
    digitalWrite(IN4, LOW);

    pwmWrite(ENA, 0);
    pwmWrite(ENB, 0);

    //PWMWriteDutyCycle(ENA, 0);
    //PWMWriteDutyCycle(ENB, 0);
}

//서서히 속도를 줄여 정지
void slowStop(int lastspd) {
    int spd = abs(lastspd);
    for(int i = spd; i > 0; i--){
        //PWMWriteDutyCycle(ENA, i);
        //PWMWriteDutyCycle(ENB, i);
	pwmWrite(ENA, i);
	pwmWrite(ENB, i);
    }
    stopMotor();
    sleep(3);
}

//모터 방향 및 속도 조절함수
void changeDutyCycle(int x, int y) {
    // 방향 설정 (전진/후진)
    if (y > 0) {  // fowward
        digitalWrite(IN1, LOW);
        digitalWrite(IN2, HIGH);
        digitalWrite(IN3, LOW);
        digitalWrite(IN4, HIGH);
    } else {  // backward
        digitalWrite(IN1, HIGH);
        digitalWrite(IN2, LOW);
        digitalWrite(IN3, HIGH);
        digitalWrite(IN4, LOW);
    }

    // y값이 -100 ~ 100일 때 듀티 사이클 0으로 설정
    if ( y > -100 && y < 100 ) { //-100 ~ 100일 시 모터 전력 부족으로 움직이지는 않지만 경우의 수 제거
        stopMotor();
        return;
    }
    int dutyA = 0, dutyB = 0;
    // 기본 속도는 y에 비례
    int baseDuty = abs(y) * 2.2;  

    // x > 0 -> 우회전, x < 0 -> 좌회전
    if (x < -100) {  // 좌회전
        // 좌회전 정도에 따라 속도 세분화
        dutyA = baseDuty;   // 왼쪽 모터는 최대 속도 유지
        dutyB = baseDuty * 0.4; // 오른쪽 모터는 속도 감소
    } else if (x > 100) {  // 우회전
        // 우회전 정도에 따라 속도 세분화
        dutyA = baseDuty * 0.4;  // 왼쪽 모터는 속도 감소
        dutyB = baseDuty;  // 오른쪽 모터는 최대 속도 유지
    } else {  // 직진
        dutyA = baseDuty;
        dutyB = baseDuty;
    }
    // PWM 신호 설정
    pwmWrite(ENA, abs(dutyA));
    pwmWrite(ENB, abs(dutyB));
    //PWMWriteDutyCycle(ENA, abs(dutyA));
    //PWMWriteDutyCycle(ENB, abs(dutyB)+320000); //모터 하드웨어 차이로 인한 조정

    // 디버그 출력
    printf("X:%d ## Y:%d ## DutyA:%d ## DutyB:%d\n", x, y, dutyA, dutyB);
}

#endif
