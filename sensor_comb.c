#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <linux/spi/spidev.h>
#include <sys/time.h>

// GPIO setting
#define GPIO_PATH "/sys/class/gpio"
#define TRIG_PIN 23  // US sensor Trigger Pin
#define ECHO_PIN 24  // US sensor Echo Pin
#define TOUCH_PIN 17 // Touch sensor Pin

// SPI setting (Pressure Sensor & ADC setting)
#define SPI_DEVICE "/dev/spidev0.0"
#define SPI_MODE 0
#define SPI_BITS 8
#define SPI_SPEED 1000000
#define SPI_DELAY 5

// GPIO Export (GPIO activation set)
int gpioExport(int pin) {
    char path[50];
    int fd = open(GPIO_PATH "/export", O_WRONLY);
    if (fd == -1) {
        perror("GPIO Export Failed");
        return -1;
    }
    snprintf(path, sizeof(path), "%d", pin);
    write(fd, path, strlen(path));
    close(fd);
    return 0;
}

// GPIO Direction Seting (setting GPIO as input or out Pin)
int gpioSetDirection(int pin, const char *direction) {
    char path[50];
    snprintf(path, sizeof(path), GPIO_PATH "/gpio%d/direction", pin);
    int fd = open(path, O_WRONLY);
    if (fd == -1) {
        perror("Failed to Set GPIO Direction");
        return -1;
    }
    write(fd, direction, strlen(direction));
    close(fd);
    return 0;
}

// GPIO Read Value from Pin
int gpioRead(int pin) {
    char path[50], value;
    snprintf(path, sizeof(path), GPIO_PATH "/gpio%d/value", pin);
    int fd = open(path, O_RDONLY);
    if (fd == -1) {
        perror("GPIO Read Failure");
        return -1;
    }
    read(fd, &value, 1);
    close(fd);
    return value - '0';
}

// GPIO Write Value to Pin
int gpioWrite(int pin, int value) {
    char path[50];
    snprintf(path, sizeof(path), GPIO_PATH "/gpio%d/value", pin);
    int fd = open(path, O_WRONLY);
    if (fd == -1) {
        perror("GPIO Write Failure");
        return -1;
    }
    dprintf(fd, "%d", value);
    close(fd);
    return 0;
}

// MCP3008 Read pressure sensor Value
int readADC(int fd, u_int8_t channel) {
    u_int8_t tx[] = {1, (8 + channel) << 4, 0};
    u_int8_t rx[3];
    struct spi_ioc_transfer tr = {
        .tx_buf = (unsigned long)tx,
        .rx_buf = (unsigned long)rx,
        .len = sizeof(tx),
        .delay_usecs = SPI_DELAY,
        .speed_hz = SPI_SPEED,
        .bits_per_word = SPI_BITS,
    };

    if (ioctl(fd, SPI_IOC_MESSAGE(1), &tr) < 1) {
        perror("SPI transmission Failed");
        return -1;
    }
    return ((rx[1] & 3) << 8) + rx[2];
}

// US Sensor Measure Distance
float measureDistance() {
    struct timeval start, end;
    long duration;

    gpioWrite(TRIG_PIN, 0);
    usleep(2);
    gpioWrite(TRIG_PIN, 1);
    usleep(10);
    gpioWrite(TRIG_PIN, 0);

    while (gpioRead(ECHO_PIN) == 0);
    gettimeofday(&start, NULL);

    while (gpioRead(ECHO_PIN) == 1);
    gettimeofday(&end, NULL);

    duration = (end.tv_sec - start.tv_sec) * 1000000 + (end.tv_usec - start.tv_usec);
    return (float)(duration * 0.034) / 2;
}

int main() {
    // GPIO reset & acitvate
    gpioExport(TRIG_PIN);
    gpioExport(ECHO_PIN);
    gpioExport(TOUCH_PIN);

    gpioSetDirection(TRIG_PIN, "out");
    gpioSetDirection(ECHO_PIN, "in");
    gpioSetDirection(TOUCH_PIN, "in");

    // Set SPI
    int spi_fd = open(SPI_DEVICE, O_RDWR);
    if (spi_fd < 0) {
        perror("Failed to open SPI Device");
        return -1;
    }
    u_int8_t mode = SPI_MODE;
    u_int8_t bits = SPI_BITS;
    u_int32_t speed = SPI_SPEED;
    if (ioctl(spi_fd, SPI_IOC_WR_MODE, &mode) == -1 ||
        ioctl(spi_fd, SPI_IOC_WR_BITS_PER_WORD, &bits) == -1 ||
        ioctl(spi_fd, SPI_IOC_WR_MAX_SPEED_HZ, &speed) == -1) {
        perror("SPI 설정 실패");
        close(spi_fd);
        return -1;
    }

    while (1) { //1 scan per sec
        // US sensor distance Scan
        float distance = measureDistance();

        // Touch Sensor Scan
        int touchState = gpioRead(TOUCH_PIN);

        // Pressure Sensor Scan (MCP3008 Ch.0)
        int pressureValue = readADC(spi_fd, 0);

        // Print Sensing results
        printf("Distance: %.2f cm, Touch State: %d, Pressure: %d\n",
               distance, touchState, pressureValue);

        sleep(1); // wait 1 sec
    }

    close(spi_fd);
    return 0;
}

