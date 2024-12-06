// UltraSonic & IR Sensor
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/time.h>
#include <fcntl.h>
#include <string.h>
#include <pthread.h>
#include <arpa/inet.h> 

// GPIO 관련 경로
#define GPIO_PATH "/sys/class/gpio"
#define F_TRIG_PIN 17
#define F_ECHO_PIN 27
#define F_IR_PIN 22
#define B_TRIG_PIN 23
#define B_ECHO_PIN 24
#define B_IR_PIN 25

// 초음파 플래그
volatile int frontUltrasonicActive = 0;
volatile int backUltrasonicActive = 0;

// 소켓 통신 관련 설정
#define SERVER_IP "127.0.0.1" // 로컬 서버 IP
#define SERVER_PORT 8080      // 포트 번호
int sockfd;                   // 소켓 파일 디스크립터



// 소켓 데이터 전송 함수
void sendSocketData(const char *data) {
	if (send(sockfd, data, strlen(data), 0) == -1) {
		perror("소켓 데이터 전송 실패");
	}
	printf("[Socket] 전송 데이터: %s\n", data);
}



// 초음파 센서 쓰레드 (Front)
void *frontUltrasonicThread(void *arg) {
	while (1) {
		float distance = measureDistance(F_TRIG_PIN, F_ECHO_PIN);
		if (distance < 50.0) {
			frontUltrasonicActive = 1; // 50cm 미만이면 플래그 활성화
		} else {
			frontUltrasonicActive = 0; // 50cm 이상이면 플래그 비활성화
		}
		printf("[Front] 거리: %.2f cm (Flag: %d)\n", distance, frontUltrasonicActive);
		sleep(1);
	}
	return NULL;
}

// 초음파 센서 쓰레드 (Back)
void *backUltrasonicThread(void *arg) {
	while (1) {
		float distance = measureDistance(B_TRIG_PIN, B_ECHO_PIN);
		if (distance < 50.0) {
			backUltrasonicActive = 1;
		} else {
			backUltrasonicActive = 0;
		}
		printf("[Back] 거리: %.2f cm (Flag: %d)\n", distance, backUltrasonicActive);
		sleep(1);
	}
	return NULL;
}

// IR 센서 쓰레드
void *irSensorThread(void *arg) {
	while (1) {
		if (frontUltrasonicActive) {
			char message1[2];
			snprintf(message1, sizeof(message1), "F");
			sendSocketData(message1);
			printf("[Front IR] 감지 상태: %d\n", frontUltrasonicActive);
		}
		if (backUltrasonicActive) {
			char message2[2];
			snprintf(message2, sizeof(message2), "B");
			sendSocketData(message2);
			printf("[Back IR] 감지 상태: %d\n", backUltrasonicActive);
		}
		else {
			char massage3[2];
			snprintf(message3, sizeof(message3), "S");
			sendSocketData(message3);
			printf("감지 X/ 연결 중\n")
		}
		sleep(1);
	}
	return NULL;
}

int main() {
	// 소켓 초기화
	struct sockaddr_in serverAddr;

	if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
		perror("소켓 생성 실패");
		exit(1);
	}

	serverAddr.sin_family = AF_INET;
	serverAddr.sin_port = htons(SERVER_PORT);

	// 서버 주소 설정
	if (inet_pton(AF_INET, SERVER_IP, &serverAddr.sin_addr) <= 0) {
		perror("유효하지 않은 주소");
		exit(1);
	}

	// 서버 연결
	if (connect(sockfd, (struct sockaddr *)&serverAddr, sizeof(serverAddr)) < 0) {
		perror("서버 연결 실패");
		exit(1);
	}

	printf("서버에 연결됨: %s:%d\n", SERVER_IP, SERVER_PORT);

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
	pthread_create(&irThread, NULL, irSensorThread, (void*)&sockfd);

	// 쓰레드 종료 대기
	pthread_join(frontThread, NULL);
	pthread_join(backThread, NULL);
	pthread_join(irThread, NULL);

	// 소켓 종료
	close(sockfd);
	printf("소켓 연결 종료\n");

	return 0;
}
