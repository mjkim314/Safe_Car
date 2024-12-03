#include <wiringPi.h>
#include <stdio.h>
#include <unistd.h> // for sleep()

#define BUZZER_PIN 25  // wiringPi 기준 GPIO 18번 핀

int main() {
    // WiringPi 초기화
    if (wiringPiSetup() == -1) {
        printf("WiringPi 초기화 실패!\n");
        return 1;
    }

    // BUZZER_PIN을 출력으로 설정
    pinMode(BUZZER_PIN, OUTPUT);

    printf("부저 테스트 시작\n");

    while (1) {
        digitalWrite(BUZZER_PIN, HIGH); // 부저 ON
        printf("부저 ON\n");
        sleep(1); // 1초 대기

        digitalWrite(BUZZER_PIN, LOW);  // 부저 OFF
        printf("부저 OFF\n");
        sleep(1); // 1초 대기
    }

    return 0;
}
