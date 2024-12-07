#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/time.h>
#include <fcntl.h>
#include <string.h>
#include <pthread.h>
#include <wiringPiSPI.h>

// GPIO 관련 경로
#define GPIO_PATH "/sys/class/gpio"
#define F_TRIG_PIN 17
#define F_ECHO_PIN 27
#define F_IR_PIN 22
#define B_TRIG_PIN 23
#define B_ECHO_PIN 24
#define B_IR_PIN 25

// SPI 설정
#define SPI_CHANNEL 0 // SPI 채널 (0 또는 1 사용 가능)
#define SPI_SPEED 500000 // SPI 속도 (500kHz)

// 초음파 플래그
volatile int frontUltrasonicActive = 0;
volatile int backUltrasonicActive = 0;

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
float measureDistance(int trigPin, int echoPin) {
	struct timeval start, end;
	long duration;

	gpioWrite(trigPin, 0);
	usleep(2);
	gpioWrite(trigPin, 1);
	usleep(10);
	gpioWrite(trigPin, 0);

	while (gpioRead(echoPin) == 0);
	gettimeofday(&start, NULL);
	while (gpioRead(echoPin) == 1);
	gettimeofday(&end, NULL);

	duration = (end.tv_sec - start.tv_sec) * 1000000 + (end.tv_usec - start.tv_usec);
	return (float)(duration * 0.034) / 2;
}


// SPI 통신 함수
void sendSPIData(const char* data) {
	int len = strlen(data) + 1; // NULL 문자 포함
	char buffer[256];
	memset(buffer, 0, sizeof(buffer));
	strncpy(buffer, data, sizeof(buffer) - 1);

	if (wiringPiSPIDataRW(SPI_CHANNEL, (unsigned char*)buffer, len) == -1) {
		perror("SPI 데이터 전송 실패");
	}
	printf("[SPI] 전송 데이터: %s\n", buffer);
}

// 초음파 센서 쓰레드 (Front)
void* frontUltrasonicThread(void* arg) {
	while (1) {
		float distance = measureDistance(F_TRIG_PIN, F_ECHO_PIN);
		if (distance < 50.0) {
			frontUltrasonicActive = 1; // 50cm 미만이면 플래그 활성화
			sendSPIData("Front Active"); // SPI로 상태 전송
		} else {
			frontUltrasonicActive = 0; // 50cm 이상이면 플래그 비활성화
			sendSPIData("Front Inactive"); // SPI로 상태 전송
		}
		printf("[Front] 거리: %.2f cm (Flag: %d)\n", distance, frontUltrasonicActive);
		sleep(1);
	}
	return NULL;
}

// 초음파 센서 쓰레드 (Back)
void* backUltrasonicThread(void* arg) {
	while (1) {
		float distance = measureDistance(B_TRIG_PIN, B_ECHO_PIN);
		if (distance < 50.0) {
			backUltrasonicActive = 1; // 50cm 미만이면 플래그 활성화
			sendSPIData("Back Active"); // SPI로 상태 전송
		} else {
			backUltrasonicActive = 0; // 50cm 이상이면 플래그 비활성화
			sendSPIData("Back Inactive"); // SPI로 상태 전송
		}
		printf("[Back] 거리: %.2f cm (Flag: %d)\n", distance, backUltrasonicActive);
		sleep(1);
	}
	return NULL;
}

// IR 센서 쓰레드
void* irSensorThread(void* arg) {
	while (1) {
		// Front IR 센서 활성화
		if (frontUltrasonicActive) {
			int firValue = gpioRead(F_IR_PIN);
			char message[50];
			snprintf(message, sizeof(message), "Front IR: %d", firValue);
			sendSPIData(message); // SPI로 IR 상태 전송
			printf("[Front IR] 감지 상태: %d\n", firValue);
		}
		sleep(1);
	}
	return NULL;
}

int main() {
	// WiringPi SPI 초기화
	if (wiringPiSPISetup(SPI_CHANNEL, SPI_SPEED) == -1) {
		perror("SPI 초기화 실패");
		exit(1);
	}

	// GPIO 초기화 (기존 코드 유지)
	gpioExport(F_TRIG_PIN);
	gpioExport(F_ECHO_PIN);
	gpioExport(F_IR_PIN);
	gpioExport(B_TRIG_PIN);
	gpioExport(B_ECHO_PIN);
	gpioExport(B_IR_PIN);

	gpioSetDirection(F_TRIG_PIN, "out");
	gpioSetDirection(F_ECHO_PIN, "in");
	gpioSetDirection(F_IR_PIN, "in");
	gpioSetDirection(B_TRIG_PIN, "out");
	gpioSetDirection(B_ECHO_PIN, "in");
	gpioSetDirection(B_IR_PIN, "in");

	// 쓰레드 생성
	pthread_t frontThread, backThread, irThread;

	pthread_create(&frontThread, NULL, frontUltrasonicThread, NULL);
	pthread_create(&backThread, NULL, backUltrasonicThread, NULL);
	pthread_create(&irThread, NULL, irSensorThread, NULL);

	// 쓰레드 종료 대기
	pthread_join(frontThread, NULL);
	pthread_join(backThread, NULL);
	pthread_join(irThread, NULL);

	return 0;
}

