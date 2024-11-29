#include <fcntl.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <time.h>
#include <unistd.h>

#define BUFFER_MAX 3
#define DIRECTION_MAX 256
#define VALUE_MAX 256

#define IN 0
#define OUT 1
#define LOW 0
#define HIGH 1

#define POUT 23 // 초음파 트리거 핀
#define PIN 24  // 초음파 에코 핀
#define LED 18  // PWM 핀 (LED)

static pthread_mutex_t lock;
double distance = 0.0;

static int GPIOExport(int pin) {
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
	const char s_directions_str[] = "in\0out";

	char path[DIRECTION_MAX];
	int fd;

	snprintf(path, DIRECTION_MAX, "/sys/class/gpio/gpio%d/direction", pin);
	fd = open(path, O_WRONLY);
	if (-1 == fd) {
		fprintf(stderr, "Failed to open gpio direction for writing!\n");
		return (-1);
	}

	if (-1 == write(fd, &s_directions_str[dir == IN ? 0 : 3], dir == IN ? 2 : 3)) {
		fprintf(stderr, "Failed to set direction!\n");
		return (-1);
	}

	close(fd);
	return (0);
}

// PWM 설정 함수
static int SetPWMValue(int pin, int value) {
	char path[VALUE_MAX];
	char buffer[BUFFER_MAX];
	int fd;

	snprintf(path, VALUE_MAX, "/sys/class/pwm/pwmchip0/pwm%d/duty_cycle", pin);
	fd = open(path, O_WRONLY);
	if (fd == -1) {
		fprintf(stderr, "Failed to open PWM duty cycle for writing!\n");
		return -1;
	}

	snprintf(buffer, BUFFER_MAX, "%d", value);
	write(fd, buffer, strlen(buffer));
	close(fd);

	return 0;
}

void *sensor_thread(void *arg) {
	clock_t start_t, end_t;
	double time;

	while (1) {
		// Trigger 초음파 센서
		GPIOWrite(POUT, 1);
		usleep(10);
		GPIOWrite(POUT, 0);

		// Echo 핀 상태 확인
		while (GPIORead(PIN) == 0) {
			start_t = clock();
		}
		while (GPIORead(PIN) == 1) {
			end_t = clock();
		}

		// 거리 계산
		time = (double)(end_t - start_t) / CLOCKS_PER_SEC;
		pthread_mutex_lock(&lock);
		distance = (time / 2.0) * 34000.0; // 단위: cm
		pthread_mutex_unlock(&lock);

		usleep(500000); // 0.5초 대기
	}
}

void *led_thread(void *arg) {
	while (1) {
		pthread_mutex_lock(&lock);
		double current_distance = distance;
		pthread_mutex_unlock(&lock);

		// 거리와 밝기 비례 관계 설정
		// 0cm에서 가장 밝고, 100cm 이상에서는 LED가 꺼짐
		int brightness = (current_distance < 100) ? (int)(100 - current_distance) * 1000 : 0;
		SetPWMValue(LED, brightness);

		usleep(100000); // LED 상태 업데이트 주기
	}
}

int main() {
	// GPIO 설정
	if (GPIOExport(POUT) || GPIOExport(PIN) || GPIOExport(LED)) {
		printf("GPIO export error\n");
		return 1;
	}
	usleep(100000);

	if (GPIODirection(POUT, OUT) || GPIODirection(PIN, IN) || GPIODirection(LED, OUT)) {
		printf("GPIO direction error\n");
		return 1;
	}

	// PWM 초기화
	SetPWMValue(LED, 0);

	// 쓰레드 초기화
	pthread_t sensor_tid, led_tid;
	pthread_mutex_init(&lock, NULL);

	pthread_create(&sensor_tid, NULL, sensor_thread, NULL);
	pthread_create(&led_tid, NULL, led_thread, NULL);

	pthread_join(sensor_tid, NULL);
	pthread_join(led_tid, NULL);

	// GPIO 해제
	GPIOUnexport(POUT);
	GPIOUnexport(PIN);
	GPIOUnexport(LED);

	pthread_mutex_destroy(&lock);

	return 0;
}
