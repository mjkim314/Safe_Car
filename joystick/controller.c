#include "spi.h"
#include "header.h"
#include "joystick.h"
#include "GPIO.h" 

int joystick_fd; 
int joy_data[3];

void *controller_to_car_output(void *arg){
	int car_serv_sock = *(int*)arg;
	joystick_fd = initJoystick();
	int count = 0;
	
	struct timespec delay;
	delay.tv_sec = 0;
	delay.tv_nsec = 10000000;
	
	while(1){
		if (count % 10 == 0) {
			int* temp = readController(joystick_fd); // 정적 배열 포인터 반환
		memcpy(joy_data, temp, sizeof(joy_data)); // 배열 데이터 복사
		for (int i = 0; i < 3; i++) {
			joy_data[i] = htonl(joy_data[i]); // 네트워크 바이트 오더 변환
		}
		write(car_serv_sock, joy_data, sizeof(joy_data)); // 데이터 전송
		}
		
		count++;
		nanosleep(&delay, NULL);

			
	}
 
}

int main(int argc, char *argv[])
{	 
	int control_clnt_sock;
	struct sockaddr_in car_serv_addr;
	
	if (argc != 3)
	{
		printf("Usage : %s <IP> <port>\n", argv[0]);
		exit(EXIT_FAILURE);
	}
	
	if ((control_clnt_sock = socket(AF_INET, SOCK_STREAM, 0)) == 0)
	{
		error_handling("Socket creation failed");
		exit(EXIT_FAILURE);
	}

	memset(&car_serv_addr, 0, sizeof(car_serv_addr));
	car_serv_addr.sin_family = AF_INET;
	car_serv_addr.sin_addr.s_addr = inet_addr(argv[1]);
	car_serv_addr.sin_port = htons(atoi(argv[2]));

	if (connect(control_clnt_sock, (struct sockaddr *)&car_serv_addr, sizeof(car_serv_addr)) < 0)
	{
		error_handling("Connection failed");
		exit(EXIT_FAILURE);
	}
	
	pthread_t controller_to_car_output_thread;
	pthread_create(&controller_to_car_output_thread, NULL, controller_to_car_output, (void *)&control_clnt_sock);
	pthread_join(controller_to_car_output_thread, NULL);
	close(control_clnt_sock);
	return 0;
}
