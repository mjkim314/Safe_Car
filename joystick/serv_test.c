#include "header.h"
#include "joystick.h"
#include "GPIO.h"
#include "spi.h"
#include "motor.h"
#include "hash_table.h"

#define LED_PIN 29
//#define BZ_PIN 21
#define PORT 12345
#define CLNT_SIZE 3


int clnt_count = 0; //클라이언트 수를 체크
int joy_data[3]; //조이스틱 데이터(0 : x값, 1 : y값, 2 : 1이면 버튼 off, 0이면 버튼 on) -400~+400
t_hash_table* clnt_info;

void* controller_to_car_input_joy(void* arg) { //조이스틱 값을 읽는 스레드
    int car_clnt_sock = *(int*)arg;
    char buffer[256]; 
	struct timespec delay;
	delay.tv_sec = 0;
	delay.tv_nsec = 10000000;
	int count = 0;

    while (1) {

		if (search_table(clnt_info, "CONTROL") && count % 10 == 0) { //0.1초마다 조이스틱 값 읽기(연결 체크)

			int bytes_read = read(car_clnt_sock, buffer, sizeof(buffer) - 1);

			
			if (bytes_read > 0){
				buffer[bytes_read] = '\0';


				// 조이스틱 데이터 처리
				memcpy(joy_data, buffer, sizeof(joy_data));
				for (int i = 0; i < 3; i++) {
					joy_data[i] = ntohl(joy_data[i]);
				}
				/*
				if (count <= 30) { //처음 3번의 값은 입력 x
					joy_data[0] = 0;
					joy_data[1] = 0;

				}*/

				printf("X: %d  Y: %d  B: %d\n", joy_data[0], joy_data[1], joy_data[2]);
			}
			else 
			{
				close(get_sock_by_key(clnt_info, "CONTROL"));
				remove_from_table(clnt_info, "CONTROL");

				clnt_count--;

				pthread_exit(NULL);
			}

			memset(buffer, 0, sizeof(buffer));
			snprintf(buffer, sizeof(buffer), "%s", "CONTROL");

			if (search_table(clnt_info, "SAFETY")) {
				strcat(buffer, ", SAFETY");
			}	
			if (search_table(clnt_info, "CRASH")) {
				strcat(buffer, ", CRASH");

			}

			write(car_clnt_sock, buffer, sizeof(buffer) - 1);
		}
		else { //컨트롤러가 연결이 안돼있을 때(모터 제어 x라던가 기능 적용해야 함)

		}
		memset(buffer, 0, sizeof(buffer));
		count++;
		nanosleep(&delay, NULL);
     
    }
	return NULL;
}

void* detect_safety(void* arg) { 
	char buffer[256];
	ssize_t bytes_read;
	struct timespec delay;
	delay.tv_sec = 0;
	delay.tv_nsec = 10000000;
	int count = 0;

	while (1) {
		
		if (search_table(clnt_info, "SAFETY") && count % 50 == 0) { // 0.5초마다 값 읽기
			bytes_read = read(get_sock_by_key(clnt_info, "SAFETY"), buffer, sizeof(buffer) - 1);
			
			if(bytes_read > 0) {
				//받은 값에 따라 어떤 행동을 할 지 결정
				if (strncmp(buffer, "Warning_1", sizeof("Warning_1")) == 0) {
					//1번 케이스는 모터 잠깐 멈춰서 사용자 깨우기
				}
				else if (strncmp(buffer, "Warning_2", sizeof("Warning_2")) == 0) {
					//2번 케이스는 비상등 점등, 부저 울리기, 모터 천천히 멈추기
				}

			}
			else
			{
				close(get_sock_by_key(clnt_info, "SAFETY"));
				remove_from_table(clnt_info, "SAFETY");
				
				clnt_count--;
				pthread_exit(NULL);

			}

		}
		/*
		else { //연결이 안돼있다면
			//모터 속도 제한을 설정
		}
		*/
		memset(buffer, 0, sizeof(buffer));
		count++;
		nanosleep(&delay, NULL);
	}
	return NULL;

}


void* detect_crash(void* arg) {
	char buffer[256];
	ssize_t bytes_read;
	struct timespec delay;
	delay.tv_sec = 0;
	delay.tv_nsec = 10000000;
	int count = 0;

	while (1) {

		if (search_table(clnt_info, "CRASH") && count % 10 == 0) { //0.1초마다 조이스틱 값 읽기(연결 체크)
			/*
			if ()//spi 통신 끊긴걸 감지했을 때 조건
			{
				remove_from_table(clnt_info, "CRASH");
				
				clnt_count--;


			}
			else { //정상적으로 받았을 때 읽을 값에 따라 모터를 정지할 것인지 조건문 추가할 것

			}
			*/
		}
		else {//연결이 끊겼을 때 어떤 행동을 할 지 결정
			//예시 : 충돌이 일어나지 않게 모터 최대 속도를 제한함

		}
		memset(buffer, 0, sizeof(buffer));
		count++;
		nanosleep(&delay, NULL);
	}
	return NULL;

}

void* control_motor(void* arg) {

	while (1) {
		if (search_table(clnt_info, "CONTROL") || search_table(clnt_info, "SAFETY")) {
			pthread_exit(NULL);
		}	
		
		if (search_table(clnt_info, "CRASH")) {
			//긴급정지 코드
			stopMotor();
		}

		if (joy_data[1] > 0){
			changeDutyCycle(1, joy_data[0], joy_data[1]);
		}
		else if (joy_data[1] < 0){
			changeDutyCycle(0, joy_data[0], joy_data[1]);
		}

		//정상 작동시 코드
		/*
		if (joy_data[1] > 0 && joy_data[0] == 0){
			goForward(joy_data[1] + 600);
		}
		else if (joy_data[1] < 0 && joy_data[0] == 0){
			goBackward(joy_data[1] + 600);
		}
		else if (joy_data[1] > 0 && joy_data[0] > 0){
			goSmoothRight(joy_data[1] + 600, joy_data[0]+600);
		}
		else if (joy_data[1] > 0 && joy_data[0] < 0){
			goSmoothLeft(joy_data[1] + 600, joy_data[0]+600);
		}
		else if (joy_data[1] == 0 && joy_data[0] > 0){
			turnRight(joy_data[0]+600);
		}
		else if (joy_data[1] == 0 && joy_data[0] < 0){
			turnLeft(joy_data[0]+600);
		}
		*/

	}

}

void* controller_to_car_input_btn(void* arg) { //조이스틱 버튼을 눌렀을 때 led 켜짐(다른 스레드에서 병렬처리)
	
	pinMode(LED_PIN, OUTPUT);

	while (1)
	{
		if (!search_table(clnt_info, "CONTROL")) {
			pthread_exit(NULL);
		} 
		else if (joy_data[2] == 0)
		{
			digitalWrite(LED_PIN, 1);
			usleep(100000);
			digitalWrite(LED_PIN, 0);
		}
		
	}
	return NULL;

}
/*
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
	return NULL;

}
*/

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
	clnt_info = create_table(); //클라이언트 정보를 저장하는 해시테이블 선언

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

	while (1) { //반복문으로 클라이언트 연결 계속 확인


		if (clnt_count < CLNT_SIZE) {

			char buffer[256];
			memset(buffer, 0, sizeof(buffer));

			/*
			* spi 통신으로 연결돼있는지 확인할 조건문(연결이 끊겼다가 다시 재연결할때 조건문으로 확인 후 연결시킬 필요가 있음, spi 통신 선이 연결돼있는지 확인할 수 있는 코드가 있으면 좋음)
			if (!search_table(clnt_info, "CRASH") && 여기에 연결됐다는 확인하는 조건문)
			{
				해시테이블에 입력 받은 id 추가
				add_to_table(clnt_info, "CRASH", 0);
				clnt_count++;
				
				
			}
			*/

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

				//모터 제어 스레드 추가할 것
				pthread_t controller_to_car_input_joy_thread, controller_to_car_input_btn_thread, control_motor_thread;

				pthread_create(&controller_to_car_input_joy_thread, NULL, controller_to_car_input_joy, (void*)&car_clnt_sock);
				pthread_create(&controller_to_car_input_btn_thread, NULL, controller_to_car_input_btn, (void*)&car_clnt_sock);
				pthread_create(&control_motor_thread, NULL, control_motor, NULL);

				pthread_detach(controller_to_car_input_joy_thread);
				pthread_detach(controller_to_car_input_btn_thread);
				pthread_detach(control_motor_thread);
				clnt_count++;

				


			}
			else if (strncmp(buffer, "SAFERTY", strlen("SAFETY")) == 0) {
				add_to_table(clnt_info, "SAFERTY", car_clnt_sock);
				

				/*
				운전자 이상 감지 시스템에서 작동할 스레드나 모터 제어 스레드에 어떤식으로 제어할 지 실행 코드 작성 필요
				*/

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
