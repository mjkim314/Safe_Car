#include "header.h"
#include "joystick.h"
#include "GPIO.h"
#include "spi.h"
#include "motor.h"

#define LED_PIN 29
#define BZ_PIN 21
#define PORT 12345

int bt_flag = 0; //0이면 비상등 off, 1이면 비상등 on

void* controller_to_car_input_joy(void* arg) {
    int car_clnt_sock = *(int*)arg;
    char buffer[256]; // 메시지 버퍼
	int joy_data[3];

    while (1) {

        int bytes_read = read(car_clnt_sock, buffer, sizeof(buffer) - 1);
        if (bytes_read > 0) {
            buffer[bytes_read] = '\0'; 
			

            // 조이스틱 데이터 처리
            memcpy(joy_data, buffer, sizeof(joy_data));
            for (int i = 0; i < 3; i++) {
                joy_data[i] = ntohl(joy_data[i]);
            }
            
            if (joy_data[2] == 0){
                bt_flag = 1;
            }

            //printf("X: %d  Y: %d  B: %d   %d\n", joy_data[0], joy_data[1], joy_data[2], bt_flag);
			//Joystick Value -512 ~ 512 ?
			if (joy_data[1] > 0 && joy_data[0] == 0){
				goForward(joy_data[1] + 500);
			}
			else if (joy_data[1] < 0 && joy_data[0] == 0){
				goBackward(joy_data[1] + 500);
			}
			else if (joy_data[1] > 0 && joy_data[0] > 0){
				goSmoothRight(joy_data[1] + 500, joy_data[0]+500);
			}
			else if (joy_data[1] > 0 && joy_data[0] < 0){
				goSmoothLeft(joy_data[1] + 500, joy_data[0]+500);
			}
			else if (joy_data[1] == 0 && joy_data[0] > 0){
				turnRight(joy_data[0]+500);
			}
			else if (joy_data[1] == 0 && joy_data[0] < 0){
				turnLeft(joy_data[0]+500);
			}

        }
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
    int car_serv_sock, car_clnt_sock;
	struct sockaddr_in car_serv_addr;
	struct sockaddr_in car_clnt_addr;
    socklen_t car_clnt_addr_size = sizeof(car_clnt_addr);

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

	if ((car_clnt_sock = accept(car_serv_sock, (struct sockaddr *)&car_clnt_addr, &car_clnt_addr_size)) < 0)
	{
		error_handling("Accept rc socket failed");
		exit(EXIT_FAILURE);
	}

	if (wiringPiSetup() == -1) {
		printf("WiringPi setup failed!\n");
		return -1;
	}

	initMotor();
	
	pthread_t controller_to_car_input_joy_thread, controller_to_car_input_btn_thread, controller_to_car_input_buzz_thread;

	pthread_create(&controller_to_car_input_joy_thread, NULL, controller_to_car_input_joy, (void*)&car_clnt_sock);
	pthread_create(&controller_to_car_input_btn_thread, NULL, controller_to_car_input_btn, (void*)&car_clnt_sock);
	pthread_create(&controller_to_car_input_buzz_thread, NULL, controller_to_car_input_buzz, (void*)&car_clnt_sock);

	pthread_join(controller_to_car_input_joy_thread, NULL);
	pthread_join(controller_to_car_input_btn_thread, NULL);
	pthread_join(controller_to_car_input_buzz_thread, NULL);

	close(car_clnt_sock);
	close(car_serv_sock);
	return 0;
}
