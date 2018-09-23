/*
 * ssd1306_lcd.c
 *
 *  Created on: Aug 24, 2018
 *      Author: danao
 *
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
 */

#include <stdint.h>
#include <stddef.h>
#include <string.h>

#include "ssd1306_lcd.h"
#include "i2c.h"
#include "font_table.h"
#include "offset.h"


///////////////////////////////////////////////////////
//Static prototypes
static void LCD_WriteReg(uint8_t reg);
static uint8_t LCD_ReadReg(uint8_t reg);
static void LCD_WriteData(uint8_t data);
static void LCD_WriteDataArray(uint8_t* buffer, uint16_t len);

static void LCD_SetPage(uint8_t page);
static void LCD_SetColumn(uint8_t offset);




//////////////////////////////////////////
//Read the chip id from LCD_CHIP_ID_ADDR
//read reg
uint8_t LCD_ReadChipID(void)
{
	return 0;
}

////////////////////////////////////////////
//Setup LCD registers.  See Figure 2 in datasheet
void LCD_Init(void)
{
	LCD_WriteReg(0xA8);		//set mux ratio
	LCD_WriteReg(0x3F);

	LCD_WriteReg(0xD3);		//set display offset
	LCD_WriteReg(0x00);

	LCD_WriteReg(0x40);		//set display startline

	/////////////////////////////////
	//Mirror and Rotate - Use A0/A1
	//and C0/C8
//	LCD_WriteReg(0xA0);		//set segment remap - A0/A1 - col 0/127 at seg0
	LCD_WriteReg(0xA1);		//set segment remap - A0/A1 - col 0/127 at seg0

	LCD_WriteReg(0xC8);		//set scan direction - reverse
//	LCD_WriteReg(0xC0);		//set scan direction - reverse

	/////////////////////////////////

	LCD_WriteReg(0xDA);		//set pin configuration
//	LCD_WriteReg(0x02);		//32 pixels?
	LCD_WriteReg(0x12);		//64 pixels

	LCD_WriteReg(0x81);		//set contrast control
	LCD_WriteReg(0x7F);

	LCD_WriteReg(0xA4);		//disable entire display on

	LCD_WriteReg(0xA6);		//set normal display

	LCD_WriteReg(0xD5);		//set osc freq.
	LCD_WriteReg(0x80);

	LCD_WriteReg(0x8D);		//enable charge pump
	LCD_WriteReg(0x14);

	LCD_WriteReg(0xAF);		//display on

}


///////////////////////////////////////////////
//Clear framebuffer with a value and update
//display
void LCD_Clear(uint8_t value)
{
	uint8_t i, j = 0;
	for (i = 0 ; i < LCD_NUM_PAGES ; i++)
	{
		LCD_SetColumn(0x00);				//reset
		LCD_SetPage(i);						//set page

		for (j = 0 ; j < LCD_WIDTH ; j++)
			LCD_WriteData(value);
	}
}


///////////////////////////////////////////////
//Clears Pages 0 and 1 with value
void LCD_ClearTop(uint8_t value)
{
	uint8_t i, j = 0;
	for (i = 0 ; i < 2 ; i++)
	{
		LCD_SetColumn(0x00);					//reset
		LCD_SetPage(i);							//set page

		for (j = 0 ; j < LCD_WIDTH ; j++)
			LCD_WriteData(value);
	}
}

//////////////////////////////////////////////
//Clears Pages 2 - 7 with value
void LCD_ClearBottom(uint8_t value)
{
	uint8_t i, j = 0;
	for (i = 2 ; i < LCD_NUM_PAGES ; i++)
	{
		LCD_SetColumn(0x00);				//reset
		LCD_SetPage(i);						//set page

		for (j = 0 ; j < LCD_WIDTH ; j++)
			LCD_WriteData(value);
	}
}


/////////////////////////////////////////////////
//Clear a page with a value, updates the contents
//in the framebuffer using offset = page * LCD_WIDTH

//offtime -
//3.5us vs 11ms - huge difference.
//basically, the single clear page in an ISR at 100ms
//will not work.
//
void LCD_ClearPage(uint8_t page, uint8_t value)
{
	uint8_t j = 0;
	if (page < LCD_NUM_PAGES)
	{
		LCD_SetColumn(0x00);
		LCD_SetPage(page);

		for (j = 0 ; j < LCD_WIDTH ; j++)
			LCD_WriteData(value);
	}
}


////////////////////////////////////////////////
//Clear len bytes at page page, starting at x
//offset = offset, contents = value
//update the framebuffer
void LCD_ClearPage_Offset(uint8_t page, uint8_t offset, uint8_t len, uint8_t value)
{
	uint8_t j = 0;
	if ((offset + len) <= LCD_WIDTH)
	{
		if (page < LCD_NUM_PAGES)
		{
			LCD_SetColumn(offset);
			LCD_SetPage(page);

			for (j = 0 ; j < len ; j++)
				LCD_WriteData(value);
		}
	}
}






////////////////////////////////////////////////////////
//LCD_DrawCharKern
//helper function for drawstring kern
//uses offset table and kerning for closer
//character spacing.  assumes that the x and
//y addresses are set in the drawstringkern function
void LCD_DrawCharKern(uint8_t kern, uint8_t letter)
{
	unsigned char line;
	unsigned int value0;
	uint8_t width = 0x00;
	int i = 0;

	//get the line based on the letter
	line = letter - 27;		//ie, for char 32 " ", it's on line 5
	value0 = (line-1) << 3;

	//now, get the char width as
	//width = 8 - char offset
	width = 8 - offset[letter - 32];

	//loop through the width
	for (i = 0 ; i < width ; i++)
	{
		LCD_WriteData(font_table[value0 + i]);
	}

	//now write the remaining spacing between chars
	for (i = 0 ; i < kern ; i++)
	{
		LCD_WriteData(0x00);
	}
}


////////////////////////////////////////////////////////
//LCD_WriteStringKern
//implements offset and kern for controlled
//character spacing.  pass the kern and letter into
//the helper function for each character
//start at x = 0, and keep writing until the
//x coordinate  = 127 - (8 - offset) of that char
void LCD_DrawStringKern(uint8_t row_initial, uint8_t kern, const char* mystring)
{
	uint8_t row = row_initial & 0x07;       //max value of row is 7
	uint8_t count = 0;
	uint8_t position = 0;		//running position
	uint8_t temp = 0;

	unsigned char line;
	unsigned int value0;
	uint8_t width = 0x00;
	int i = 0;

	uint16_t element = 0x00;        //frame buffer element
	element = row * LCD_WIDTH;

	//set the x and y start positions
	LCD_SetPage(row);
	LCD_SetColumn(0);

	while ((mystring[count] != '\0') && (position < 127))
	{
		LCD_SetColumn(position);
		temp = 8 - offset[mystring[count] - 32] + kern;

		if ((position + temp) < 127)
		{
			//put the contents of the function here, we need
			//to update the frame buffer
			//LCD_DrawCharKern(kern, mystring[count]);

			line = mystring[count] - 27;		//ie, for char 32 " ", it's on line 5
			value0 = (line-1) << 3;

			//char width from offset
			width = 8 - offset[mystring[count] - 32];

			//loop through the width
			for (i = 0 ; i < width ; i++)
			{
				LCD_WriteData(font_table[value0 + i]);
				element++;
			}

			//now write the remaining spacing between chars
			for (i = 0 ; i < kern ; i++)
			{
				LCD_WriteData(0x00);
				element++;
			}
		}

		position += temp;
		count++;
	}
}


//////////////////////////////////////////////////////
//Draw string with kerning and offset
//Args: pointer to buffer and length.  Intended
//to be used with sprintf, etc. where length is known.
void LCD_DrawStringKernLength(uint8_t row_initial, uint8_t kern, uint8_t* mystring, uint8_t length)
{
	uint8_t row = row_initial & 0x07;       //max value of row is 7
	uint8_t count = 0;
	uint8_t position = 0;		//running position
	uint8_t temp = 0;

	unsigned char line;
	unsigned int value0;
	uint8_t width = 0x00;
	int i = 0;

	uint16_t element = 0x00;        //frame buffer element
	element = row * LCD_WIDTH;

	//set the x and y start positions
	LCD_SetPage(row);
	LCD_SetColumn(0);

	for (count = 0 ; count < length ; count++)
	{
		LCD_SetColumn(position);
		temp = 8 - offset[mystring[count] - 32] + kern;

		if ((position + temp) < 127)
		{
			line = mystring[count] - 27;		//ie, for char 32 " ", it's on line 5
			value0 = (line-1) << 3;
			width = 8 - offset[mystring[count] - 32];	//width from offset table

			for (i = 0 ; i < width ; i++)
			{
				LCD_WriteData(font_table[value0 + i]);			//write to LCD directly
				element++;
			}

			//write remaining spacing between chars
			for (i = 0 ; i < kern ; i++)
			{
				LCD_WriteData(0x00);
				element++;
			}
		}

		position += temp;			//update current column position with char width
	}
}







////////////////////////////////////////////////
//Write value to register
void LCD_WriteReg(uint8_t reg)
{
	uint8_t control = 0x00;			//bit 6 low for command
	uint8_t tx[2] = {control, reg};

	i2c_write(LCD_I2C_ADDRESS, tx, 2);
}

////////////////////////////////////////////////
//Read value from register
//reg read consists of a control byte followed by
//reg read command
//control byte - bit 7 = C - 0
//bit 6 - D/C - 0 = command, 1 = data
//bits 5-0 = 000000
uint8_t LCD_ReadReg(uint8_t reg)
{
	uint8_t control = 0x00;			//bit 6 low for command
//	uint8_t value = I2C3_ReadOneByte(LCD_I2C_ADDRESS, control);

	//send control and reg, then read one byte
	uint8_t tx[2] = {control, reg};
	uint8_t rx = 0x00;



//	HAL_I2C_Master_Transmit(&hi2c1, LCD_I2C_ADDRESS, tx, 2, 0xFFF);
//	HAL_I2C_Master_Receive(&hi2c1, LCD_I2C_ADDRESS, &rx, 1, 0xFFF);

	return rx;
}


//////////////////////////////////////////////////////
//Write single data byte to address
void LCD_WriteData(uint8_t data)
{
	uint8_t control = 0x40;				//bit 6 high for data
	uint8_t tx[2] = {control, data};

	i2c_write(LCD_I2C_ADDRESS, tx, 2);
}


//////////////////////////////////////////////////////
//Write array.
void LCD_WriteDataArray(uint8_t* buffer, uint16_t len)
{
	//function sends one cmd byte followed by array
	//of data bytes contained in the buffer.
	uint8_t cmdByte = 0x40;				//write data
	i2c_writeAddress(LCD_I2C_ADDRESS, cmdByte, buffer, len);
}






////////////////////////////////////////////////////
//Set the page offset on the LCD - 0 to 7
//assumes page addressing mode - B0 to B7
void LCD_SetPage(uint8_t page)
{
	if (page < LCD_NUM_PAGES)
	{
		uint8_t data = 0xB0 | page;
		LCD_WriteReg(data);
	}
}

////////////////////////////////////////////////////
//Set the x offset 0 - 127
//uses two writes - lower column address nibble
//and upper column address nibble
void LCD_SetColumn(uint8_t offset)
{
	if (offset < LCD_WIDTH)
	{
		uint8_t lower = offset & 0x0F;
		uint8_t upper = (offset >> 4) & 0x0F;

		LCD_WriteReg(0x00 | lower);		//base address - lower = 0x00
		LCD_WriteReg(0x10 | upper);		//base address - upper = 0x10
	}
}
















