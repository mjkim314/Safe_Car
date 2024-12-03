#include <wiringPi.h>
#include <stdio.h>
#include <unistd.h> // for sleep()

#define BUZZER_PIN 26  // wiringPi 기준 GPIO 18번 핀 (PWM 가능)

void playTone(int frequency, int duration) {
    int delay = 1000000 / frequency / 2; // 주기 계산
    int cycles = frequency * duration / 1000; // 주어진 시간 동안 사이클 수 계산

    for (int i = 0; i < cycles; i++) {
        digitalWrite(BUZZER_PIN, HIGH); // 부저 ON
        usleep(delay);                 // 고음 유지 시간
        digitalWrite(BUZZER_PIN, LOW);  // 부저 OFF
        usleep(delay);                 // 저음 유지 시간
    }
}

int main() {
    // WiringPi 초기화
    if (wiringPiSetup() == -1) {
        printf("WiringPi 초기화 실패!\n");
        return 1;
    }

    // BUZZER_PIN을 출력으로 설정
    pinMode(BUZZER_PIN, OUTPUT);

    printf("부저 테스트 시작\n");

    // 다양한 음계 연주
    //playTone(440, 500);  // A4 음 (440 Hz), 500ms
    int melody[] = {440, 494, 523, 587, 659, 698, 784}; // 도레미파솔라시
    int noteDurations[] = {500, 500, 500, 500, 500, 500, 500}; // 각 음의 지속 시간

    int song[] = {1318, 1396, 1567};
    while(1){
        for (int i=0; i<3; i++){
            playTone(song[i], 600);
        }
    }
   // for (int i = 0; i < 7; i++) {
   //     playTone(melody[i], noteDurations[i]);
   //     usleep(50000); // 음 사이의 간격
   // }


    printf("테스트 종료\n");
    return 0;
}
