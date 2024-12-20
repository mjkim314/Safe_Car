// UltraSonic & IR Sensor
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/time.h>
#include <fcntl.h>
#include <string.h>
#include <pthread.h>
#include <arpa/inet.h> 
#include "GPIO.h" 

// GPIO 관련 경로
#define GPIO_PATH "/sys/class/gpio"
#define F_TRIG_PIN 27
#define F_ECHO_PIN 17
#define F_IR_PIN 22
#define B_TRIG_PIN 23
#define B_ECHO_PIN 24
#define B_IR_PIN 25

// 초음파 플래그
volatile int frontUltrasonicActive = 0;
volatile int backUltrasonicActive = 0;

// 소켓 통신 관련 설정
#define SERVER_PORT 12345      	// 포트 번호
#define Eth_IP "169.254.157.29"	//이더넷 IP
#define CLNT_ID "CRASH"		//클라이언트 ID

// 초음파 센서 거리 계산 함수
float measureDistance(int trigPin, int echoPin) {
	struct timeval start, end;
	long duration;
	
	GPIOWrite(trigPin, 0);
	usleep(2);
	GPIOWrite(trigPin, 1);
	usleep(10);
	GPIOWrite(trigPin, 0);

	while (GPIORead(echoPin) == 0);
	gettimeofday(&start, NULL);
	while (GPIORead(echoPin) == 1);
	gettimeofday(&end, NULL);

	duration = (end.tv_sec - start.tv_sec) * 1000000 + (end.tv_usec - start.tv_usec);
	return (float)(duration * 0.034) / 2;
}

// 초음파 센서 쓰레드 (Front)
void *frontUltrasonicThread(void *arg) {
	while (1) {
		float fdistance = measureDistance(F_TRIG_PIN, F_ECHO_PIN);
		if (fdistance < 15.0) {
			frontUltrasonicActive = 1; // 50cm 미만이면 플래그 활성화
		} else {
			frontUltrasonicActive = 0; // 50cm 이상이면 플래그 비활성화
		}
		printf("[Front] 거리: %.2f cm (Flag: %d)\n", fdistance, frontUltrasonicActive);
		sleep(1);
	}
	pthread_exit(NULL);
}

// 초음파 센서 쓰레드 (Back) - 위와 동일
void *backUltrasonicThread(void *arg) {
	while (1) {
		float bdistance = measureDistance(B_TRIG_PIN, B_ECHO_PIN);
		if (bdistance < 15.0) {
			backUltrasonicActive = 1;
		} else {
			backUltrasonicActive = 0;
		}
		printf("[Back] 거리: %.2f cm (Flag: %d)\n", bdistance, backUltrasonicActive);
		sleep(1);
	}
	pthread_exit(NULL);
}


// IR 센서 쓰레드
void *irSensorThread(void *arg) {
	int sockfd = *(int*)arg;
	
	while (1) {
		// Front, Back의 초음파 센서 트리거 활성화 -> IR 센서 체크, IR 센서 감지 시 소켓 전송
		if (frontUltrasonicActive) {
			int frontIr = GPIORead(F_IR_PIN);
			if(frontIr == 1){ // front ir 센서 값이 1이면 감지 
				char message1[2];
				snprintf(message1, sizeof(message1), "F");
				write(sockfd, message1, sizeof(message1));
				printf("F : %d\n", frontIr);	
			}
		}
		
		else if (backUltrasonicActive) {
			int backIr = GPIORead(B_IR_PIN);
			if(backIr == 0){ // back ir 센서 값이 1이면 감지
				char message2[2];
				snprintf(message2, sizeof(message2), "B");
				write(sockfd, message2, sizeof(message2));
				printf("B : %d\n", backIr);
			}
		}
		else {
			char message3[2];
			snprintf(message3, sizeof(message3), "S");
			write(sockfd, message3, sizeof(message3));
			//printf("감지 X/ 연결 중\n");
		}
		sleep(1.3);
	}
	pthread_exit(NULL);
}

int main(int argc, char* argv[]) {
	// 소켓 초기화
	int sockfd;

 	struct sockaddr_in serverAddr;

	// I/O 확인
	 if (argc != 1) {
	     printf("Usage : %s\n", argv[0]);
	     exit(EXIT_FAILURE);
	 }
	
	 if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
	     perror("Socket creation failed");
	     exit(EXIT_FAILURE);
	 }
 

	serverAddr.sin_family = AF_INET;
	serverAddr.sin_addr.s_addr = inet_addr(Eth_IP);
	serverAddr.sin_port = htons(SERVER_PORT);
	

	// 서버 연결
	if (connect(sockfd, (struct sockaddr *)&serverAddr, sizeof(serverAddr)) < 0) {
		perror("서버 연결 실패");
		exit(1);
	}
	
	
	// CLNT_ID 전송
	if (write(sockfd, CLNT_ID, strlen(CLNT_ID)) < 0) {
	    perror("CLNT_ID 전송 실패");
	} else {
	    printf("CLNT_ID 전송 성공: %s\n", CLNT_ID);
	}
	// GPIO 초기화
	GPIOExport(F_TRIG_PIN);
	GPIOExport(F_ECHO_PIN);
	GPIOExport(F_IR_PIN);
	GPIOExport(B_TRIG_PIN);
	GPIOExport(B_ECHO_PIN);
	GPIOExport(B_IR_PIN);

	GPIODirection(F_TRIG_PIN, OUT);
	GPIODirection(F_ECHO_PIN, IN);
	GPIODirection(F_IR_PIN, IN);
	GPIODirection(B_TRIG_PIN, OUT);
	GPIODirection(B_ECHO_PIN, IN);
	GPIODirection(B_IR_PIN, IN);

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
