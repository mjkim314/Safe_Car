#include "header.h"
#include "joystick.h"
#include "GPIO.h"
#include "spi.h"
#include "motor.h"
#include "hash_table.h"
//#include "lcd.h"


#define LED_PIN 29
//#define BZ_PIN 21
#define PORT 12345
#define CLNT_SIZE 3
int motor_control = 0;

int clnt_count = 0; //클라이언트 수를 체크
int joy_data[3] = {0, 0, 1}; //조이스틱 데이터(0 : x값, 1 : y값, 2 : 1이면 버튼 off, 0이면 버튼 on) -400~+400
t_hash_table* clnt_info;
int prey = 2;

void* controller_to_car_input_joy(void* arg) { //조이스틱 값을 읽는 스레드
    int car_clnt_sock = *(int*)arg;
    char buffer[256]; 
	struct timespec delay;
	delay.tv_sec = 0;
	delay.tv_nsec = 10000000;
	int count = 0;

    while (1) {	

		if (search_table(clnt_info, "CONTROL") && count % 5 == 0) { //0.1초마다 조이스틱 값 읽기(연결 체크)

			int bytes_read = read(car_clnt_sock, buffer, sizeof(buffer) - 1);

			
			if (bytes_read > 0){
				buffer[bytes_read] = '\0';


				// 조이스틱 데이터 처리
				memcpy(joy_data, buffer, sizeof(joy_data));
				for (int i = 0; i < 3; i++) {
					joy_data[i] = ntohl(joy_data[i]);
				}

				//printf("X: %d  Y: %d  B: %d\n", joy_data[0], joy_data[1], joy_data[2]);
			}
			else 
			{
				close(get_sock_by_key(clnt_info, "CONTROL"));
				remove_from_table(clnt_info, "CONTROL");

				print_clients(clnt_info);
				clnt_count--;

				pthread_exit(NULL);
			}
			

		}
		else { //컨트롤러가 연결이 안돼있을 때(모터 제어 x라던가 기능 적용해야 함)

		}
		memset(buffer, 0, sizeof(buffer));
		count++;
		nanosleep(&delay, NULL);
     
    }
	return NULL;
}

void* car_to_controller_lcd(void* arg) {
	int car_clnt_sock = *(int*)arg;
	char buffer[36];
	char prev_buffer[36];
	struct timespec delay;
	delay.tv_sec = 0;
	delay.tv_nsec = 10000000;
	int count = 0;

	while (1) {
		if (search_table(clnt_info, "CONTROL") && count % 150 == 0) {
			memset(buffer, 0, sizeof(buffer));
			snprintf(buffer, sizeof(buffer), "CONTROL");

			if (search_table(clnt_info, "SAFETY")) {
				strcat(buffer, ", SAFETY");
			}
			if (search_table(clnt_info, "CRASH")) {
				strcat(buffer, ", CRASH");

			}

			if (strcmp(prev_buffer, buffer) != 0) {
				write(car_clnt_sock, buffer, sizeof(buffer));

			}
		}
		else if (!search_table(clnt_info, "CONTROL")) {
			pthread_exit(NULL);
		}
		
		strcpy(prev_buffer, buffer);
		count++;
		nanosleep(&delay, NULL);

			
	}

}

void* detect_safety(void* arg) { 
	char buffer[256];
	ssize_t bytes_read;
	struct timespec delay;
	delay.tv_sec = 0;
	delay.tv_nsec = 10000000;
	int count = 0;

	while (1) {
		
		if (search_table(clnt_info, "SAFETY") && count % 30 == 0) { // 0.5초마다 값 읽기
			bytes_read = read(get_sock_by_key(clnt_info, "SAFETY"), buffer, sizeof(buffer) - 1);
			
			if(bytes_read > 0) {

				//받은 값에 따라 어떤 행동을 할 지 결정
				if (strncmp(buffer, "Warning_1", sizeof("Warning_1")) == 0) {
					//1번 케이스는 모터 잠깐 멈춰서 사용자 깨우기
					motor_control = 1;

				}
				else if (strncmp(buffer, "Warning_2", sizeof("Warning_2")) == 0) {
					//2번 케이스는 비상등 점등, 부저 울리기, 모터 천천히 멈추기
					//부저, led 기능은 led 제어 스레드, 부저 스레드에서 각각 처리해야됨(여기서 처리x)
					//LED 제어는 코드 추가함
					motor_control = 2;

				}

			}
			else
			{
				close(get_sock_by_key(clnt_info, "SAFETY"));
				remove_from_table(clnt_info, "SAFETY");
				
				print_clients(clnt_info);

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
/*
void* detect_crash(void* arg) {
	
	struct timespec delay;
	delay.tv_sec = 0;
	delay.tv_nsec = 10000000;
	int count = 0;

	while (1) {
		if (count % 10 == 0) {
			front = unitspiTransfer(CMD_REQUEST_FRONT_IR);
			backward = spiTransfer(CMD_REQUEST_BACK_IR);

			if (front == 0xFF && backward == 0xFF) {
				if (search_table(clnt_info, "CRASH")) 
				{
					remove_from_table(clnt_info, "CRASH");
					print_clients(clnt_info);
					clnt_count--;
				}
			}
			else{
				if (!search_table(clnt_info, "CRASH")) {
					add_to_table(clnt_info, "CRASH");
					print_clients(clnt_info);
					clnt_count++;
				}

				if (front != 0xFF) {

				}
				else if (backward != 0xFF) {

				}
			}
		}
		
		
		memset(buffer, 0, sizeof(buffer));
		count++;
		nanosleep(&delay, NULL);
	}
	return NULL;

}
*/

void* control_motor(void* arg) {

	while (1) {
		//changeDutyCycle(joy_data[0], joy_data[1], prey);
		//prey = joy_data[1];
		if (!search_table(clnt_info, "CONTROL") || !search_table(clnt_info, "SAFETY")) {
			//모터 구동 필요 없음, 코드 없어도 됨
			continue;
		}	
		else if (!search_table(clnt_info, "CRASH")) {
			//CONTROL, SAFETY 클라이언트는 있는데 CRASH 클라이언트가 없을 경우, 최대 속도 제한 (정지아님)
			if(joy_data[0] > 0){
				joy_data[0] -= 200;
			}
			else if(joy_data[0] < 0){
				joy_data[0] += 200;
			}
			else if(joy_data[1] > 0){
				joy_data[1] -= 200;
			}
			else if(joy_data[1] < 0){
				joy_data[0] += 200;
			}
		}
		//모든 클라이언트 연결했을 때 정상구동
			/*
		control_motor = 0은 평시 상황임
		control_motor = 1 일때, 0.2초 정도로 속도를 75%정도로 줄이기 (3번 반복)
		control_motor = 2 일때, 속도를 0까지 천천히 줄이기, 값은 마음대로
		(이건 control_motor 1보다 우선순위가 높음, control=1일 때 2로 바뀌면, 속도 75%로 줄이는 건 무시된다는 뜻임)
		*/

		//정상 작동시 코드
			if(motor_control == 1){
				for(int i = 3; i > 0; i--){
					if(motor_control == 2){
						slowStop(joy_data[1]);
					}
					emerBrake(joy_data[1]);
				}
				sleep(1);
				motor_control = 0;
			}
			else if(motor_control == 2){
				slowStop(joy_data[1]);
				motor_control = 0;
			}
			changeDutyCycle(joy_data[0], joy_data[1], prey);
			printf("Thread ### X:%d ### Y:%d ###\n",joy_data[0], joy_data[1] );
			prey = joy_data[1];
			sleep(1);
	}
	return NULL;
}


void* controller_to_car_input_btn(void* arg) { //조이스틱 버튼을 눌렀을 때 led 켜짐(다른 스레드에서 병렬처리)
	
	pinMode(LED_PIN, OUTPUT);

	while (1)
	{	
		if (motor_control != 2) {
			if (search_table(clnt_info, "CONTROL") && joy_data[2] == 0) {
				digitalWrite(LED_PIN, 1);
				usleep(100000);
				digitalWrite(LED_PIN, 0);
			}
		}
		else { //위험 감지 파이가 위험신호를 보낼 때(control_motor==2일 때)
			digitalWrite(LED_PIN, 1);
			usleep(10000);
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
//	if (wiringPiSPISetup(SPI_CHANNEL, SPI_SPEED) == -1) {
//		perror("SPI 초기화 실패");
//		return 1;
//	}

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

	/*
	pthread_t detect_crash_thread;
	pthread_create(&detect_crash_thread, NULL, detect_crash, NULL);
	pthread_detach(detect_crash_thread); //만약 연결 안돼있으면 이 스레드에서 해시테이블에 추가
	*/


	pthread_t controller_to_car_input_btn_thread, control_motor_thread;
	pthread_create(&controller_to_car_input_btn_thread, NULL, controller_to_car_input_btn, NULL);
	pthread_create(&control_motor_thread, NULL, control_motor, NULL);

	pthread_detach(controller_to_car_input_btn_thread);
	pthread_detach(control_motor_thread);

	while (1) { //반복문으로 클라이언트 연결 계속 확인


		if (clnt_count < CLNT_SIZE) {

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

				pthread_t controller_to_car_input_joy_thread, car_to_controller_lcd_thread;

				pthread_create(&controller_to_car_input_joy_thread, NULL, controller_to_car_input_joy, (void*)&car_clnt_sock);
				pthread_create(&car_to_controller_lcd_thread, NULL, car_to_controller_lcd, (void*)&car_clnt_sock);

				pthread_detach(controller_to_car_input_joy_thread);
				pthread_detach(car_to_controller_lcd_thread);

				clnt_count++;
				print_clients(clnt_info);

				


			}
			else if (strncmp(buffer, "SAFETY", strlen("SAFETY")) == 0) {

				add_to_table(clnt_info, "SAFETY", car_clnt_sock);
				pthread_t detect_safety_thread;
				pthread_create(&detect_safety_thread, NULL, detect_safety, (void*)&car_clnt_sock);
				pthread_detach(detect_safety_thread);
				
				clnt_count++;
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
