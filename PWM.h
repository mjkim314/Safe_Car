#ifndef __PWM__
#define __PWM__

#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#define IN 0
#define OUT 1

#define LOW 0
#define HIGH 1
#define VALUE_MAX 256
#define DIRECTION_MAX 256

static int PWMExport(int pwmnum) {
#define BUFFER_MAX 3
	char buffer[BUFFER_MAX];
	int fd, byte;
	
	fd = open("/sys/class/pwm/pwmchip0/export", O_WRONLY);
	if (-1 == fd) {
		fprintf(stderr, "Failed to open export for export!\n");
		return (-1);
	}
	
	byte = snprintf(buffer, BUFFER_MAX, "%d", pwmnum);
	write(fd, buffer, byte);
	close(fd);
	
	sleep(2);
	return (0);
}

static int PWMEnable(int pwmnum) {
	static const char s_enable_str[] = "1";
	char path[DIRECTION_MAX];
	int fd;
	
	snprintf(path, DIRECTION_MAX, "/sys/class/pwm/pwmchip0/pwm0/enable", pwmnum);
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
	
	snprintf(path, VALUE_MAX, "/sys/class/pwm/pwmchip0/pwm0/period", pwmnum);
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

static int PWMWriteDutyCycle(int pwmnum, int value) {
	char s_value_str[VALUE_MAX];
	char path[VALUE_MAX];
	int fd, byte;
	
	snprintf(path, VALUE_MAX, "/sys/class/pwm/pwmchip0/pwm0/duty_cycle", pwmnum);
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
	PWMWriteDutyCycle(pwm, 0);      // 초기 듀티 사이클을 0으로 설정
	PWMEnable(pwm);
}

#endif
