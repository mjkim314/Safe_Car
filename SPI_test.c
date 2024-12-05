#include <wiringPiSPI.h>
#include <stdio.h>
#include <unistd.h>

#define SPI_CHANNEL 0
#define SPI_SPEED 500000

int main() {
    
    if (wiringPiSPISetup(SPI_CHANNEL, SPI_SPEED) == -1) {
        printf("SPI 설정 실패!\n");
        return 1;
    }

    printf("SPI 마스터 시작...\n");

    while (1) {
        unsigned char data[2] = {0xA5, 0x00}; // 송신 데이터
        printf("송신 데이터: 0x%02X 0x%02X\n", data[0], data[1]);
        wiringPiSPIDataRW(SPI_CHANNEL, data, 2);
        printf("수신된 데이터: 0x%02X 0x%02X\n", data[0], data[1]);

        sleep(1); // 1초 대기
    }

    return 0;
}