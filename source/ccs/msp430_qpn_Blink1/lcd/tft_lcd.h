/*
Header file for TFT LCD Driver
This is for the 128 x 160 lcd display from
adafruit.  This file is intended for use 
with the MSP 430 chip.  The LCD uses the
ST7735R controller. 
1/26/15
*/

#ifndef LCD_TFT_H__
#define LCD_TFT_H__

#include <msp430g2553.h>
#include <stdio.h>
#include <stddef.h>
#include <stdint.h>
#include "font.h"
#include <math.h>

//defines
#define CS_PIN		BIT5			//port 1
#define SOMI_PIN	BIT1			//port 1
#define SIMO_PIN	BIT2			//port 1
#define CLK_PIN		BIT4			//port 1
#define RESET_PIN	BIT0			//port 2
#define DC_PIN		BIT1			//port 2


////////////////////
//color defines
/* LCD color */
#define BLACK                   0x0000
#define NAVY                    0x000F
#define DGREEN                  0x03E0
#define DCYAN                   0x03EF
#define MAROON                  0x7800
#define PURPLE                  0x780F
#define OLIVE                   0x7BE0
#define GREY                    0xF7DE
#define LGRAY                   0xC618
#define DGRAY                   0x7BEF
#define BLUE                    0x001F
#define BLUE2                   0x051F
#define GREEN                   0x07E0
#define CYAN                    0x07FF
#define RED                     0xF800
#define MAGENTA                 0xF81F
#define YELLOW                  0xFFE0
#define WHITE                   0xFFFF

#define swap(a, b) { int16_t t = a; a = b; b = t; }

#define PI					3.14159

//function prototypes
void SPIA_init(void);				//setup for SPI
void SPIA_WriteCommand(uint8_t);	//command only
void SPIA_WriteData(uint8_t);		//data only

void LCD_Clear(uint16_t);

void LCD_PutPixel(uint8_t x, uint8_t y, uint16_t color);
void LCD_DrawLine(int16_t x0, int16_t y0, int16_t x1, int16_t y1, uint16_t color);

void LCD_DrawChar88(uint8_t row, uint8_t col, uint8_t letter);
void LCD_DrawString(uint8_t row, char* temp, uint16_t font, uint16_t back);

//draw string function that draws the complete string at one time
//it puts kern pixels between each character
void LCD_DrawStringx(uint8_t row, char* mystring, uint8_t kern, uint16_t font, uint16_t back);

uint16_t rgb2bgr(uint16_t);

void SPIA_Write8(uint8_t, uint8_t);								//command + 1 data
void SPIA_Write16(uint8_t, uint8_t, uint8_t);					//command + 2 data
void SPIA_Write24(uint8_t, uint8_t, uint8_t, uint8_t);			//command + 3 data
void SPIA_Write32(uint8_t, uint8_t, uint8_t, uint8_t, uint8_t);	//command + 4 data

void LCD_init(void);			//send commands to set up the LCD

void delay_us(volatile int ticks);			//short delay function













#endif
