#ifndef __NOKIA_H
#define __NOKIA_H

/*
Nokia XXXXX 84x48 Graphic Display

*/

#include <stdint.h>
#include <msp430.h>


#define LCD_DC_PIN              BIT3
#define LCD_RESET_PIN           BIT4
#define LCD_CS_PIN              BIT5


#define LCD_WIDTH               84
#define LCD_HEIGHT              48
#define LCD_MAX_ROW             5
#define LCD_MAX_COL             83


void LCD_DummyDelay(unsigned int delay);

void LCD_init(void);			    
void LCD_WriteCommand(uint8_t command);
void LCD_WriteData(uint8_t data);
void LCD_SetRow(uint8_t row);
void LCD_SetColumn(uint8_t col);
void LCD_Clear(uint8_t value);

void LCD_ClearRow(uint8_t row, uint8_t value);

uint8_t LCD_DecimaltoBuffer(uint32_t val, char* buffer, uint8_t size);
void LCD_DrawChar(uint8_t row, uint8_t col, uint8_t letter);
void LCD_WriteString(uint8_t row, const char* mystring);
void LCD_WriteStringLength(uint8_t row, char* buffer, uint8_t len);


#endif


