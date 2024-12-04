#ifndef __LCD__
#define __LCD__

#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <wiringPi.h>
#include <wiringPiI2C.h>
#include <memory.h>
#include <string.h>
#include <string.h>
#include <time.h>

#define PIR 4       // BCM_GPIO 23

#define LCD_ADDR   0x27
#define LCD_CHR  1
#define LCD_CMD  0
#define LINE1  0x80
#define LINE2  0xC0
#define LCD_BACKLIGHT   0x08
#define ENABLE  0b00000100
#define BUTTON_PIN 23

int fd;
void lcd_init(void);
void lcd_byte(int bits, int mode);
void lcd_toggle_enable(int bits);
void lcd_m(int line);
void print_int(int i);
void cursor_to_home(void);
void print_str(const char *s);
void get_set(void);
int setup_touch_sensor(void);

int setup_touch_sensor(void)
{
    // Check if WiringPi setup is successful
    if (wiringPiSetup() == -1)
        return -1;
    // Set the mode of the PIR pin to INPUT
    pinMode(PIR, INPUT);

    return 0;
}

void print_int(int i) {
    // Declare a character array to store the converted integer as a string
    char array1[20];
    // Convert the integer 'i' to a string and store it in 'array1'
    sprintf(array1, "%d", i);
    // Call the 'print_str' function to print the string
    print_str(array1);
}

void cursor_to_home(void) {
    // Send the command 0x01 to clear the display
    lcd_byte(0x01, LCD_CMD);
    // Send the command 0x02 to move the cursor to the home position
    lcd_byte(0x02, LCD_CMD);    
}

void lcd_m(int line) {
    // Send the value of 'line' as a command to the LCD
    lcd_byte(line, LCD_CMD);
}

//send each character to the LCD in data mode
void print_str(const char *s) {
    while (*s) lcd_byte(*(s++), LCD_CHR);
}


//responsible for sending a byte to the LCD
void lcd_byte(int bits, int mode) {
    int bits_high;
    int bits_low;

    bits_high = mode | (bits & 0xF0) | LCD_BACKLIGHT;
    bits_low = mode | ((bits << 4) & 0xF0) | LCD_BACKLIGHT;

    wiringPiI2CReadReg8(fd, bits_high);
    lcd_toggle_enable(bits_high);

    wiringPiI2CReadReg8(fd, bits_low);
    lcd_toggle_enable(bits_low);
}

//responsible for toggling the enable signal of the LCD
void lcd_toggle_enable(int bits) {
    delayMicroseconds(500);
    wiringPiI2CReadReg8(fd, (bits | ENABLE));
    delayMicroseconds(500);
    wiringPiI2CReadReg8(fd, (bits & ~ENABLE));
    delayMicroseconds(500);
}

//initializes the LCD with a sequence of commands
void lcd_init() {
    lcd_byte(0x33, LCD_CMD);
    lcd_byte(0x32, LCD_CMD);
    lcd_byte(0x06, LCD_CMD);
    lcd_byte(0x0C, LCD_CMD);
    lcd_byte(0x28, LCD_CMD);
    lcd_byte(0x01, LCD_CMD);
    delayMicroseconds(500);
}

void ClrLcd(void) {
    lcd_byte(0x01, LCD_CMD);
    lcd_byte(0x02, LCD_CMD);
}

#endif