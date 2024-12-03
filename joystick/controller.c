#include "spi.h"
#include "header.h"
#include "joystick.h"
#include "GPIO.h"
#include "lcd.h"

#define PORT 12345
#define CLNT_ID "CONTROL"

void* controller_to_car_output(void* arg) {
    int car_serv_sock = *(int*)arg;

    int joystick_fd = initJoystick();
    int count = 0;
    int joy_data[3];
    int prev_joy_data[2];
    int base_joy_data[2] = {500, 500};

    int base_cnt = 0;

    float alpha = 0.4;

    struct timespec delay;
    delay.tv_sec = 0;
    delay.tv_nsec = 10000000;

    while (1) {

        if (count % 1 == 0) {

            int* temp = readController(joystick_fd);
            memcpy(joy_data, temp, sizeof(joy_data)); // 배열 데이터 복사

            

            { //1초 후 부터 조이스틱 값 전송 가능
                //저역통과 필터 단게
                //printf("before   -    X: %d  Y: %d   B: %d ||     ", joy_data[0], joy_data[1], joy_data[2]);

                joy_data[0] = (int)(alpha * joy_data[0] + (1 - alpha) * prev_joy_data[0]);
                joy_data[1] = (int)(alpha * joy_data[1] + (1 - alpha) * prev_joy_data[1]);
                prev_joy_data[0] = joy_data[0];
                prev_joy_data[1] = joy_data[1];


                if (joy_data[0] > base_joy_data[0]) {
                    if (joy_data[0] == 0)
                        joy_data[0] -= abs(base_joy_data[0] - 512);
                }

                if (joy_data[1] > base_joy_data[1]) {
                    if (joy_data[1] == 0)
                        joy_data[1] -= abs(base_joy_data[1] - 512);
                }


                //0, 0을 조이스틱의 중앙으로 설정
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

                //버튼이 안눌리면 1, 눌리면 0으로 전송
                if (joy_data[2] < 50)
                    joy_data[2] = 0;
                else
                    joy_data[2] = 1;

                //printf("after   -    X: %d  Y: %d  B : %d\n", joy_data[0], joy_data[1], joy_data[2]);


                if (count < 300)
                {
                    joy_data[0] = 0;
                    joy_data[1] = 0;
                }
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


void* check_clnt_lcd(void* arg) {
    int serv_sock = *(int*)arg;
    char buffer[36];
    char prev_buffer[36];
    int count = 0;
    struct timespec delay;
    delay.tv_sec = 0;
    delay.tv_nsec = 10000000;

    while (1) {


        if (count % 50 == 0) {
            ssize_t bytes_read = read(serv_sock, buffer, sizeof(buffer));

            if (bytes_read > 0) {
                printf("tot : %s\n", buffer);
                if (strcmp(prev_buffer, buffer) != 0) {
                    char str1[18] = { 0 }; // 처음 16자를 저장할 공간 (NULL 문자 포함)
                    char str2[18] = { 0 }; // 나머지 문자를 저장할 공간
                    if (strlen(buffer) > 16) {
                        strncpy(str1, buffer, 16); // 처음 16자 복사
                        str1[16] = '\0'; // NULL 문자 추가
                        strncpy(str2, buffer + 16, sizeof(str2) - 1); // 17번째 문자부터 복사
                        str2[sizeof(str2) - 1] = '\0'; // NULL 문자 추가
                    }
                    else {
                        strncpy(str1, buffer, sizeof(str1) - 1); // buffer 전체를 str1에 복사
                        str1[sizeof(str1) - 1] = '\0'; // NULL 문자 추가
                    }

                    lcd_init();
                    lcd_m(LINE1);
                    print_str(str1);

                    lcd_m(LINE2);
                    print_str(str2);
                    usleep(100000);
                }
                 
                strcpy(prev_buffer, buffer);

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

    fd = wiringPiI2CSetup(LCD_ADDR);  // I2C 장치 설정 (주소는 0x27)
    if (fd == -1) {
        printf("LCD initialization failed.\n");
        return -1;
    }

    memset(&car_serv_addr, 0, sizeof(car_serv_addr));
    car_serv_addr.sin_family = AF_INET;
    car_serv_addr.sin_addr.s_addr = inet_addr(argv[1]);
    car_serv_addr.sin_port = htons(PORT);

    if (connect(car_serv_sock, (struct sockaddr*)&car_serv_addr, sizeof(car_serv_addr)) < 0) {
        perror("Connection failed");
        exit(EXIT_FAILURE);
    }
    write(car_serv_sock, CLNT_ID, sizeof(CLNT_ID));

    pthread_t controller_to_car_output_thread, check_clnt_lcd_thread;
    pthread_create(&controller_to_car_output_thread, NULL, controller_to_car_output, (void*)&car_serv_sock);
    pthread_create(&check_clnt_lcd_thread, NULL, check_clnt_lcd, (void*)&car_serv_sock);

    pthread_detach(check_clnt_lcd_thread);
    pthread_join(controller_to_car_output_thread, NULL);

    close(car_serv_sock);
    return 0;
}
