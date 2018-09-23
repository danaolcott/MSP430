/*
 * ssd1306_lcd.h
 *
 *  Created on: Aug 24, 2018
 *      Author: danao
 *
 *  LCD Files for the SSD1306 Display Controller
 *  Attached to a 0.96 inch OLED Display.
 *  on a breakout board with I2C interface.
 *  This particular setup is connected to I2C3 on
 *  PA8 and PC9 on the STM32F407 discovery board.
 *  The display is available from Amazon.
 *
 *  When using the MSP430, i2c is connected on P1.6
 *  and P1.7
 *
 *  The purpose of this controller file is to
 *  init the lcd and provide basic graphics functions
 *  such as clear, fill, put pixel, draw line... etc.
 *
 *
 *  128x64 display, 4 color??
 */

#ifndef SSD1306_LCD_H_
#define SSD1306_LCD_H_



#define LCD_HEIGHT				64
#define LCD_WIDTH				128
#define LCD_NUM_PAGES			8
//#define LCD_I2C_ADDRESS			0x78		//this is the upshifted value
#define LCD_I2C_ADDRESS			(0x78 >> 1)		//non-upshifted value

//reg defines
#define LCD_CHIP_ID_ADDR		0x00		//??





uint8_t LCD_ReadChipID(void);
void LCD_Init(void);

void LCD_Clear(uint8_t value);
void LCD_ClearTop(uint8_t value);
void LCD_ClearBottom(uint8_t value);
void LCD_ClearPage(uint8_t page, uint8_t value);
void LCD_ClearPage_Offset(uint8_t page, uint8_t offset, uint8_t len, uint8_t value);

//graphics functions
void LCD_DrawCharKern(uint8_t kern, uint8_t letter);
void LCD_DrawStringKern(uint8_t row_initial, uint8_t kern, const char* mystring);
void LCD_DrawStringKernLength(uint8_t row_initial, uint8_t kern, uint8_t* mystring, uint8_t length);



#endif /* SSD1306_LCD_H_ */
