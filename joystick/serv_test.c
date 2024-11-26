#include "header.h"
#include "joystick.h"
#include "GPIO.h"
#include "spi.h"

void* controller_to_car_input(void* arg){
	int car_clnt_sock = *(int *)arg;
	while (1)
	{
		int joy_data[3];
		read(car_clnt_sock, joy_data, sizeof(joy_data));
		for(int i=0; i<3; i++)
			joy_data[i] = ntohl(joy_data[i]);
		printf("X : %d  Y : %d  B : %d \n", joy_data[1], joy_data[2], joy_data[0]);
	}
}
int main(int argc, char *argv[])
{

    if (argc != 2)
    {
        printf("Usage : %s <port>\n", argv[0]);
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
	car_serv_addr.sin_port = htons(atoi(argv[1]));

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
	
	pthread_t controller_to_car_input_thread;
	pthread_create(&controller_to_car_input_thread, NULL, controller_to_car_input, (void*)&car_clnt_sock);
	pthread_join(controller_to_car_input_thread, NULL);
	close(car_clnt_sock);
	close(car_serv_sock);
	return 0;
}
