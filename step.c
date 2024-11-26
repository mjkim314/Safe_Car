#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#define IN 0
#define OUT 1

#define LOW 0
#define HIGH 1

#define IN1 17 // 모터 IN1
#define IN2 18 // 모터 IN2
#define IN3 27 // 모터 IN3
#define IN4 22 // 모터 IN4

#define VALUE_MAX 40
#define DIRECTION_MAX 40

// 스텝 시퀀스 정의 (HALF-STEP)
static int step_seq[8][4] = {
    {1, 0, 0, 0},
    {1, 1, 0, 0},
    {0, 1, 0, 0},
    {0, 1, 1, 0},
    {0, 0, 1, 0},
    {0, 0, 1, 1},
    {0, 0, 0, 1},
    {1, 0, 0, 1}
};

// GPIO 관련 함수
static int GPIOExport(int pin);
static int GPIOUnexport(int pin);
static int GPIODirection(int pin, int dir);
static int GPIOWrite(int pin, int value);

void rotate_motor(int direction, int steps, int delay_ms) {
    int i, step;

    for (step = 0; step < steps; step++) {
        for (i = 0; i < 8; i++) {
            // 방향에 따라 스텝 순서 변경
            int idx = direction == 1 ? i : 7 - i;

            GPIOWrite(IN1, step_seq[idx][0]);
            GPIOWrite(IN2, step_seq[idx][1]);
            GPIOWrite(IN3, step_seq[idx][2]);
            GPIOWrite(IN4, step_seq[idx][3]);

            usleep(delay_ms * 1000); // 딜레이
        }
    }
}

static int GPIOExport(int pin) {
	#define BUFFER_MAX 3
	char buffer[BUFFER_MAX];
	ssize_t bytes_written;
	int fd;
	fd = open("/sys/class/gpio/export", O_WRONLY);
	if (-1 == fd) {
		fprintf(stderr, "Failed to open export for writing!\n");
		return (-1);
	}

	bytes_written = snprintf(buffer, BUFFER_MAX, "%d", pin);
	write(fd, buffer, bytes_written);
	close(fd);
	return (0);
}

static int GPIOUnexport(int pin) {
	char buffer[BUFFER_MAX];
	ssize_t bytes_written;
	int fd;

	fd = open("/sys/class/gpio/unexport", O_WRONLY);
	if (-1 == fd) {
		fprintf(stderr, "Failed to open unexport for writing!\n");
		return (-1);
	}

	bytes_written = snprintf(buffer, BUFFER_MAX, "%d", pin);
	write(fd, buffer, bytes_written);
	close(fd);
	return (0);
}

static int GPIODirection(int pin, int dir) {
	static const char s_directions_str[] = "in\0out";

	char path[DIRECTION_MAX];
	int fd;

	snprintf(path, DIRECTION_MAX, "/sys/class/gpio/gpio%d/direction", pin);
	fd = open(path, O_WRONLY);
	if (-1 == fd) {
		fprintf(stderr, "Failed to open gpio direction for writing!\n");
		return (-1);
	}

	if (-1 == write(fd, &s_directions_str[IN == dir ? 0 : 3], IN == dir ? 2 : 3)) {
		fprintf(stderr, "Failed to set direction!\n");
		return (-1);
	}

	close(fd);
	return (0);
}

static int GPIORead(int pin) {
	char path[VALUE_MAX];
	char value_str[3];
	int fd;

	snprintf(path, VALUE_MAX, "/sys/class/gpio/gpio%d/value", pin);
	fd = open(path, O_RDONLY);
	if (-1 == fd) {
		fprintf(stderr, "Failed to open gpio value for reading!\n");
		return (-1);
	}
	if (-1 == read(fd, value_str, 3)) {
		fprintf(stderr, "Failed to read value!\n");
		return (-1);
	}
	close(fd);
	return (atoi(value_str));
}

static int GPIOWrite(int pin, int value) {
	static const char s_values_str[] = "01";

	char path[VALUE_MAX];
	int fd;

	snprintf(path, VALUE_MAX, "/sys/class/gpio/gpio%d/value", pin);
	fd = open(path, O_WRONLY);
	if (-1 == fd) {
		fprintf(stderr, "Failed to open gpio value for writing!\n");
		return (-1);
	}
	if (1 != write(fd, &s_values_str[LOW == value ? 0 : 1], 1)) {
		fprintf(stderr, "Failed to write value!\n");
		return (-1);
	}
	close(fd);
	return (0);
}

int main(int argc, char* argv[]) {
    int direction = 1; // 1: 시계 방향, -1: 반시계 방향
    int steps = 512; // 회전할 스텝 수
    int delay_ms = 5; // 각 스텝 사이의 딜레이(ms)

    // GPIO 핀 설정
    if (GPIOExport(IN1) == -1 || GPIOExport(IN2) == -1 ||
        GPIOExport(IN3) == -1 || GPIOExport(IN4) == -1) {
        return 1;
    }

    if (GPIODirection(IN1, OUT) == -1 || GPIODirection(IN2, OUT) == -1 ||
        GPIODirection(IN3, OUT) == -1 || GPIODirection(IN4, OUT) == -1) {
        return 2;
    }

    // 사용자 정의 회전 실행
    rotate_motor(direction, steps, delay_ms);

    // GPIO 해제
    GPIOUnexport(IN1);
    GPIOUnexport(IN2);
    GPIOUnexport(IN3);
    GPIOUnexport(IN4);

    return 0;
}
