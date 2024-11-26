#ifndef __GPIO__
#define __GPIO__

#define IN 0
#define OUT 1

#define LOW 0
#define HIGH 1

#define VALUE_MAX1 40
#define DIRECTION_MAX1 40

void error_handling(char *message)
{
    fputs(message, stderr);
    fputc('\n', stderr);
    exit(1);
}

//커널에 GPIO의 제어권을 이 파일에 기록하여 사용자 공간으로 내보내도록 요청
static int GPIOExport(int pin)
{
#define BUFFER_MAX 3
    char buffer[BUFFER_MAX];
    ssize_t bytes_written;
    int fd;

    fd = open("/sys/class/gpio/export", O_WRONLY);
    if (-1 == fd)
    {
        fprintf(stderr, "Failed to open export for writing!\n");
        return (-1);
    }

    bytes_written = snprintf(buffer, BUFFER_MAX, "%d", pin);
    write(fd, buffer, bytes_written);
    close(fd);
    return (0);
}


//GPIO의 제어권 반납(Export 반대 과정) 오류 발생시 생략 가능
static int GPIOUnexport(int pin)
{
    char buffer[BUFFER_MAX];
    ssize_t bytes_written;
    int fd;

    fd = open("/sys/class/gpio/unexport", O_WRONLY);
    if (-1 == fd)
    {
        fprintf(stderr, "Failed to open unexport for writing!\n");
        return (-1);
    }

    bytes_written = snprintf(buffer, BUFFER_MAX, "%d", pin);
    write(fd, buffer, bytes_written);
    close(fd);
    return (0);
}


//해당 GPIO의 입력 OR 출력 결정
static int GPIODirection(int pin, int dir)
{
    static const char s_directions_str[] = "in\0out";

    char path[DIRECTION_MAX1];
    int fd;

    snprintf(path, DIRECTION_MAX1, "/sys/class/gpio/gpio%d/direction", pin);
    fd = open(path, O_WRONLY);
    if (-1 == fd)
    {
        fprintf(stderr, "Failed to open gpio direction for writing!\n");
        return (-1);
    }

    if (-1 ==
        write(fd, &s_directions_str[IN == dir ? 0 : 3], IN == dir ? 2 : 3))
    {
        fprintf(stderr, "Failed to set direction!\n");
        return (-1);
    }

    close(fd);
    return (0);
}

//GPIORead fd(File Descripter) - gpio pin의 값을 읽어오는 것
static int GPIORead(int pin)
{
    char path[VALUE_MAX1];
    char value_str[3];
    int fd;

    snprintf(path, VALUE_MAX1, "/sys/class/gpio/gpio%d/value", pin);
    fd = open(path, O_RDONLY);
    if (-1 == fd)
    {
        fprintf(stderr, "Failed to open gpio value for reading!\n");
        return (-1);
    }

    if (-1 == read(fd, value_str, 3))
    {
        fprintf(stderr, "Failed to read value!\n");
        return (-1);
    }

    close(fd);

    return (atoi(value_str));
}


//GPIOWrite value 
static int GPIOWrite(int pin, int value)
{
    static const char s_values_str[] = "01";

    char path[VALUE_MAX1];
    int fd;
    
    // Construct the path to the GPIO value file
    snprintf(path, VALUE_MAX1, "/sys/class/gpio/gpio%d/value", pin);
    fd = open(path, O_WRONLY);
    if (-1 == fd)
    {
        fprintf(stderr, "Failed to open gpio value for writing!\n");
        return (-1);
    }

    if (1 != write(fd, &s_values_str[LOW == value ? 0 : 1], 1))
    {
        fprintf(stderr, "Failed to write value!\n");
        return (-1);
    }

    close(fd);
    return (0);
}

#endif