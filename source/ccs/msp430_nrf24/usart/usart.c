/*
MSP430 UART Driver File
Assumes the following:
clock = 16mhz
baud rate = 9600
uses uart A connected to the ftdi chip (ie,
it has to be configured to 9600

Settings (Table 15-4)
UCBRx = 1666
UCBRSx = 6
UCBRSx = 0

UCA0BR0 = 1666 & 0xFF;
UCA0BR1 = (1666 >> 8) & 0xFF;
UCA0MCTL = UCBRS2 + UCBRS1;     //bits 2 and 1 = 6


*/

#include <msp430.h>
#include <msp430g2553.h>
#include <stdint.h>
#include <string.h>

#include "usart.h"




//UART variables
#define RX_BUFFER_SIZE	32
static volatile uint8_t rxIndex = 0;
static volatile uint8_t rxActiveBuffer = 1;
volatile uint8_t rxBuffer1[RX_BUFFER_SIZE];		//read buffer for UART
volatile uint8_t rxBuffer2[RX_BUFFER_SIZE];		//read buffer for UART



//////////////////////////////////////
//prototypes for uart

//setup the USART pins for tx / rx
//P1.1 is RX, P1.2 is tx

//use the following settings for 9600
//clock = 1mhz
//	UCA0BR0 = 104;				//low bits 1 mhz clock and 9600 baud
//	UCA0BR1 = 0;				//high bits 1 mhz clock and 9600 baud
//	UCA0MCTL = UCBRS0;			//modulation UCBRSx = 1

//use the following settings for 230400 baud
//use an external ftdi chip
//UCA0BR0 = 833;				//low bits 16 mhz clock and 230400 baud
//UCA0BR1 = 0;				//high bits 16 mhz clock and 230400 baud
//UCA0MCTL = UCBRS2 + UCBRS1 + UCBRS0;			//modulation UCBRSx = 7

//for 16mhz clock and 9600 baud
//UCA0BR0 = 1666 & 0xFF;
//UCA0BR1 = (1666 >> 8) & 0xFF;
//UCA0MCTL = UCBRS2 + UCBRS1;     //bits 2 and 1 = 6


void usart_init(void)
{
	//init the buffers
	rxIndex = 0;
	rxActiveBuffer = 1;
	memset(rxBuffer1, 0x00, RX_BUFFER_SIZE);
	memset(rxBuffer2, 0x00, RX_BUFFER_SIZE);

	//configure special function for 1.1 and 1.2
	P1SEL = BIT1 | BIT2;
	P1SEL2 = BIT1 | BIT2;

	UCA0CTL1 |= UCSWRST;			//put UART into reset config
	UCA0CTL1 |= UCSSEL_2;		//config serial IO - use SMCLK

    //configure the bit rate - Table 15-4
    //for 16mhz clock and 9600 baud:
    //UCOS16 = 0 (no over sampling, UCA0MCTL register)
    UCA0BR0 = 1666 & 0xFF;
    UCA0BR1 = (1666 >> 8) & 0xFF;
    UCA0MCTL = UCBRS2 + UCBRS1;     //bits 2 and 1 = 6

	UCA0CTL1 &=~ UCSWRST;			//take it out of reset mode
	IE2 |= UCA0RXIE;				//enable rx interrupts
}



/////////////////////////////////////////
//write one byte over the usart
void usart_writeByte(uint8_t value)
{
    while (!(IFG2&UCA0TXIFG));                // USCI_A0 TX buffer ready?
    UCA0TXBUF = value;
    while (!(IFG2&UCA0TXIFG));                // USCI_A0 TX buffer ready?
}


/////////////////////////////////////
//SerialWriteString
//used for transmitting just a string with
//unknown size.  does not clear a buffer
void usart_writeString(uint8_t* buffer)
{
	uint8_t i = 0;

	while ((buffer[i] != '\0') && (i < 40))
	{
		//write out each char in the buffer
	    while (!(IFG2&UCA0TXIFG));                // USCI_A0 TX buffer ready?
	    UCA0TXBUF = buffer[i];
	    i++;
	}
}


/////////////////////////////////////
//SerialWriteStringBytes
//used for transmitting the rx buffer
//back over the serial port - write the buffer
//and clear it
void usart_writeStringLength(uint8_t* buffer, uint8_t size)
{
	uint8_t i = 0;
	for (i = 0 ; i < size ; i++)
	{
	    while (!(IFG2&UCA0TXIFG));		// USCI_A0 TX buffer ready?
	    UCA0TXBUF = buffer[i];			//put into tx register
	}
}




/////////////////////////////////////
//Duplicate Functions - For Compatabilty
void UART_sendString(uint8_t* buffer)
{
	uint8_t i = 0;

	while ((buffer[i] != '\0') && (i < 40))
	{
		//write out each char in the buffer
	    while (!(IFG2&UCA0TXIFG));                // USCI_A0 TX buffer ready?
	    UCA0TXBUF = buffer[i];
	    i++;
	}

}


/////////////////////////////////////
//Duplicate Function - Included for compatibilty
void UART_sendStringLength(uint8_t* buffer, uint8_t size)
{
	uint8_t i = 0;
	for (i = 0 ; i < size ; i++)
	{
		//write out each char in the buffer
	    while (!(IFG2&UCA0TXIFG));                // USCI_A0 TX buffer ready?

	    UCA0TXBUF = buffer[i];
	}

}


//////////////////////////////////////
//ProcessCommand
//For now, echo the data
void usart_processCommand(uint8_t* buffer, uint8_t len)
{
	//temporarily disable the rx interrupts
	IE2 &=~ UCA0RXIE;

    //echo the message
	usart_writeString("RX: ");
	usart_writeStringLength(buffer, len);

	//reenable the rx interrupts
	IE2 |= UCA0RXIE;
}




//////////////////////////////////////////
//UART interrupt - RX
//check which buffer is active, put char
//into active buffer, test if the command 
//is complete, call process command, flip 
//clear non-active buffer, flip active buffer
//
#pragma vector=USCIAB0RX_VECTOR
__interrupt void USCI0RX_ISR(void)
{
    while (!(IFG2&UCA0TXIFG));      // USCI_A0 TX buffer ready?

    char t = UCA0RXBUF;				//read the char to clear the flag

    //test the buffer
    if (rxIndex < (RX_BUFFER_SIZE - 1))
    {
        if (rxActiveBuffer == 1)
        {
        	rxBuffer1[rxIndex] = t;
        	rxIndex++;
        }

        else
        {
        	rxBuffer2[rxIndex] = t;
        	rxIndex++;
        }

        //end of command?
        if (t == '\n')
        {
            if (rxActiveBuffer == 1)
            {
            	rxBuffer1[rxIndex] = 0x00;	//terminate
            	memset((uint8_t*)rxBuffer2, 0x00, RX_BUFFER_SIZE);
            	usart_processCommand((uint8_t*)rxBuffer1, rxIndex);
            	rxActiveBuffer = 2;
            	rxIndex = 0x00;
            }

            else
            {
            	rxBuffer2[rxIndex] = 0x00;	//terminate
            	memset((uint8_t*)rxBuffer1, 0x00, RX_BUFFER_SIZE);
            	usart_processCommand((uint8_t*)rxBuffer2, rxIndex);
            	rxActiveBuffer = 1;
            	rxIndex = 0x00;
            }
        }
    }

    else
    {
    	//buffer is full - reset the counter and clear the buffer
    	rxIndex = 0x00;
    	rxActiveBuffer = 1;
    	memset((uint8_t*)rxBuffer1, 0x00, RX_BUFFER_SIZE);
    	memset((uint8_t*)rxBuffer2, 0x00, RX_BUFFER_SIZE);
    }
}












