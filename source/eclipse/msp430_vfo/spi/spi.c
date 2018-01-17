/*


 * SPI:
 * P1.1 - UCA0SOMI	- slave out, master in
 * P1.2 - UCA0SIMO - slave in master out
 * P1.4 - UCA0CLK - serial clock
 * P1.5 - chip select - config as general output

*/



#include <stdint.h>

#include <msp430.h>
#include "spi.h"


/////////////////////////////////////////////////
//SPI_init()
//Setup GPIO pins for SPIA.
//Configure Chip select, MOSI pin, MISO pin,
//
//Here's the pinout
//* P1.1 - UCA0SOMI	- slave out, master in
//* P1.2 - UCA0SIMO - slave in master out
//* P1.4 - UCA0CLK - serial clock
//* P1.5 - chip select - config as general output

void spi_init(void)
{
	//chip select - P1.5
	P1DIR |= SPI_CS_PIN;
	P1OUT |= SPI_CS_PIN;





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

	//deselect the device
    spi_deselect();
}


void spi_select(void)
{
	P1OUT &=~ SPI_CS_PIN;
}

void spi_deselect(void)
{
	P1OUT |= SPI_CS_PIN;
}

void spi_write(uint8_t data)
{
	spi_select();

    while (!(IFG2 & UCA0TXIFG));   // USCI_A0 TX buffer ready?

    //send high byte data
    UCA0TXBUF = data;
    while (!(IFG2 & UCA0TXIFG));   // USCI_A0 TX buffer ready?
    while(UCA0STAT & UCBUSY);

    spi_deselect();

}

void spi_writeData(uint16_t address, uint8_t data)
{
	//split the high and low bytes
	uint8_t lowByte = address & 0x00FF;
	uint8_t highByte = (address >> 8) & 0xFF;

	spi_select();

    while (!(IFG2 & UCA0TXIFG));   // USCI_A0 TX buffer ready?

    //load the tx buffer with a write command value
    UCA0TXBUF = 0xCC;
    while (!(IFG2 & UCA0TXIFG));   // USCI_A0 TX buffer ready?
    while(UCA0STAT & UCBUSY);

    //send high byte data
    UCA0TXBUF = highByte;
    while (!(IFG2 & UCA0TXIFG));   // USCI_A0 TX buffer ready?
    while(UCA0STAT & UCBUSY);

    //send low byte
    UCA0TXBUF = lowByte;
    while (!(IFG2 & UCA0TXIFG));   // USCI_A0 TX buffer ready?
    while(UCA0STAT & UCBUSY);

    //finally, send the data
    UCA0TXBUF = data;
    while (!(IFG2 & UCA0TXIFG));   // USCI_A0 TX buffer ready?
    while(UCA0STAT & UCBUSY);

    spi_deselect();

}

uint8_t spi_readData(uint16_t address)
{
	//split the high and low bytes
	uint8_t lowByte = address & 0x00FF;
	uint8_t highByte = (address >> 8) & 0xFF;

	spi_select();

    while (!(IFG2 & UCA0TXIFG));   // USCI_A0 TX buffer ready?

    //load the tx buffer with a read command
    //to set it to reading mode

    UCA0TXBUF = 0xFF;
    while (!(IFG2 & UCA0TXIFG));   // USCI_A0 TX buffer ready?
    while (!(IFG2 & UCA0RXIFG));   // USCI_A0 TX buffer ready?
    while(UCA0STAT & UCBUSY);

    //send high byte address
    UCA0TXBUF = highByte;
    while (!(IFG2 & UCA0TXIFG));   // USCI_A0 TX buffer ready?
    while (!(IFG2 & UCA0RXIFG));   // USCI_A0 TX buffer ready?
    while(UCA0STAT & UCBUSY);

    //send low byte
    UCA0TXBUF = lowByte;
    while (!(IFG2 & UCA0TXIFG));   // USCI_A0 TX buffer ready?
    while (!(IFG2 & UCA0RXIFG));   // USCI_A0 TX buffer ready?
    while(UCA0STAT & UCBUSY);

    //send dummy byte to generate clock
    UCA0TXBUF = 0x00;
    while (!(IFG2 & UCA0TXIFG));   // USCI_A0 TX buffer ready?
    while (!(IFG2 & UCA0RXIFG));   // USCI_A0 RX buffer ready?
    while(UCA0STAT & UCBUSY);

    uint8_t readValue = UCA0RXBUF;	//read the incomming byte

    spi_deselect();

	return readValue;
}



