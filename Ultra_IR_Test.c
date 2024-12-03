#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/time.h>
#include <fcntl.h>
#include <string.h>

// GPIO 관련 경로
#define GPIO_PATH "/sys/class/gpio"
#define TRIG_PIN 23
#define ECHO_PIN 24
#define IR_PIN 25

// GPIO 핀 초기화
int gpioExport(int pin) {
    char path[50];
    int fd = open(GPIO_PATH "/export", O_WRONLY);
    if (fd == -1) {
        perror("GPIO Export 실패");
        return -1;
    }
    snprintf(path, sizeof(path), "%d", pin);
    write(fd, path, strlen(path));
    close(fd);
    return 0;
}

// GPIO 방향 설정
int gpioSetDirection(int pin, const char *direction) {
    char path[50];
    snprintf(path, sizeof(path), GPIO_PATH "/gpio%d/direction", pin);
    int fd = open(path, O_WRONLY);
    if (fd == -1) {
        perror("GPIO SetDirection 실패");
        return -1;
    }
    write(fd, direction, strlen(direction));
    close(fd);
    return 0;
}

// GPIO 값 읽기
int gpioRead(int pin) {
    char path[50], value;
    snprintf(path, sizeof(path), GPIO_PATH "/gpio%d/value", pin);
    int fd = open(path, O_RDONLY);
    if (fd == -1) {
        perror("GPIO Read 실패");
        return -1;
    }
    read(fd, &value, 1);
    close(fd);
    return value - '0';
}

// GPIO 값 쓰기
int gpioWrite(int pin, int value) {
    char path[50];
    snprintf(path, sizeof(path), GPIO_PATH "/gpio%d/value", pin);
    int fd = open(path, O_WRONLY);
    if (fd == -1) {
        perror("GPIO Write 실패");
        return -1;
    }
    dprintf(fd, "%d", value);
    close(fd);
    return 0;
}

// 거리 측정 함수
float measureDistance() {
    struct timeval start, end;
    long duration;

    // 트리거 핀 초기화
    gpioWrite(TRIG_PIN, 0);
    usleep(2);
    gpioWrite(TRIG_PIN, 1);
    usleep(10);
    gpioWrite(TRIG_PIN, 0);

    // 에코 핀에서 신호가 HIGH로 바뀔 때까지 대기
    while (gpioRead(ECHO_PIN) == 0);
    gettimeofday(&start, NULL);

    // 에코 핀에서 신호가 LOW로 바뀔 때까지 대기
    while (gpioRead(ECHO_PIN) == 1);
    gettimeofday(&end, NULL);

    // 시간 차이를 계산 (마이크로초 단위)
    duration = (end.tv_sec - start.tv_sec) * 1000000 + (end.tv_usec - start.tv_usec);

    // 거리 계산 (음속: 34300 cm/s)
    return (float)(duration * 0.034) / 2;
}

int main() {
    // GPIO 초기화
    gpioExport(TRIG_PIN);
    gpioExport(ECHO_PIN);
    gpioExport(IR_PIN);

    gpioSetDirection(TRIG_PIN, "out");
    gpioSetDirection(ECHO_PIN, "in");
    gpioSetDirection(IR_PIN, "in");

    while (1) {
        float distance = measureDistance();
   int irvalue = gpioRead(IR_PIN);
   printf("%d ", irvalue);
        printf("측정된 거리: %.2f cm\n", distance);
        sleep(1);  // 1초 간격으로 측정
    }

    return 0;

}
