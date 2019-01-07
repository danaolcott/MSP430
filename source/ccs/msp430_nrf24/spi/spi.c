/*
 * SPIB:
 * P1.5 - UCA0CLK - serial clock
 * P1.6 - UCA0SOMI	- slave out, master in
 * P1.7 - UCA0SIMO - slave in master out
 * P2.4 - CS Pin
 *
*/

#include <stdint.h>
#include <msp430.h>
#include <msp430g2553.h>
#include "spi.h"


/////////////////////////////////////////////////
//SPI_init()
//Setup GPIO pins for SPIB.
//Configure Chip select, MOSI pin, MISO pin,
//
//Here's the pinout
// * SPIB:
// * P1.5 - UCA0CLK - serial clock
// * P1.6 - UCA0SOMI	- slave out, master in
// * P1.7 - UCA0SIMO - slave in master out
// * P2.4 - CS Pin
//
void SPI_init(SPISpeed_t speed)
{
	uint8_t lowByte = 0x00;
	uint8_t highByte = 0x00;

	switch(speed)
	{
		case SPI_SPEED_400KHZ:
		{
			lowByte = 0x28;
			highByte = 0x00;
			break;
		}
		case SPI_SPEED_1MHZ:
		{
			lowByte = 0x10;
			highByte = 0x00;
			break;
		}

		case SPI_SPEED_2MHZ:
		{
			lowByte = 0x08;
			highByte = 0x00;
			break;
		}

		case SPI_SPEED_4MHZ:
		{
			lowByte = 0x04;
			highByte = 0x00;
			break;
		}
	}


	//chip select - P2.4
	P2DIR |= SPI_CS_PIN;
	P2OUT |= SPI_CS_PIN;

    //SPI B setup routine.  Start by holding the
    //USCI in reset state while setting up registers
    UCB0CTL1 = UCSWRST;					//set to ini the SPI module

    //configure pins for SPIB -
    //P1.5, P1.6, P1.7
    //P1.5 - clock
    //P1.7 - MOSI
    //P1.6 - MISO

    P1SEL |= BIT5 | BIT6 | BIT7;
    P1SEL2 |= BIT5 | BIT6 | BIT7;

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

//    UCB0CTL0 |= UCCKPL + UCMSB + UCMST + UCSYNC;  // 3-pin, 8-bit SPI master

    //from spi setup from demo radio files
	UCB0CTL0  =  0x00+UCMST + UCSYNC + UCMODE_0 + UCMSB + UCCKPH;
    //clock control - USCI clock source. 1 - aclock, 2 smclock
    //3 smclock
    UCB0CTL1 |= UCSSEL_2;                     // SMCLK


    //bit rate control register - two registers
    //high bytes on BR1, low bytes on BR0.  no need to set
    //the BR1. seems to be up to 16 divider, so try 0x02, 0x04
    //0x08, etc.  In theory, this could slow it down so we can
    //run a higher clock speed but keep the bit generator slow

    UCB0BR0 = lowByte;                          //prescaler 8 - low bits
    UCB0BR1 = highByte;                              //prescaler - high bits

    //clear the bit to start the SPI module
    UCB0CTL1 &= ~UCSWRST;                     // **Initialize USCI state machine**

	//deselect the device
    SPI_deselect();

}



void SPI_select(void)
{
	P2OUT &=~ SPI_CS_PIN;
}

void SPI_deselect(void)
{
	P2OUT |= SPI_CS_PIN;
}



////////////////////////////////////
//transmit 1 byte with no cs control
uint8_t SPI_tx(uint8_t data)
{
    while (!(IFG2 & UCB0TXIFG));   // USCI_A0 TX buffer ready?

    //send high byte data
    UCB0TXBUF = data;
    while (!(IFG2 & UCB0TXIFG));   // USCI_A0 TX buffer ready?
    while (!(IFG2 & UCB0RXIFG));    // spi rx ready??
    while(UCB0STAT & UCBUSY);

    uint8_t readValue = UCB0RXBUF;	//read the incomming byte
    return readValue;

}


////////////////////////////////////
//read 1 byte with no cs control
uint8_t SPI_rx(void)
{
    while (!(IFG2 & UCB0TXIFG));   // USCI_A0 TX buffer ready?

    //dummy data - 0xFF
    UCB0TXBUF = 0xFF;
    while (!(IFG2 & UCB0TXIFG));   // USCI_A0 TX buffer ready?
    while (!(IFG2 & UCB0RXIFG));   // USCI_A0 TX buffer ready?
    while(UCB0STAT & UCBUSY);

    uint8_t readValue = UCB0RXBUF;	//read the incomming byte

    return readValue;
}




void SPI_write(uint8_t data)
{
	SPI_select();
	SPI_tx(data);
    SPI_deselect();
}

void SPI_writeArray(uint8_t* buffer, uint8_t length)
{
	uint8_t i = 0;

	SPI_select();

	for (i = 0 ; i < length ; i++)
		SPI_tx(buffer[i]);

	SPI_deselect();
}


uint8_t SPI_read(void)
{
	SPI_select();
	uint8_t data = SPI_rx();
    SPI_deselect();

    return data;
}




