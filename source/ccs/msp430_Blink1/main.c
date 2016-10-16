/*
 * 4/14/16
 * MSP430 Example
 *
 * This example contains the following:
 * clock 16mhz
 * UartA configured at 230400 - tx/rx on P1.2 and P1.1
 *
 * Note: to use the uart at this speed, you'll need to
 * use an external FTDI.  the onboard uart/serial only
 * supports up to 9600
 *
 * Simple command line flag in the main loop
 *
 * SPI Interface - SPIB
 * SPIB uses the following pins:
 * P1.5 - clock
 * P1.6 - SOMI - slave out master in - MISO
 * P1.7 - SIMO - slave in, master out - MOSI
 *
 *
 *
 *
 */
//includes
#include <msp430g2553.h>
#include <stdio.h>
#include <stdint.h>
#include <stddef.h>
#include <string.h>

//clock and baud rate defines
//uncomment out the following
//to use clock speed 16mhz and baud rate = 230000
//for this speed, you cant use the onboard usart / usb
//you have to use external ftdi / bluetooth, etc
//if not defined, the speed is 1mhz, the baud rate is 9600
//#define CLK16_BAUD230000	1


//pin defines
#define CS_PIN		BIT4


//prototypes
void delay_ms(volatile int ticks);
void TimeDelay_Decrement(void);
void GPIO_init(void);
void TimerA_init(void);
void Interrupt_init(void);
void LED_RED_TOGGLE(void);
void LED_GREEN_TOGGLE(void);

//prototypes for uart
void UART_init(void);				//setup for SPI
void SerialWriteString(char * buffer);
void SerialWriteStringBytes(char buffer[], uint8_t size);
void ProcessCommand();
void printMenu();

//prototypes for SPIB
void SPI_init(void);
void SPI_select(void);
void SPI_deselect(void);

void SPI_Write(uint8_t);
void SPI_WriteData(uint16_t address, uint8_t data);	//write data to address
uint8_t SPI_ReadData(uint16_t address);				//read data at address

//global variables
static volatile int TimeDelay;

//UART variables
#define RX_BUFFER_SIZE	50
#define PRINT_BUFFER_SIZE 50
static int rxIndex = 0;
char rxBuffer[RX_BUFFER_SIZE];		//read buffer for UART
static uint8_t rxFlag;	//flag indicating the buffer is done
char printBuffer[PRINT_BUFFER_SIZE];


//main program
void main(void)
{
	//disable the watchdog timer
	WDTCTL = WDTPW + WDTHOLD;
	
	TimerA_init();
	GPIO_init();
	Interrupt_init();
	UART_init();
	SPI_init();



	//main loop
	while(1)
	{
		LED_RED_TOGGLE();
		delay_ms(10);

		//test the SPI functions
		SPI_Write(0xAA);
		SPI_WriteData(0xAA88, 0x11);

		//test the rx flag for incoming data
		if (rxFlag == 1)
		{
			//there's a buffer available for processing
			//so process it here.
			//SerialWriteStringBytes(rx_buffer, rx_index);//echo the string back
			ProcessCommand();		//process command and clear rx_buffer
		}

	}

}


////////////////////////////////////////
void delay_ms(volatile int ticks)
{
	TimeDelay = ticks;

	//TimeDelay is static and gets decremented
	//in the TimerA ISR
	while (TimeDelay !=0);
}


/////////////////////////////////////////
//Function called from the TimerA ISR
void TimeDelay_Decrement(void)
{
	if (TimeDelay != 0x00)
	{
		TimeDelay--;
	}

}

//////////////////////////////////////
//Note: SPIB conflicts with P1.6, so the green led does not work
void GPIO_init(void)
{
	//setup bit 0 and 6 as output

	P1DIR |= BIT0;
	P1DIR |= BIT6;

	P1OUT &=~ BIT0;		//turn off
	P1OUT &=~ BIT6;		//turn off

	//set up the user button bit 3
	//as input with pull up enabled
	P1DIR &=~ BIT3;		//input
	P1REN |= BIT3;		//enable pullup/down
	P1OUT |= BIT3;		//resistor set to pull up



}

/////////////////////////////////////////
//Sets up the timer And the frequency
//of the clock, since the rate of overflow
//is connected to this

void TimerA_init(void)
{

	//set the clock frequency to 16 mhz
	//use this when uart at 230400
#ifdef CLK16_BAUD230000
	BCSCTL1 = CALBC1_16MHZ;
	DCOCTL = CALDCO_16MHZ;
#else
	//use this with uart at 9600
	BCSCTL1 = CALBC1_1MHZ;
	DCOCTL = CALDCO_1MHZ;
#endif

	//Set up Timer A register TACTL
	TACTL = 0x0000;		//start from all all clear
	TACTL |= BIT9;		//use SMCLK as the source
	TACTL |= BIT7;		//use prescaler 8
	TACTL |= BIT6;
	TACTL |= BIT4;		//count up to TACCR0(set below)
	TACTL |= BIT1;		//enable the interrupt
	TACTL &=~ BIT0;		//clear all pending interrupts

	//set the countup value.  for 1ms delay,
	//the countup value depends on the clock freq.

	//1 mhz or 16
#ifdef CLK16_BAUD230000
	TACCR0 = 2000;		//use 2000 for 16 mhz
#else
	TACCR0 = 125;		//use 125 for  1 mhz
#endif
	//	TACCR0 = 1000;		//use 1000 for 8 mhz

	//TACCTL0 - compare capture control register.
	//this has to be set up along with the timer interrupt
	//bit 4 enables the interrupts for this register
	//also called CCIE

	TACCTL0 = CCIE;		//compare, capture interrupt enable

}

///////////////////////////////////////////
void Interrupt_init(void)
{
	//enable all the interrupts
	__bis_SR_register(GIE);

	P1IE |= BIT3;		//enable button interrupt
	P1IFG &=~ BIT3;		//clear the flag

}

///////////////////////////////////////////
void LED_RED_TOGGLE(void)
{
	P1OUT ^= BIT0;
}

///////////////////////////////////////////
void LED_GREEN_TOGGLE(void)
{
	P1OUT ^= BIT6;
}


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

void UART_init(void)
{
	//configure special function for 1.1 and 1.2
	//using the P1SEL register
	P1SEL = BIT1 | BIT2;
	P1SEL2 = BIT1 | BIT2;

	//put the UART in reset for configuration
	UCA0CTL1 |= UCSWRST;

	//configure the serial IO
	UCA0CTL1 |= UCSSEL_2;		//use SMCLK


#ifdef CLK16_BAUD230000
	//230400 baud and 16mhz
	UCA0BR0 = (uint8_t)833;
	UCA0BR1 = 0;
	UCA0MCTL = UCBRS2 + UCBRS1 + UCBRS0;			//modulation UCBRSx = 7
#else
	//9600 baud and 1 mhz clock
	UCA0BR0 = 104;				//low bits 1 mhz clock and 9600 baud
	UCA0BR1 = 0;				//high bits 1 mhz clock and 9600 baud
	UCA0MCTL = UCBRS0;			//modulation UCBRSx = 1
#endif




	//take it out of reset mode
	UCA0CTL1 &=~ UCSWRST;

	//enable the USCI_A0 Rx interrupt
	//this get's disabled in process command on entry
	//and re-enabled when exiting process command
	IE2 |= UCA0RXIE;

}



//////////////////////////////////////
//ProcessCommand
//process the incomming data and write out something
//use string compare functiosn with buffer
void ProcessCommand()
{
	//temporarily disable the rx interrupts
	IE2 &=~ UCA0RXIE;

	if (!strcmp(rxBuffer, "?\n"))
	{
		//print out the menu list of commands
		printMenu();
	}

	//digital pins - P1.0 and P1.6
	//setD0, resetD0, toggleD0
	else if (!strcmp(rxBuffer, "setD0\n"))
	{
		P1OUT |= BIT0;
	}
	else if (!strcmp(rxBuffer, "resetD0\n"))
	{
		P1OUT &=~ BIT0;
	}
	else if (!strcmp(rxBuffer, "toggleD0\n"))
	{
		P1OUT ^= BIT0;
	}
	else if (!strcmp(rxBuffer, "setD1\n"))
	{
		P1OUT |= BIT6;
	}
	else if (!strcmp(rxBuffer, "resetD1\n"))
	{
		P1OUT &=~ BIT6;
	}
	else if (!strcmp(rxBuffer, "toggleD1\n"))
	{
		P1OUT ^= BIT6;
	}

	else if (!strcmp(rxBuffer, "toggleD1"))
	{
		P1OUT ^= BIT6;
	}


	else
	{
		SerialWriteString("Command not recognized, type ? for menu\n");
	}

	//clear rxBuffer and rxIndex
	memset(rxBuffer, 0x00, RX_BUFFER_SIZE);
	rxIndex = 0;
	rxFlag = 0;

	//reenable the rx interrupts
	IE2 |= UCA0RXIE;

}

/////////////////////////////////////
//SerialWriteStringBytes
//used for transmitting the rx buffer
//back over the serial port - write the buffer
//and clear it
void SerialWriteStringBytes(char buffer[], uint8_t size)
{
	uint8_t i = 0;
	for (i = 0 ; i < size ; i++)
	{
		//write out each char in the buffer
	    while (!(IFG2&UCA0TXIFG));                // USCI_A0 TX buffer ready?

	    UCA0TXBUF = buffer[i];
	}

}




/////////////////////////////////////
//SerialWriteString
//used for transmitting just a string with
//unknown size.  does not clear a buffer
void SerialWriteString(char * buffer)
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


//////////////////////////////////////
//printMenu function - type a ? and
//it prints out the list of commands
//
void printMenu(void)
{
	SerialWriteString("Commands:\r\n");

	SerialWriteString("setD0    - Pin 2  high\r\n");
	SerialWriteString("setD1    - Pin 14 high\r\n");
	SerialWriteString("resetD0  - Pin 2  low\r\n");
	SerialWriteString("resetD1  - Pin 14 low\r\n");
	SerialWriteString("toggleD0 - Pin 2 toggle\r\n");
	SerialWriteString("toggleD1 - Pin 14 toggle\r\n");

}

//////////////////////////////////////////
//UART interrupt - RX
//use variables to track the buffer size,
//position, etc.
/*  Echo back RXed character, confirm TX buffer is ready first */
#pragma vector=USCIAB0RX_VECTOR
__interrupt void USCI0RX_ISR(void)
{
    while (!(IFG2&UCA0TXIFG));                // USCI_A0 TX buffer ready?

    char t = UCA0RXBUF;				//read the char to clear the flag

    //test the buffer to see if it's full
    if (rxIndex < (RX_BUFFER_SIZE - 1))
    {
    	rxBuffer[rxIndex] = t;	//put the char into the buffer
    	rxIndex++;					//increment the index

    	//now test the char for end of line
    	if (t == '\n')
    	{
    		rxBuffer[rxIndex] = '\0';		//add null char
    		rxFlag = 1;
    	}
    }

    else
    {
    	//buffer is full - reset the counter and clear the buffer
    	rxIndex = 0x00;
    	rxFlag = 0;
    	memset(rxBuffer, 0x00, RX_BUFFER_SIZE);

    }

}



/////////////////////////////////////////////////
//SPI_init()
//Setup GPIO pins for SPIB.
//We already have a uart on P1.1 and P1.2.  Now
//we need to set up the SPI.
//
//For this, we'll need a chip select, MOSI pin, MISO pin,
//clock.
//
//Here's the pinout
//* P1.1 - UCA0SOMI	- slave out, master in
//* P1.2 - UCA0SIMO - slave in master out
//* P1.4 - UCA0CLK - serial clock
//* P1.5 - chip select - config as general output

void SPI_init(void)
{
	//chip select - P1.4
	P1DIR |= CS_PIN;
	P1OUT |= CS_PIN;

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

    UCB0BR0 |= 0x08;                          //prescaler 8 - low bits
    UCB0BR1 = 0;                              //prescaler - high bits
    //UCA0MCTL = 0;                             // No modulation - not needed for SPIB

    //clear the bit to start the SPI module
    UCB0CTL1 &= ~UCSWRST;                     // **Initialize USCI state machine**


	//deselect the device
    SPI_deselect();
}


/////////////////////////////////////////////////
//SPI_select
void SPI_select(void)
{
	P1OUT &=~ CS_PIN;
}

void SPI_deselect(void)
{
	P1OUT |= CS_PIN;
}

/////////////////////////////////////////////////
//SPI write functions
///////////////////////////////////////////////////
//send 8 bits over SPI
void SPI_Write(uint8_t data)
{
	SPI_select();

    while (!(IFG2 & UCB0TXIFG));   // USCI_A0 TX buffer ready?

    //send high byte data
    UCB0TXBUF = data;
    while (!(IFG2 & UCB0TXIFG));   // USCI_A0 TX buffer ready?
    while(UCB0STAT & UCBUSY);

    SPI_deselect();

}

///////////////////////////////////////////////////
//write 8 bit data to 16 bit address
void SPI_WriteData(uint16_t address, uint8_t data)
{
	//split the high and low bytes
	uint8_t lowByte = address & 0x00FF;
	uint8_t highByte = (address >> 8) & 0xFF;

	SPI_select();

    while (!(IFG2 & UCB0TXIFG));   // USCI_A0 TX buffer ready?

    //load the  tx buffer with a write command value
    UCB0TXBUF = 0xCC;
    while (!(IFG2 & UCB0TXIFG));   // USCI_A0 TX buffer ready?
    while(UCB0STAT & UCBUSY);

    //send high byte data
    UCB0TXBUF = highByte;
    while (!(IFG2 & UCB0TXIFG));   // USCI_A0 TX buffer ready?
    while(UCB0STAT & UCBUSY);

    //send low byte
    UCB0TXBUF = lowByte;
    while (!(IFG2 & UCB0TXIFG));   // USCI_A0 TX buffer ready?
    while(UCB0STAT & UCBUSY);

    //finally, send the data
    UCB0TXBUF = data;
    while (!(IFG2 & UCB0TXIFG));   // USCI_A0 TX buffer ready?
    while(UCB0STAT & UCBUSY);

    SPI_deselect();

}


////////////////////////////////////////////////////
//SPI_ReadData(address16)
//
uint8_t SPI_ReadData(uint16_t address)
{
	//split the high and low bytes
	uint8_t lowByte = address & 0x00FF;
	uint8_t highByte = (address >> 8) & 0xFF;

	SPI_select();

    while (!(IFG2 & UCB0TXIFG));   // USCI_A0 TX buffer ready?

    //load the tx buffer with a read command
    //to set it to reading mode

    UCB0TXBUF = 0xFF;
    while (!(IFG2 & UCB0TXIFG));   // USCI_A0 TX buffer ready?
    while (!(IFG2 & UCB0RXIFG));   // USCI_A0 TX buffer ready?
    while(UCB0STAT & UCBUSY);

    //send high byte address
    UCB0TXBUF = highByte;
    while (!(IFG2 & UCB0TXIFG));   // USCI_A0 TX buffer ready?
    while (!(IFG2 & UCB0RXIFG));   // USCI_A0 TX buffer ready?
    while(UCB0STAT & UCBUSY);

    //send low byte
    UCB0TXBUF = lowByte;
    while (!(IFG2 & UCB0TXIFG));   // USCI_A0 TX buffer ready?
    while (!(IFG2 & UCB0RXIFG));   // USCI_A0 TX buffer ready?
    while(UCB0STAT & UCBUSY);

    //send dummy byte to generate clock
    UCB0TXBUF = 0x00;
    while (!(IFG2 & UCB0TXIFG));   // USCI_A0 TX buffer ready?
    while (!(IFG2 & UCB0RXIFG));   // USCI_A0 RX buffer ready?
    while(UCB0STAT & UCBUSY);

    uint8_t readValue = UCB0RXBUF;	//read the incomming byte

    SPI_deselect();

	return readValue;
}











//////////////////////////////////////
//TimerA ISR
#pragma vector = TIMER0_A0_VECTOR
__interrupt void Timer_A(void)
{
	//clear the timer interrupt
	TACTL &=~ BIT0;

	TimeDelay_Decrement();

}


/////////////////////////////////////
//P1 ISR for the user button
#pragma vector = PORT1_VECTOR
__interrupt void Port_1(void)
{
	//clear the interrupt flag - button
	P1IFG &=~ BIT3;

	//do something....
	P1OUT ^= BIT6;

}

