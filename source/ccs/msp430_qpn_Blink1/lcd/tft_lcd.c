/*
Driver File for running the TFT LCD display
from adafruit on the msp 430 chip.  
1/26/15
*/

////////////////////////////////////////
//includes
#include "tft_lcd.h"
#include <msp430g2553.h> /* MSP430 variant used on MSP-EXP430G2 LaunchPad */

#include "font.h"
#include <string.h>

//////////////////////////////////////
//globals
uint16_t fontColor = WHITE;
uint16_t backColor = BLACK;

///////////////////////////////////////////
//function definitions
//SPIA_init()
//SPIA_init()
//set up the GPIO pins for SPI mode
//with one pin for the chip select
//Here's the pinout
//* P1.1 - UCA0SOMI	- slave out, master in
//* P1.2 - UCA0SIMO - slave in master out
//* P1.4 - UCA0CLK - serial clock
//* P1.5 - chip select - config as general output
// low clock polarity, data is sampled on the first edge
// DC low is command, DC high is data
// MSB first
void SPIA_init(void)
{
		//chip select - P1.5
	P1DIR |= CS_PIN;
	P1OUT |= CS_PIN;

	//reset pin - 2.0
	P2OUT |= RESET_PIN;		//high
	P2DIR |= RESET_PIN;		//output
	P2OUT |= RESET_PIN;		//make sure it's high

	//dc pin - p2.1
	P2OUT |= DC_PIN;		//high
	P2DIR |= DC_PIN;		//output
	P2OUT &=~ DC_PIN;		//make sure it's low for command



    //SPI A setup routine.  Start by holding the
    //USCI in reset state while setting up registers
    UCA0CTL1 = UCSWRST;					//set to ini the SPI module

    //configure pins for SPIA
    P1SEL |= BIT1 | BIT2 | BIT4;
    P1SEL2 |= BIT1 | BIT2 | BIT4;

    //set all the registers while the UCSWRST bit is high
    //see page 445 of the programmers manual
    //UCCKPH - high - data captured on the first edge
    //UCCKPL - high - inactive clock is high
    //USCMSB - high - MSB first
    //UC7BIT - high - 7 bit, low - 8 bit
    //UCMST - high - master mode
    //UCMODEx - not set - 3 wire, output only - 2 bit value
    //UCSYNC - high - synchronous mode, low is assync mode


    UCA0CTL0 |= UCMSB + UCMST + UCCKPH + UCSYNC;  // 3-pin, 8-bit SPI master
//    UCA0CTL0 |= UCMSB + UCMST + UCSYNC;  // 3-pin, 8-bit SPI master

    //clock control - USCI clock source. 1 - aclock, 2 smclock
    //3 smclock
    UCA0CTL1 |= UCSSEL_2;                     // SMCLK

    //bit rate control register - two registers
    //high bytes on BR1, low bytes on BR0.  no need to set
    //the BR1. seems to be up to 16 divider, so try 0x02, 0x04
    //0x08, etc.  In theory, this could slow it down so we can
    //run a higher clock speed but keep the bit generator slow
    UCA0BR0 |= 0x02;                          //prescaler  2
    UCA0BR1 = 0;                              //prescaler - high bits
    UCA0MCTL = 0;                             // No modulation, not sure ??

    //clear the bit to start the SPI module
    UCA0CTL1 &= ~UCSWRST;                     // **Initialize USCI state machine**

	P1OUT |= CS_PIN;			//chip select - high
	P2OUT &=~ DC_PIN;			//command

	//reset the LCD
	P2OUT &=~ RESET_PIN;
	delay_us(1000);
	P2OUT |= RESET_PIN;

}

///////////////////////////////////////
//SPIA_WriteCommand()
void SPIA_WriteCommand(uint8_t reg)
{
	P1OUT &=~ CS_PIN;		//select device
	P2OUT &=~ DC_PIN;		//command

    while (!(IFG2 & UCA0TXIFG));   // USCI_A0 TX buffer ready?

    UCA0TXBUF = reg;
    while (!(IFG2 & UCA0TXIFG));   // USCI_A0 TX buffer ready?
    while(UCA0STAT & UCBUSY);

	P1OUT |= CS_PIN;		//unselect device
	P2OUT &=~ DC_PIN;		//command

}

/////////////////////////////////////
//SPIA_WriteData()
void SPIA_WriteData(uint8_t data)
{
	P1OUT &=~ CS_PIN;		//select device
	P2OUT |= DC_PIN;		//data

    while (!(IFG2 & UCA0TXIFG));   // USCI_A0 TX buffer ready?

    UCA0TXBUF = data;
    while (!(IFG2 & UCA0TXIFG));   // USCI_A0 TX buffer ready?
    while(UCA0STAT & UCBUSY);

	P1OUT |= CS_PIN;		//unselect device
	P2OUT &=~ DC_PIN;		//command
}


/////////////////////////////////////
//LCD_Clear
void LCD_Clear(uint16_t color)
{
	uint8_t top = (uint8_t)(rgb2bgr(color) >> 8);
	uint8_t bottom = (uint8_t)(rgb2bgr(color) & 0xFF);

	SPIA_Write32(0x2A, 0x00, 0x00, 0x00, 0x7F);		//column address set xs = 0 and xe = 127
	SPIA_Write32(0x2B, 0x00, 0x00, 0x00, 0x9F);		//row address set ys = 0, ye = 159

	uint8_t i, j;

	//set to ram write - 2c.  column and row start and stop
	//registers are assumed to be set prior to this.  probably
	//should set these again here.  might try to set just a
	//small area.
	SPIA_WriteCommand(0x2C);

	for (i = 0 ; i < 160 ; i++)
	{
		for (j = 0 ; j < 128 ; j++)
		{
			SPIA_WriteData(top);
			SPIA_WriteData(bottom);

		}
	}

}


///////////////////////////////////////
//LCD_PutPixel()
void LCD_PutPixel(uint8_t x, uint8_t y, uint16_t color)
{
	//simple test for valid input x and y
	if ((x > 127) || (y > 159))
	{
		return;
	}

	//set up the registers for x and y start and stop
	SPIA_Write32(0x2A, 0x00, x, 0x00, x);		//col address, x
	SPIA_Write32(0x2B, 0x00, y, 0x00, y);		//row address, y

	uint8_t top = (uint8_t)(rgb2bgr(color) >> 8);
	uint8_t bottom = (uint8_t)(rgb2bgr(color) & 0xFF);

	SPIA_Write16(0x2C, top, bottom);
}

////////////////////////////////////////
//LCD_DrawLine
void LCD_DrawLine(int16_t x0, int16_t y0, int16_t x1, int16_t y1, uint16_t color)
{
	  int16_t steep = abs(y1 - y0) > abs(x1 - x0);
	  if (steep)
	  {
		swap(x0, y0);
		swap(x1, y1);
	  }

	  if (x0 > x1)
	  {
		swap(x0, x1);
		swap(y0, y1);
	  }

	  int16_t dx, dy;
	  dx = x1 - x0;
	  dy = abs(y1 - y0);

	  int16_t err = dx / 2;
	  int16_t ystep;

	  if (y0 < y1)
	  {
		ystep = 1;
	  }

	  else
	  {
		ystep = -1;
	  }

	  for (; x0<=x1; x0++) {
		if (steep)
		{
		  LCD_PutPixel(y0, x0, color);
		}

		else
		{
		  LCD_PutPixel(x0, y0, color);
		}

		err -= dy;
		if (err < 0)
		{
		  y0 += ystep;
		  err += dx;
		}
	  }
}



//////////////////////////////////////////////
//LCD_DrawStringx
//The purpose of this fucntion is to draw a
//string all at once using a single drawing
//window.  that way, we can add kerning to the
//string.
void LCD_DrawStringx(uint8_t row, char* mystring, uint8_t kern, uint16_t font, uint16_t back)
{
	//set the font color and back color global variables
	fontColor = font;
	backColor = back;

	//next, get the length of the string.  we'll
	//need this to build the size of our drawing window
	uint8_t numChars = strlen(mystring);
	uint8_t charWidth = 8;
	uint8_t charHeight = 8;

	//set up the drawing space
	uint8_t windowLength = numChars * (8 + kern);	//kern in pixels
	windowLength = windowLength - (windowLength % 8) + 8;	//round up to nearest 8

	uint8_t windowRemainder = windowLength - (numChars * (8 + kern));	//draw at end of each line

	uint8_t startX = 1;
	uint8_t stopX = startX + windowLength - 1;
	uint8_t startY = row * 8;
	uint8_t stopY = startY + 7;

	//set up the drawing window registers
	SPIA_Write32(0x2A, 0x00, startX, 0x00, stopX);		//col address, x1 and x2
	SPIA_Write32(0x2B, 0x00, startY, 0x00, stopY);		//row address, y1 and y2

	//all further writes go directly to ram....
	SPIA_WriteCommand(0x2C);		//ram set

	//set up the colors
	uint8_t topFont = (uint8_t)(rgb2bgr(fontColor) >> 8);
	uint8_t bottomFont = (uint8_t)(rgb2bgr(fontColor) & 0xFF);
	uint8_t topBack = (uint8_t)(rgb2bgr(backColor) >> 8);
	uint8_t bottomBack = (uint8_t)(rgb2bgr(backColor) & 0xFF);


	//for each line (ie , top to bottom) in a character
	//and for each character in the string, do the following:

	//get the letter bits from the font table
	//for each bit, write font color or back color
	//depending on if it's 1 or 0.  1 = font, 0 = back
	//go to next letter.
	//repeat.
	//at end of string, write windowRemainder pixels
	//to complete the drawing line.
	//go back to the beginning of the string, this time
	//with a line offset++ and repeat until the entire
	//drawing window is filled.

    uint8_t p = 8, bit = 0;
    uint8_t i = 0, j = 0, k = 0, m = 0;


	for (i = 0 ; i < charHeight ; i++)
	{

		for (j = 0 ; j < numChars ; j++)
		{
			//get the bits for the character out of mystring
		    uint8_t line = mystring[j] - (uint8_t)32;	//each char is on it's own line

		    //i is the pixel line offset in the font table
		    //the value 8 is the width of each character
		    uint8_t temp = font_table[(line*charWidth)+i];

	        p = 8;                                           //pixels on the line
	        bit = 0;                                          //test if color or no color
	        while (p > 0)
	        {
	            bit = ((1u << (p-1)) & temp) >> (p-1);      //test each of 8 pixels on a single line

	            //if the bit = 0 - it's blank               //background
	            //if the bit = 1 - it's a color             //font
	            if (bit == 1)
	            {
	    			SPIA_WriteData(topFont);
	    			SPIA_WriteData(bottomFont);
	            }
	            else
	            {
	    			SPIA_WriteData(topBack);
	    			SPIA_WriteData(bottomBack);

	            }

	            p--;
	        }

	        //write the kern
	        for (m = 0 ; m < kern ; m++)
	        {
				SPIA_WriteData(topBack);
	    		SPIA_WriteData(bottomBack);
	        }
		}

		//write windowRemainder pixels
		for (k = 0 ; k < windowRemainder ; k++)
		{
			SPIA_WriteData(topBack);
    		SPIA_WriteData(bottomBack);
		}
	}

}









///////////////////////////////////////
//LCD_DrawChar88() - character
//at a given row and col
void LCD_DrawChar88(uint8_t row, uint8_t col, uint8_t letter)
{
	if ((row > 20) || (col > 16))
	{
		return;
	}

	//set the start and stop x and y values
	uint8_t x1 = col * 8;
	uint8_t x2 = x1 + 7;
	uint8_t y1 = row * 8;
	uint8_t y2 = y1 + 7;

	SPIA_Write32(0x2A, 0x00, x1, 0x00, x2);		//col address, x1 and x2
	SPIA_Write32(0x2B, 0x00, y1, 0x00, y2);		//row address, y1 and y2

	SPIA_WriteCommand(0x2C);		//ram set

	//get the values from the font table

    uint8_t line = letter - (uint8_t)32;
    uint8_t p = 8, bit = 0;
    uint8_t i = 0;

	uint8_t topFont = (uint8_t)(rgb2bgr(fontColor) >> 8);
	uint8_t bottomFont = (uint8_t)(rgb2bgr(fontColor) & 0xFF);

	uint8_t topBack = (uint8_t)(rgb2bgr(backColor) >> 8);
	uint8_t bottomBack = (uint8_t)(rgb2bgr(backColor) & 0xFF);


    for (i = 0 ; i < 8 ;  i++)
    {
        uint8_t temp = font_table[(line*8)+i];

        p = 8;                                           //pixels on the line
        bit = 0;                                          //test if color or no color
        while (p > 0)
        {
            bit = ((1u << (p-1)) & temp) >> (p-1);      //test each of 8 pixels on a single line

            //if the bit = 0 - it's blank               //background
            //if the bit = 1 - it's a color             //font
            if (bit == 1)
            {
    			SPIA_WriteData(topFont);
    			SPIA_WriteData(bottomFont);
            }
            else
            {
    			SPIA_WriteData(topBack);
    			SPIA_WriteData(bottomBack);

            }

            p--;
        }
    }

}

//////////////////////////////////////////
//whole string
void LCD_DrawString(uint8_t row, char* temp, uint16_t font, uint16_t back)
{
	//set the font color and back color
	//global variables
	fontColor = font;
	backColor = back;

	uint8_t i = 0;

	while ((temp[i] != '\0') && (i < 20))
	{
		LCD_DrawChar88(row, i, temp[i]);
		i++;
	}
}


////////////////////////////////////////
//flip the red and blue
uint16_t rgb2bgr(uint16_t color)
{
    uint16_t  r, g, b, rgb;

    b = ( color>>(uint16_t)0 )  & 0x1f;
    g = ( color>>(uint16_t)5 )  & 0x3f;
    r = ( color>>(uint16_t)11 ) & 0x1f;

    rgb =  (b<<(uint16_t)11) + (g<<(uint16_t)5) + (r<<(uint16_t)0);

    return( rgb );


}

////////////////////////////////////////
//write reg + 1 byte
void SPIA_Write8(uint8_t reg, uint8_t value1)
{
	P1OUT &=~ CS_PIN;		//select device
	P2OUT &=~ DC_PIN;		//command first

	delay_us(1);			//small delay

    while (!(IFG2 & UCA0TXIFG));   // USCI_A0 TX buffer ready?

    UCA0TXBUF = reg;
    while (!(IFG2 & UCA0TXIFG));   // USCI_A0 TX buffer ready?
    while(UCA0STAT & UCBUSY);

    //not sure exactly where this goes, but the
    //DC pin is sampled at the 8th clock cycle, and
    //it should be uninterrupted, so set it high here
    //might have to remove some of those while statements

    P2OUT |= DC_PIN;				//DC high = data

    UCA0TXBUF = value1;
    while (!(IFG2 & UCA0TXIFG));   // USCI_A0 TX buffer ready?
    while(UCA0STAT & UCBUSY);

	delay_us(1);
	P1OUT |= CS_PIN;		//unselect device
	P2OUT &=~ DC_PIN;		//back to command - 0
}

//////////////////////////////////////
//write reg + 2 bytes
void SPIA_Write16(uint8_t reg, uint8_t value1, uint8_t value2)
{
	P1OUT &=~ CS_PIN;		//select device
	P2OUT &=~ DC_PIN;		//command
	//make a really short delay

	delay_us(1);

    while (!(IFG2 & UCA0TXIFG));   // USCI_A0 TX buffer ready?

    UCA0TXBUF = reg;
    while (!(IFG2 & UCA0TXIFG));   // USCI_A0 TX buffer ready?
    while(UCA0STAT & UCBUSY);

    P2OUT |= DC_PIN;				//data

    UCA0TXBUF = value1;
    while (!(IFG2 & UCA0TXIFG));   // USCI_A0 TX buffer ready?
    while(UCA0STAT & UCBUSY);

    UCA0TXBUF = value2;
    while (!(IFG2 & UCA0TXIFG));   // USCI_A0 TX buffer ready?
    while(UCA0STAT & UCBUSY);

	delay_us(1);
	P1OUT |= CS_PIN;		//unselect device
	P2OUT &=~ DC_PIN;		//back to command	
}

//////////////////////////////////////////
//write reg + 3 bytes
void SPIA_Write24(uint8_t reg, uint8_t value1, uint8_t value2, uint8_t value3)
{
	P1OUT &=~ CS_PIN;		//select device
	P2OUT &=~ DC_PIN;		//command
	//make a really short delay

	delay_us(1);

    while (!(IFG2 & UCA0TXIFG));   // USCI_A0 TX buffer ready?

    UCA0TXBUF = reg;
    while (!(IFG2 & UCA0TXIFG));   // USCI_A0 TX buffer ready?
    while(UCA0STAT & UCBUSY);

    P2OUT |= DC_PIN;				//data

    UCA0TXBUF = value1;
    while (!(IFG2 & UCA0TXIFG));   // USCI_A0 TX buffer ready?
    while(UCA0STAT & UCBUSY);

    UCA0TXBUF = value2;
    while (!(IFG2 & UCA0TXIFG));   // USCI_A0 TX buffer ready?
    while(UCA0STAT & UCBUSY);

    UCA0TXBUF = value3;
    while (!(IFG2 & UCA0TXIFG));   // USCI_A0 TX buffer ready?
    while(UCA0STAT & UCBUSY);


	delay_us(1);
	P1OUT |= CS_PIN;		//unselect device
	P2OUT &=~ DC_PIN;		//back to command
}

//////////////////////////////////////////
//write reg + 4 bytes
void SPIA_Write32(uint8_t reg, uint8_t value1, uint8_t value2, uint8_t value3, uint8_t value4)
{
	P1OUT &=~ CS_PIN;		//select device
	P2OUT &=~ DC_PIN;		//command

	delay_us(1);

    while (!(IFG2 & UCA0TXIFG));   // USCI_A0 TX buffer ready?

    UCA0TXBUF = reg;
    while (!(IFG2 & UCA0TXIFG));   // USCI_A0 TX buffer ready?
    while(UCA0STAT & UCBUSY);

    //switch to data
    P2OUT |= DC_PIN;


    UCA0TXBUF = value1;
    while (!(IFG2 & UCA0TXIFG));   // USCI_A0 TX buffer ready?
    while(UCA0STAT & UCBUSY);

    UCA0TXBUF = value2;
    while (!(IFG2 & UCA0TXIFG));   // USCI_A0 TX buffer ready?
    while(UCA0STAT & UCBUSY);

    UCA0TXBUF = value3;
    while (!(IFG2 & UCA0TXIFG));   // USCI_A0 TX buffer ready?
    while(UCA0STAT & UCBUSY);

    UCA0TXBUF = value4;
    while (!(IFG2 & UCA0TXIFG));   // USCI_A0 TX buffer ready?
    while(UCA0STAT & UCBUSY);

	delay_us(1);
	P1OUT |= CS_PIN;		//unselect device
	P2OUT &=~ DC_PIN;		//back to command

}

/////////////////////////////////////////
//LCD_init()
void LCD_init(void)
{
		//reset the LCD
	P2OUT |= RESET_PIN;
	delay_us(1200);
	P2OUT &=~ RESET_PIN;
	delay_us(1200);
	P2OUT |= RESET_PIN;
	delay_us(1200);

	//commands for setting up the lcd.  This sequence was found
	//from some site that sells these displays with the ST7735 controller

	SPIA_WriteCommand(0x11);		//take out of sleep mode
	delay_us(1000);

	//Frame rate - reg B1, B2, B3
	SPIA_Write24(0xB1, 0x01, 0x2C, 0x2D);	//frame rate, front porch, back porch, etc
	SPIA_Write24(0xB2, 0x01, 0x2C, 0x2D);	//same, but in idle mode
	SPIA_Write24(0xB3, 0x01, 0x2C, 0x2D);	//same, but in partial mode, full color

	//column inversion
	SPIA_Write8(0xB4, 0x07);				//frame inversion for each mode

	//power on sequence
	SPIA_Write24(0xC0, 0xA2, 0x02, 0x84);	//
	SPIA_Write8(0xC1, 0xC5);
	SPIA_Write16(0xC2, 0x0A, 0x00);
	SPIA_Write16(0xC3, 0x8A, 0x2A);
	SPIA_Write16(0xC4, 0x8A, 0xEE);

	SPIA_Write8(0xC5, 0x0E);				//VCOM
	SPIA_Write8(0x36, 0xC8);				//Mx, My, RBG mode

	//gamma sequence - a bunch of writes here
	SPIA_WriteCommand(0xE0);
	SPIA_WriteData(0x0F);
	SPIA_WriteData(0x1A);
	SPIA_WriteData(0x0F);
	SPIA_WriteData(0x18);
	SPIA_WriteData(0x2F);
	SPIA_WriteData(0x28);
	SPIA_WriteData(0x20);
	SPIA_WriteData(0x22);
	SPIA_WriteData(0x1F);
	SPIA_WriteData(0x1B);
	SPIA_WriteData(0x23);
	SPIA_WriteData(0x37);
	SPIA_WriteData(0x00);
	SPIA_WriteData(0x07);
	SPIA_WriteData(0x02);
	SPIA_WriteData(0x10);

	SPIA_WriteCommand(0xE1);
	SPIA_WriteData(0x0F);
	SPIA_WriteData(0x1B);
	SPIA_WriteData(0x0F);
	SPIA_WriteData(0x17);
	SPIA_WriteData(0x33);
	SPIA_WriteData(0x2C);
	SPIA_WriteData(0x29);
	SPIA_WriteData(0x2E);
	SPIA_WriteData(0x30);
	SPIA_WriteData(0x30);
	SPIA_WriteData(0x39);
	SPIA_WriteData(0x3F);
	SPIA_WriteData(0x00);
	SPIA_WriteData(0x07);
	SPIA_WriteData(0x03);
	SPIA_WriteData(0x10);

	SPIA_Write32(0x2A, 0x00, 0x00, 0x00, 0x7F);		//column address set xs = 0 and xe = 127
	SPIA_Write32(0x2B, 0x00, 0x00, 0x00, 0x9F);		//row address set ys = 0, ye = 159

	SPIA_Write8(0xF0, 0x01);		//enable test command
	SPIA_Write8(0xF6, 0x00);		//disable ram power save mode

	SPIA_Write8(0x3A, 0x05);		//65k mode


	//add writes to registers for read back project code, LCD code, etc
	//ID2 and ID3 in registers 0xD1 and 0xD2, stores values in EEPROM

	//read these back using registers 0xDB for ID2
	//and 0xDC for ID3
	SPIA_Write8(0xD1, 0x55); 	//bits 6, 4, 2, 0 high (ID26, ID24, ID22, ID20)
	SPIA_Write8(0xD2, 0xCC); 	//bits 7, 6, 3, 2 (ID37, ID36, ID33, ID32)

	SPIA_WriteCommand(0x29);		//display on
	LCD_Clear(0x0000);				//black

}


///////////////////////////////////////
//delay_us
void delay_us(volatile int ticks)
{
	int value = ticks;
	while (value >0)
	{
		value--;

	}	
}

