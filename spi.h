#ifndef __SPI__

#define __SPI__



#include "header.h"



#define ARRAY_SIZE(a) (sizeof(a) / sizeof((a)[0]))



static const char *DEVICE = "/dev/spidev0.0";

static uint8_t MODE = 0;

static uint8_t BITS = 8;

static uint32_t CLOCK = 1000000;

static uint16_t DELAY = 5;



static int prepare(int fd) {

  if (ioctl(fd, SPI_IOC_WR_MODE, &MODE) == -1) {

    perror("Can't set MODE");

    return -1;

  }



  if (ioctl(fd, SPI_IOC_WR_BITS_PER_WORD, &BITS) == -1) {

    perror("Can't set number of BITS");

    return -1;

  }



  if (ioctl(fd, SPI_IOC_WR_MAX_SPEED_HZ, &CLOCK) == -1) {

    perror("Can't set write CLOCK");

    return -1;

  }



  if (ioctl(fd, SPI_IOC_RD_MAX_SPEED_HZ, &CLOCK) == -1) {

    perror("Can't set read CLOCK");

    return -1;

  }



  return 0;

}



uint8_t control_bits_differential(uint8_t channel) {

  return (channel & 7) << 4;

}



uint8_t control_bits(uint8_t channel) {

  return 0x8 | control_bits_differential(channel);

}



int readadc(int fd, uint8_t channel) {

  uint8_t tx[] = {1, control_bits(channel), 0};

  uint8_t rx[3];



  struct spi_ioc_transfer tr = {

      .tx_buf = (unsigned long)tx,

      .rx_buf = (unsigned long)rx,

      .len = ARRAY_SIZE(tx),

      .delay_usecs = DELAY,

      .speed_hz = CLOCK,

      .bits_per_word = BITS,

  };



  if (ioctl(fd, SPI_IOC_MESSAGE(1), &tr) == 1) {

    perror("IO Error");

    abort();

  }

  return ((rx[1] << 8) & 0x300) | (rx[2] & 0xFF);

}





int SPI_init()

{

   int fd = open(DEVICE, O_RDWR);

  if (fd <= 0) {

    perror("Device open error");

    return -1;

  }



  if (prepare(fd) == -1) {

    perror("Device prepare error");

    return -1;

  }

  return fd;

}



    //printf("value: %d\n", readadc(fd, 0));         //0번 채널의 값을 불러들임 (B의 값)

    //printf("value: %d\n", readadc(fd, 1));         //1번 채널의 값을 불러들임 (X의 값)

    //printf("value: %d\n", readadc(fd, 2));         //2번 채널의 값을 불러들임 (Y의 값)



#endif

