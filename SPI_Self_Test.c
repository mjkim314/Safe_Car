#include <stdio.h>
#include <stdlib.h>
#include <wiringPiSPI.h>
#include <unistd.h>

#define SPI_CHANNEL 0   // SPI 채널 (CE0)
#define SPI_SPEED 500000 // SPI 속도 (500kHz)

#define TRIG_PIN 23
#define ECHO_PIN 24

void sendTriggerSignal() {
    unsigned char data[2];
    data[0] = 0x01; // Start signal
    data[1] = 0x00; // Trigger configuration
    wiringPiSPIDataRW(SPI_CHANNEL, data, 2); // SPI 전송
}

float readEchoSignal() {
    unsigned char data[2];
    data[0] = 0x02; // Echo start
    data[1] = 0x00; // Dummy byte for read
    wiringPiSPIDataRW(SPI_CHANNEL, data, 2); // SPI 전송 및 데이터 수신
    int echoTime = (data[0] << 8) | data[1]; // 데이터 조합
    return echoTime * 0.034 / 2;             // 거리 계산
}

int main() {
    if (wiringPiSPISetup(SPI_CHANNEL, SPI_SPEED) == -1) {
        printf("SPI 설정 실패\n");
        exit(1);
    }

    while (1) {
        sendTriggerSignal();
        usleep(10000); // 트리거 신호 대기

        float distance = readEchoSignal();
        printf("거리: %.2f cm\n", distance);

        sleep(1); // 1초 간격 측정
    }

    return 0;
}
