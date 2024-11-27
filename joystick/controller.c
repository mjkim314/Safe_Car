#include "spi.h"
#include "header.h"
#include "joystick.h"
#include "GPIO.h" 

#define PORT 12345

void* controller_to_car_output(void* arg) {
    int car_serv_sock = *(int*)arg;

    int joystick_fd = initJoystick();
    int count = 0;
    int joy_data[3];
    int prev_joy_data[2];
    int base_joy_data[2] = { 0, };

    int base_cnt = 0;

    float alpha = 0.2;

    struct timespec delay;
    delay.tv_sec = 0;
    delay.tv_nsec = 10000000;

    while (1) {

        if (count % 10 == 0) {

            int* temp = readController(joystick_fd);
            memcpy(joy_data, temp, sizeof(joy_data)); // 배열 데이터 복사

            
            base_cnt++;
            if (base_cnt <= 10) {  //베이스 값 정하기, 조이스틱 안움직일 때의 값을 설정
                

                base_joy_data[0] += temp[0];
                base_joy_data[1] += temp[1];

                if (base_cnt == 10) {
                    base_joy_data[0] = base_joy_data[0] / base_cnt;
                    base_joy_data[1] = base_joy_data[1] / base_cnt;

                    prev_joy_data[0] = base_joy_data[0];
                    prev_joy_data[1] = base_joy_data[1];

                }

            }
            else { //1초 후 부터 조이스틱 값 전송 가능
                //저역통과 필터 단게
                printf("before   -    X: %d  Y: %d   B: %d ||     ", joy_data[0], joy_data[1], joy_data[2]);

                joy_data[0] = (int)(alpha * joy_data[0] + (1 - alpha) * prev_joy_data[0]);
                joy_data[1] = (int)(alpha * joy_data[1] + (1 - alpha) * prev_joy_data[1]);

                prev_joy_data[0] = joy_data[0];
                prev_joy_data[1] = joy_data[1];


                joy_data[0] = joy_data[0] - base_joy_data[0];
                joy_data[1] = base_joy_data[1] - joy_data[1];

                // X, Y 값이 -100 ~ +100 범위에 있는 경우 0으로 설정
                if (joy_data[0] >= -100 && joy_data[0] <= 100) {
                    joy_data[0] = 0;
                }
                else if (joy_data[0] > 100)
                    joy_data[0] -= 100;
                else
                    joy_data[0] += 100;


                if (joy_data[1] >= -100 && joy_data[1] <= 100) {
                    joy_data[1] = 0;
                }
                else if (joy_data[1] > 100)
                    joy_data[1] -= 100;
                else
                    joy_data[1] += 100;

                if (joy_data[2])
                    joy_data[2] = 1;

                printf("after   -    X: %d  Y: %d  B : %d\n", joy_data[0], joy_data[1], joy_data[2]);

                for (int i = 0; i < 3; i++) {
                    joy_data[i] = htonl((int)joy_data[i]);
                }

                write(car_serv_sock, joy_data, sizeof(joy_data));


            }
            
        }
        count++;
        nanosleep(&delay, NULL);
    }
    return NULL;
}


int main(int argc, char* argv[]) {
    int car_serv_sock;

    struct sockaddr_in car_serv_addr;

    if (argc != 2) {
        printf("Usage : %s <IP>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    if ((car_serv_sock = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
        perror("Socket creation failed");
        exit(EXIT_FAILURE);
    }

    memset(&car_serv_addr, 0, sizeof(car_serv_addr));
    car_serv_addr.sin_family = AF_INET;
    car_serv_addr.sin_addr.s_addr = inet_addr(argv[1]);
    car_serv_addr.sin_port = htons(PORT);

    if (connect(car_serv_sock, (struct sockaddr*)&car_serv_addr, sizeof(car_serv_addr)) < 0) {
        perror("Connection failed");
        exit(EXIT_FAILURE);
    }

    pthread_t controller_to_car_output_thread;
    
        
    pthread_create(&controller_to_car_output_thread, NULL, controller_to_car_output, (void*)&car_serv_sock);


    pthread_join(controller_to_car_output_thread, NULL);
    close(car_serv_sock);
    return 0;
}
