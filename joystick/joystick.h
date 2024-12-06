#ifndef __JOTSTICK__
#define __JOYSTICK__

#include "header.h"
#include "spi.h"


int initJoystick()
{
    int fd = open(DEVICE, O_RDWR);
    if (fd <= 0)
    {
        perror("Device open error");
        return -1;
    }

    if (prepare(fd) == -1)
    {
        perror("Device prepare error");
        return -1;
    }

    return fd;
}


int* readController(int fd) {
    int *ch = malloc(3 * sizeof(int));
    if (!ch) {
        perror("malloc failed");
        exit(EXIT_FAILURE);
    }

    for (int i = 0; i < 3; i++) {
        int value = readadc(fd, i);
        if (value < 0) {
            fprintf(stderr, "readadc failed on channel %d\n", i);
            free(ch); 
            exit(EXIT_FAILURE);
        }
        ch[i] = value;
    }
    		printf("X : %d  Y : %d  B : %d \n", ch[1], ch[2], ch[0]);


    return ch;
}
#endif
