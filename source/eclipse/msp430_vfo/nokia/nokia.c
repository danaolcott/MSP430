/*
Nokia 84x48 Graphic LCD

SPI interface using SPIA.  In addition to 
SPI intrface, also uses data/cmd pin and 
a reset pin.  These are setup on P2.3 and P2.4
in the LCD init function.  Due to limited
ram, only put together functions for basic
clearing and simple text.


*/

#include <stdint.h>

#include "nokia.h"
#include "font.h"



////////////////////////////////////////
//
void LCD_DummyDelay(unsigned int delay)
{
    volatile unsigned int temp = delay;

    while (temp > 0)
        temp--;
}


//////////////////////////////////
//LCD_init()
//Configure the Nokia SPI interface and
//associated registers for controlling
//the display.
//
//Interface uses SPIA, P1.5 for CS line,
//P2.3 data/cmd line, and P2.4 for reset line
//
void LCD_init(void)
{
	LCD_SPI_init();

	//reset the lcd and put the control
	//pins where they need to be
	P2OUT &=~ LCD_RESET_PIN;
	LCD_DummyDelay(2000);
	P2OUT |= LCD_RESET_PIN;

	P1OUT |= LCD_CS_PIN;	//deselect
	P2OUT &=~ LCD_DC_PIN;	//command

	//init lcd.  Setup routine from Sparkfun

	LCD_WriteCommand(0x21); //Tell LCD that extended commands follow
	LCD_WriteCommand(0xB0); //Set LCD Vop (Contrast): Try 0xB1(good @ 3.3V) or 0xBF if your display is too dark
	LCD_WriteCommand(0x04); //Set Temp coefficent

	//0x13 is really faded, 0x14 less faded
	//looks like you can go to 0x17
	//0x15 - black, but you start to see background
	LCD_WriteCommand(0x15); //LCD bias mode 1:48: Try 0x13 or 0x14
	LCD_WriteCommand(0x20); //We must send 0x20 before modifying the display control mode
	LCD_WriteCommand(0x0C); //Set display control, normal mode. 0x0D for inverse

	LCD_DummyDelay(2000);

	//clear it and go home
	LCD_Clear(0x00);
	LCD_SetRow(0);
	LCD_SetColumn(0);
}


//////////////////////////////////////
//Configure SPI interface and control
//lines for the Nokia display
//
void LCD_SPI_init(void)
{
	//chip select - P1.5
	P1DIR |= LCD_CS_PIN;
	P1OUT |= LCD_CS_PIN;		//deselect device

	//control lines - reset and data/cmd pins
	P2DIR |= LCD_RESET_PIN;		//output P2.4
	P2DIR |= LCD_DC_PIN;		//output P2.3

	P2OUT |= LCD_RESET_PIN;
	P2OUT &=~ LCD_DC_PIN;


    //SPI A setup routine.  Start by holding the
    //USCI in reset state while setting up registers
    UCB0CTL1 = UCSWRST;

    //* P1.1 - UCA0SOMI	- slave out, master in
    //* P1.2 - UCA0SIMO - slave in master out
    //* P1.4 - UCA0CLK - serial clock

    P1SEL |= BIT1 | BIT2 | BIT4;
    P1SEL2 |= BIT1 | BIT2 | BIT4;

    //set all the registers while the UCSWRST bit is high
    //see page 445 of the programmers manual
    //UCCKPH - high - data captured on the first edge
    //UCCKPL - high - inactive clock is high
    //USCMSB - high - MSB first
    //UC7BIT - high - 7 bit, low - 8 bit
    //UCMST - highi - master mode
    //UCMODEx - not set - 3 wire, output only - 2 bit value
    //UCSYNC - high - synchronous mode, low is assync mode

    //the WDOG display is idle clock high, data read on
    //the second edge

//    UCA0CTL0 |= UCCKPL + UCMSB + UCMST + UCSYNC;  // 3-pin, 8-bit SPI master

    UCA0CTL0 |= UCCKPH + UCMSB + UCMST + UCSYNC;  // 3-pin, 8-bit SPI master

	//clock control - USCI clock source. 1 - aclock, 2 smclock
	//3 smclock
	UCA0CTL1 |= UCSSEL_2;                     // SMCLK


	//bit rate control register - two registers
	//high bytes on BR1, low bytes on BR0.  no need to set
	//the BR1. seems to be up to 16 divider, so try 0x02, 0x04
	//0x08, etc.  In theory, this could slow it down so we can
	//run a higher clock speed but keep the bit generator slow
	UCA0BR0 |= 0x40;                          //prescaler 64 - low bits
	UCA0BR1 = 0;                              //prescaler - high bits
	UCA0MCTL = 0;                             // No modulation, not sure ??

    //clear the bit to start the SPI module
    UCA0CTL1 &= ~UCSWRST;

}



////////////////////////////////////
//LCD_writeCommand()
//writes command to the LCD
void LCD_WriteCommand(uint8_t command)
{
	P2OUT &=~ LCD_DC_PIN;		//low for command
	P1OUT &=~ LCD_CS_PIN;		//select device

    while (!(IFG2 & UCA0TXIFG));   // USCI_A0 TX buffer ready?

    UCA0TXBUF = command;
    while (!(IFG2 & UCA0TXIFG));   // USCI_A0 TX buffer ready?
    while(UCA0STAT & UCBUSY);

	P1OUT |= LCD_CS_PIN;		//unselect device
}


///////////////////////////////////
//LCD_writeData()
//writes data to the LCD
void LCD_WriteData(uint8_t data)
{
	P2OUT |= LCD_DC_PIN;		//high for data
	P1OUT &=~ LCD_CS_PIN;		//select device

    while (!(IFG2 & UCA0TXIFG));   // USCI_A0 TX buffer ready?

    UCA0TXBUF = data;
    while (!(IFG2 & UCA0TXIFG));   // USCI_A0 TX buffer ready?
    while(UCA0STAT & UCBUSY);

	P1OUT |= LCD_CS_PIN;		//unselect device
	P2OUT &=~ LCD_DC_PIN;		//low, back to instruction
}


/////////////////////////////////////
//LCD_SetRow(row)
//
//Sets the row address in LCD ram
//values range from 0 - 5 (6rows)
//it's a write command with a base value
//of 0x40.  add the row value and
//you're done
void LCD_SetRow(uint8_t row)
{
	//test the row 0 to 5
	if (row <= LCD_MAX_ROW)
	{
        uint8_t value = 0x40 | row;
        LCD_WriteCommand(value);
	}
}

////////////////////////////////////
//LCD_SetColumn(col)
//
//Sets the column address, values range
//from 0 to 83??.  The base value in the
//command is 0x80.  add the column value
//and you're done

void LCD_SetColumn(uint8_t col)
{
	//test the column
	if (col < LCD_WIDTH)
	{
        uint8_t value = 0x80 | col;
        LCD_WriteCommand(value);
	}
}





//////////////////////////////////
//LCD_Clear(uint8_t)
void LCD_Clear(uint8_t value)
{
	//reset to home
	LCD_SetRow(0);
	LCD_SetColumn(0);

	int i, j;
	for (i = 0 ; i < 6 ; i++)
	{
		for (j = 0 ; j < 84 ; j++)
		{
			LCD_WriteData(value);
		}
	}
}


void LCD_ClearRow(uint8_t row, uint8_t value)
{
	uint8_t count = 84;

	if (row <= LCD_MAX_ROW)
	{
		LCD_SetRow(row);
		LCD_SetColumn(0);

		while (count > 0)
		{
			LCD_WriteData(value);
			count--;
		}
	}
}


//////////////////////////////////////////
//Print val into buffer of max size size.
//returns the number of characters in the
//the buffer.  Assumes the buffer
//is clear prior to writing to it.
uint8_t LCD_DecimaltoBuffer(uint32_t val, char* buffer, uint8_t size)
{
    uint8_t i = 0;
    char digit;
    uint8_t num = 0;
    char t;

    while (val > 0)
    {
        digit = (char)(val % 10);
        buffer[num] = (0x30 + digit) & 0x7F;
        num++;
        val = val / 10;
    }

    //reverse in place
    for (i = 0 ; i < num / 2 ; i++)
    {
        t = buffer[i];
        buffer[i] = buffer[num - i - 1];
        buffer[num - i - 1] = t;
    }

    buffer[num] = 0x00;     //null terminated

    return num;
}



//////////////////////////////////////////
//8 x 8 pixel character
void LCD_DrawChar(uint8_t row, uint8_t col, uint8_t letter)
{
	unsigned char line;
	unsigned int value0;

	//test for valid inputs - row from 0 to 5
	//and cols from 0 to 9
	if ((row > 5) || (col > 9))
	{
		return;
	}

	//get the line based on the letter
	line = letter - 27;		//ie, for char 32 " ", it's on line 5
	value0 = (line-1) << 3;

	//set the page
	LCD_SetRow(row);

	//set the column address
	LCD_SetColumn(col << 3);

	//now, do 8 data writes
	LCD_WriteData(font_table[value0 + 0]);
	LCD_WriteData(font_table[value0 + 1]);
	LCD_WriteData(font_table[value0 + 2]);
	LCD_WriteData(font_table[value0 + 3]);
	LCD_WriteData(font_table[value0 + 4]);
	LCD_WriteData(font_table[value0 + 5]);
	LCD_WriteData(font_table[value0 + 6]);
	LCD_WriteData(font_table[value0 + 7]);

}

///////////////////////////////////////////
//write string at a given row location
//valid rows from 0 to 5, start at col zero
void LCD_WriteString(uint8_t row, const char* mystring)
{
	//only write the first 10 values in mystring
	uint8_t count = 0;

	while ((mystring[count] != '\0') && (count <= 10))
	{
		LCD_DrawChar(row, count, mystring[count]);
		count++;
	}
}

//////////////////////////////////////////////////
//Draw buffer to display starting at x offset
//0, row = row, num chars = len
void LCD_WriteStringLength(uint8_t row, char* buffer, uint8_t len)
{
	uint8_t i = 0;
	for (i = 0 ; i < len ; i++)
	{
		LCD_DrawChar(row, i, buffer[i]);
	}
}




