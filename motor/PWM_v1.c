#ifndef __PWM__
#define __PWM__

#include "header.h"

#define IN 0
#define OUT 1
#define PWM 0
#define MAX_PWM 2  // 최대 PWM 핀 개수

#define LOW 0
#define HIGH 1
#define VALUE_MAX 256
#define DIRECTION_MAX 256

int duty_cycle_fds[MAX_PWM] = {-1, -1};  // PWM 핀별 duty_cycle 파일 디스크립터 저장


static int PWMExport(int pwmnum) {
#define BUFFER_MAX 3
  char buffer[BUFFER_MAX];
  int fd, byte;

  // TODO: Enter the export path.
  fd = open("/sys/class/pwm/pwmchip0/export", O_WRONLY);
  if (-1 == fd) {
    fprintf(stderr, "Failed to open export for export!\n");
    return (-1);
  }

  byte = snprintf(buffer, BUFFER_MAX, "%d", pwmnum);
  write(fd, buffer, byte);
  close(fd);

  sleep(1);

  return (0);
}

static int PWMEnable(int pwmnum) {
  static const char s_enable_str[] = "1";

  char path[DIRECTION_MAX];
  int fd;

  // TODO: Enter the enable path.
  snprintf(path, DIRECTION_MAX, "/sys/class/pwm/pwmchip0/pwm%d/enable", pwmnum);
  fd = open(path, O_WRONLY);
  if (-1 == fd) {
    fprintf(stderr, "Failed to open in enable!\n");
    return -1;
  }

  write(fd, s_enable_str, strlen(s_enable_str));
  close(fd);

  return (0);
}

static int PWMWritePeriod(int pwmnum, int value) {
  char s_value_str[VALUE_MAX];
  char path[VALUE_MAX];
  int fd, byte;

  // TODO: Enter the period path.
  snprintf(path, VALUE_MAX, "/sys/class/pwm/pwmchip0/pwm%d/period", pwmnum);
  fd = open(path, O_WRONLY);
  if (-1 == fd) {
    fprintf(stderr, "Failed to open in period!\n");
    return (-1);
  }
  byte = snprintf(s_value_str, VALUE_MAX, "%d", value);

  if (-1 == write(fd, s_value_str, byte)) {
    fprintf(stderr, "Failed to write value in period!\n");
    close(fd);
    return -1;
  }
  close(fd);

  return (0);
}

int OpenDutyCycleFile(int pwmnum) {
    if (pwmnum >= MAX_PWM) {
        fprintf(stderr, "PWM pin %d is out of range! (MAX: %d)\n", pwmnum, MAX_PWM);
        return -1;
    }

    char path[VALUE_MAX];
    snprintf(path, VALUE_MAX, "/sys/class/pwm/pwmchip0/pwm%d/duty_cycle", pwmnum);

    duty_cycle_fds[pwmnum] = open(path, O_WRONLY);
    if (duty_cycle_fds[pwmnum] == -1) {
        fprintf(stderr, "Failed to open duty cycle file for PWM %d!\n", pwmnum);
        return -1;
    }

    return 0;
}

// 특정 PWM 핀의 duty_cycle 파일 닫기
void CloseDutyCycleFile(int pwmnum) {
    if (pwmnum >= MAX_PWM || duty_cycle_fds[pwmnum] == -1) {
        fprintf(stderr, "PWM pin %d is not open or out of range!\n", pwmnum);
        return;
    }

    close(duty_cycle_fds[pwmnum]);
    duty_cycle_fds[pwmnum] = -1;
}

// 특정 PWM 핀의 duty_cycle 업데이트
int PWMWriteDutyCycle(int pwmnum, int value) {
  printf("%d\n", duty_cycle_fds[pwmnum]);
    if (pwmnum >= MAX_PWM || duty_cycle_fds[pwmnum] == -1) {
        fprintf(stderr, "Duty cycle file for PWM %d is not open or out of range!\n", pwmnum);
        return -1;
    }

    char s_value_str[VALUE_MAX];
    int byte = snprintf(s_value_str, VALUE_MAX, "%d", value);

    if (write(duty_cycle_fds[pwmnum], s_value_str, byte) == -1) {
        fprintf(stderr, "Failed to write duty cycle value for PWM %d!\n", pwmnum);
        return -1;
    }

    return 0;
}

static int PWMWriteDutyCycle1(int pwmnum, int value) {
  char s_value_str[VALUE_MAX];
  char path[VALUE_MAX];
  int fd, byte;

  // TODO: Enter the duty_cycle path.
  snprintf(path, VALUE_MAX, "/sys/class/pwm/pwmchip0/pwm%d/duty_cycle", pwmnum);
  fd = open(path, O_WRONLY);
  if (-1 == fd) {
    fprintf(stderr, "Failed to open in duty cycle!\n");
    return (-1);
  }
  byte = snprintf(s_value_str, VALUE_MAX, "%d", value);

  if (-1 == write(fd, s_value_str, byte)) {
    fprintf(stderr, "Failed to write value in duty cycle!\n");
    close(fd);
    return -1;
  }
  close(fd);

  return (0);
}


void PWMinit(int pwm) {
    PWMExport(pwm);
    PWMWritePeriod(pwm, 10000000);  // PWM 주기 설정
    PWMWriteDutyCycle1(pwm, 0);     // 초기 듀티 사이클을 0으로 설정
    PWMEnable(pwm);
}

#endif
