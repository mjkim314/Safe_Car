#include "header.h"
#include "joystick.h"
#include "GPIO.h"
#include "spi.h"
#include "hash_table.h"

#define LED_PIN 29
#define BZ_PIN 21
#define PORT 12345
#define CLNT_SIZE 3


int bt_flag = 0; //0이면 비상등 off, 1이면 비상등 on
int clnt_count = 0; //클라이언트 수를 체크
int joy_data[3]; //조이스틱 데이터(0 : x값, 1 : y값, 2 : 1이면 버튼 off, 0이면 버튼 on)

t_hash_table* clnt_info; //클라이언트 정보를 저장하는 해시테이블 선언

void* check_clnt(void* arg) { //나중에 체크 주기를 설정해야됨. 계속 값을 받으면 자원 너무 많이쓰게됨(일단 조이스틱 - 1초, 안전 감지 시스템 - 5초로 설정)
	char buffer[256];
	ssize_t bytes_read;
	struct timespec delay;
	delay.tv_sec = 0;
	delay.tv_nsec = 10000000;
	int count = 0;

	while (1) {
		if (search_table(clnt_info, "CONTROL") && count % 10 == 0) {
			bytes_read = read(get_sock_by_key(clnt_info, "CONTROL"), buffer, sizeof(buffer) - 1);
			if (bytes_read <= 0)
			{
				close(get_sock_by_key(clnt_info, "CONTROL"));
				remove_from_table(clnt_info, "CONTROL");
				printf("CONTROL client is disconnected\n");
				printf("Number of clients : %d\n", --clnt_count);
				print_clients(clnt_info);
			}
		}
		else if (search_table(clnt_info, "SAFETY") && count % 50 == 0) { 
			bytes_read = read(get_sock_by_key(clnt_info, "SAFETY"), buffer, sizeof(buffer) - 1);
			if (bytes_read <= 0)
			{
				close(get_sock_by_key(clnt_info, "SAFETY"));
				remove_from_table(clnt_info, "SAFETY");
				printf("SAFETY client is disconnected\n");
				printf("Number of clients : %d\n", --clnt_count);
				print_clients(clnt_info);
			}

			
			
		}
		else if (search_table(clnt_info, "CRASH")) { 
			//여긴 SPI 통신이라 다른 코드로 확인해야 함
			remove_from_table(clnt_info, "CRASH");
			printf("CRASH client is disconnected\n");
			printf("Number of clients : %d\n", --clnt_count);
			print_clients(clnt_info);
		}
		count++;
		nanosleep(&delay, NULL);
	}
	

}

void* controller_to_car_input_joy(void* arg) { //조이스틱 값을 읽는 스레드
    int car_clnt_sock = *(int*)arg;
    char buffer[256]; 
	struct timespec delay;
	delay.tv_sec = 0;
	delay.tv_nsec = 10000000;
	int count = 0;

    while (1) {
		if (count % 10 == 0) {

			int bytes_read = read(car_clnt_sock, buffer, sizeof(buffer) - 1);

			if (bytes_read > 0) {
				buffer[bytes_read] = '\0';


				// 조이스틱 데이터 처리
				memcpy(joy_data, buffer, sizeof(joy_data));
				for (int i = 0; i < 3; i++) {
					joy_data[i] = ntohl(joy_data[i]);
				}

				if (joy_data[2] == 0)
					bt_flag = 1;

				//printf("X: %d  Y: %d  B: %d   %d\n", joy_data[0], joy_data[1], joy_data[2], bt_flag);
			}
		}
		count++;
		nanosleep(&delay, NULL);
     
    }
}

void* controller_to_car_input_btn(void* arg) { //조이스틱 버튼을 눌렀을 때 led 켜짐(다른 스레드에서 병렬처리)
	
	pinMode(LED_PIN, OUTPUT);

	while (1)
	{
		if (bt_flag) {
			digitalWrite(LED_PIN, 1);
			usleep(100000);
			digitalWrite(LED_PIN, 0);
			bt_flag = 0;
		}
		
	}
}

void* controller_to_car_input_buzz(void* arg) { //버튼 눌렀을 때 부저 울림(나중에 피에조 부저로 대체)
	
	pinMode(BZ_PIN, OUTPUT);

	while (1)
	{
		if (bt_flag) {
			digitalWrite(BZ_PIN, 1);
			usleep(100000);
			digitalWrite(BZ_PIN, 0);
			bt_flag = 0;
		}

	}
}

int main(int argc, char *argv[])
{

    if (argc != 1)
    {
        printf("Usage : %s\n", argv[0]);
        exit(EXIT_FAILURE);
    }

	if (wiringPiSetup() == -1) {
		printf("WiringPi setup failed!\n");
		return -1;
	}

    int car_serv_sock, car_clnt_sock;
	struct sockaddr_in car_serv_addr;
	struct sockaddr_in car_clnt_addr;
    socklen_t car_clnt_addr_size = sizeof(car_clnt_addr);
	clnt_info = create_table();

	if ((car_serv_sock = socket(AF_INET, SOCK_STREAM, 0)) == 0)
	{	
		error_handling("RC Socket creation failed");
		exit(EXIT_FAILURE);
	}

	memset(&car_serv_addr, 0, sizeof(car_serv_addr));
	car_serv_addr.sin_family = AF_INET;
	car_serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
	car_serv_addr.sin_port = htons(PORT);

	if (bind(car_serv_sock, (struct sockaddr *)&car_serv_addr, sizeof(car_serv_addr)) < 0)
	{
		error_handling("RC Bind failed");
		exit(EXIT_FAILURE);
	}

	if (listen(car_serv_sock, 3) < 0)
	{
		error_handling("Listen failed");
		exit(EXIT_FAILURE);
	}

	//클라이언트 연결이 정상적인지 체크하는 스레드 생성
	pthread_t check_clnt_thread;
	pthread_create(&check_clnt_thread, NULL, check_clnt, NULL);
	pthread_detach(check_clnt_thread);


	while (1) { //반복문으로 클라이언트 연결 계속 확인
		if (clnt_count < CLNT_SIZE - 1) { //1대는 spi 유선통신이므로 1을 뺌

			char buffer[256];
			memset(buffer, 0, sizeof(buffer));

			if ((car_clnt_sock = accept(car_serv_sock, (struct sockaddr*)&car_clnt_addr, &car_clnt_addr_size)) < 0)
			{
				error_handling("Accept rc socket failed");
				exit(EXIT_FAILURE);
			}
			ssize_t bytes_read = read(car_clnt_sock, buffer, sizeof(buffer) - 1);

			if (bytes_read > 0) {
				buffer[bytes_read] = '\0';
			}
			else if (bytes_read == 0) {
				printf("Connection closed\n");
				close(car_clnt_sock);
			}
			else {
				perror("Read failed\n");
			}


			if (strncmp(buffer, "CONTROL", strlen("CONTROL")) == 0) {


				add_to_table(clnt_info, "CONTROL", car_clnt_sock);

				//모터 제어 스레드 필요함
				pthread_t controller_to_car_input_joy_thread, controller_to_car_input_btn_thread;

				pthread_create(&controller_to_car_input_joy_thread, NULL, controller_to_car_input_joy, (void*)&car_clnt_sock);
				pthread_create(&controller_to_car_input_btn_thread, NULL, controller_to_car_input_btn, (void*)&car_clnt_sock);

				pthread_detach(controller_to_car_input_joy_thread);
				pthread_detach(controller_to_car_input_btn_thread);
				clnt_count++;

				printf("CONTROL client connected\n");
				printf("Number of clients : %d\n", clnt_count);
				print_clients(clnt_info);


			}
			else if (strncmp(buffer, "SAFERTY", strlen("SAFETY")) == 0) {
				add_to_table(clnt_info, "SAFERTY", car_clnt_sock);


				/*
				운전자 이상 감지 시스템에서 작동할 스레드나 모터 제어 스레드에 어떤식으로 제어할 지 실행 코드 작성 필요
				*/


				clnt_count++;
				printf("SAFERTY client connected\n");
				printf("Number of clients : %d\n", clnt_count);
				print_clients(clnt_info);
			}
			else {
				printf("Client id is not valid\n");
				close(car_clnt_sock);
			}

		}
	
	}
	
	free_table(clnt_info);
	close(car_serv_sock);
	return 0;
}
