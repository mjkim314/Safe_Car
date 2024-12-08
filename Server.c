
#include "header.h"
#include "joystick.h"
#include "GPIO.h"
#include "spi.h"
#include "motor.h"
#include "hash_table.h"
//#include "lcd.h"


#define LED_PIN 29
#define BUZZER_PIN 28
#define PORT 12345
#define CLNT_SIZE 3
int motor_control = 0;
int crash_detect_ou = 0;

int clnt_count = 0; //클라이언트 수를 체크
int joy_data[3] = { 0, 0, 1 }; //조이스틱 데이터(0 : x값, 1 : y값, 2 : 1이면 버튼 off, 0이면 버튼 on) -400~+400
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


            if (bytes_read > 0) {
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
    int car_clnt_sock = *(int*)arg;
    char buffer[256];
    ssize_t bytes_read;
    struct timespec delay;
    delay.tv_sec = 0;
    delay.tv_nsec = 10000000;
    int count = 0;
   
    while (1) {

        if (search_table(clnt_info, "SAFETY") && count % 30 == 0) { // 0.5초마다 값 읽기
            bytes_read = read(car_clnt_sock, buffer, sizeof(buffer)-1);
            if (bytes_read > 0) {

                //받은 값에 따라 어떤 행동을 할 지 결정
                if (strncmp(buffer, "1", sizeof("1")) == 0) {
                    //1번 케이스는 모터 잠깐 멈춰서 사용자 깨우기
                    motor_control = 1;

                }
                else if (strncmp(buffer, "2", sizeof("2")) == 0) {
                    //2번 케이스는 비상등 점등, 부저 울리기, 모터 천천히 멈추기
                    //부저, led 기능은 led 제어 스레드, 부저 스레드에서 각각 처리해야됨(여기서 처리x)
                    //LED 제어는 코드 추가함
                    motor_control = 2;

                }


            }
            else
            {
                close(car_clnt_sock);
                remove_from_table(clnt_info, "SAFETY");
                motor_control = 1;

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

void* detect_crash(void* arg) {
   int car_clnt_sock = *(int*)arg;
   char buffer[256];
   struct timespec delay;
   ssize_t bytes_read;
   delay.tv_sec = 0;
   delay.tv_nsec = 10000000;
   int count = 0;

   while (1) {
       if (search_table(clnt_info, "CRASH") && count % 25 == 0) { // 0.5초마다 값 읽기
           bytes_read = read(car_clnt_sock, buffer, sizeof(buffer) - 1);

           if (bytes_read > 0) {
               
                if (strncmp(buffer, "F", sizeof("F")) == 0) {
                    crash_detect_ou = 1;
                    usleep(20000);
                    crash_detect_ou = 0;
                }
                else if (strncmp(buffer, "B", sizeof("B")) == 0) {
                    crash_detect_ou = 1;
                    usleep(20000);
                    crash_detect_ou = 0;
                }
                else{
                    continue;
                }

           }
           else
           {
                close(car_clnt_sock);
                remove_from_table(clnt_info, "CRASH");
                print_clients(clnt_info);
                clnt_count--;
                pthread_exit(NULL);
           }

       }
      memset(buffer, 0, sizeof(buffer));
      count++;
      nanosleep(&delay, NULL);
   }
   return NULL;
}


void* control_motor(void* arg) {
    while (1) {
        if (!search_table(clnt_info, "CONTROL")) {
            //모터 구동 필요 없음, 코드 없어도 됨
            continue;
        }
        else if (!search_table(clnt_info, "CRASH") || !search_table(clnt_info, "SAFETY")) {
            //CONTROL 클라이언트는 있는데 CRASH, SAFETY 클라이언트가 없을 경우, 최대 속도 제한 (정지아님)
            if(joy_data[1] > 250){
                joy_data[1] = 230;
            }else if(joy_data[1] < -250){
                joy_data[1] = -230;
            }else{
                joy_data[1] *- 0.9;
            }
            
        }
        //정상 작동시 코드
        if (motor_control == 1 || crash_detect_ou == 1) {
            printf("[*]Emergency Brake Activate!\n");
            stopMotor();
            sleep(1);
            motor_control = 0;
        }
        else if (motor_control == 2) {
            printf("[*]Slow Stop Activate!\n");
            slowStop(joy_data[1]);
            motor_control = 0;
        }
        else if(motor_control == 0){
            changeDutyCycle(joy_data[0], joy_data[1]);
            usleep(10000);
        }

        
    }
    return NULL;
}

void playTone(int frequency, int duration) {//Buzzer 재생 함수
    int delay = 1000000 / frequency / 2; // 주기 계산
    int cycles = frequency * duration / 1000; // 주어진 시간 동안 사이클 수 계산
    for (int i = 0; i < cycles; i++) {
        digitalWrite(BUZZER_PIN, HIGH); // 부저 ON
        usleep(delay);                 // 고음 유지 시간
        digitalWrite(BUZZER_PIN, LOW);  // 부저 OFF
        usleep(delay);                 // 저음 유지 시간
    }
}

void* controller_to_car_input_btn(void* arg) { //조이스틱 버튼을 눌렀을 때 led 켜짐(다른 스레드에서 병렬처리)

    pinMode(LED_PIN, OUTPUT);
    pinMode(BUZZER_PIN, OUTPUT);
    
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
            int song[] = {523, 587, 659};//{1318, 1396, 1567};
            for(int j=0; j<3; j++){
                digitalWrite(LED_PIN, 1);
                for (int i=0; i<3; i++){
                    playTone(song[i], 300);
                }
                digitalWrite(LED_PIN, 0);
                usleep(100000);
            }
            digitalWrite(BUZZER_PIN, LOW);
        }
    }
    return NULL;

}

int main(int argc, char* argv[])
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

    if (bind(car_serv_sock, (struct sockaddr*)&car_serv_addr, sizeof(car_serv_addr)) < 0)
    {
        error_handling("RC Bind failed");
        exit(EXIT_FAILURE);
    }

    if (listen(car_serv_sock, 3) < 0)
    {
        error_handling("Listen failed");
        exit(EXIT_FAILURE);
    }



    initMotor();
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
            else if (strncmp(buffer, "CRASH", strlen("CRASH")) == 0)
            {
                add_to_table(clnt_info, "CRASH", car_clnt_sock);
                pthread_t detect_crash_thread;
                pthread_create(&detect_crash_thread, NULL, detect_crash, (void*)&car_clnt_sock);
                pthread_detach(detect_crash_thread); 

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
