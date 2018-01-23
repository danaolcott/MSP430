/*
 * 1/20/18
 * SD Card Project
 *
 * Shield containing the sd card is from Wiznet
 * IO Shield Version L for the MSP430.  The shield
 * contains the Wiznet5500(?) ic for connecting to
 * a network.  Also contains an sdcard holder and
 * serial SRAM.
 *
 * The spi interface uses SPIB located on pins
 * P1.5, P1.6, and P1.7 (clk, MISO, MOSI)
 * CS pins:
 * SD Card: P2.4
 * SRAM: P2.3
 *
 * See schematic for other item.
 *
 * Disable the green led
 *
 * 16x32 =
 *
 *
 *
 *
 */

//includes
#include <msp430.h>
#include <msp430g2553.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>


#include "timer.h"
#include "spi.h"
#include "usart.h"

#include "pff.h"
#include "diskio.h"


void GPIO_init(void);
void Interrupt_init(void);
void LED_Red_Toggle(void);
void LED_Red_On(void);
void LED_Red_Off(void);

//main program
int main(void)
{
	uint8_t rxBuffer[64];
	memset(rxBuffer, 0x00, 64);

	//disable the watchdog timer
	WDTCTL = WDTPW + WDTHOLD;
	
	GPIO_init();		//leds and buttons
	Interrupt_init();	//button interrupts
	Timer_init();
	spi_init(SPI_SPEED_400KHZ);
	usart_init();

	int result = mmc_init();
	if (result < 0)
	{
		while(1)
		{
			LED_Red_Toggle();
			Timer_delay_ms(50);
		}
	}


	while (1)
	{
		LED_Red_Toggle();
		Timer_delay_ms(1000);

		//unsigned int mmc_append(char* name, char appendChar, char* buffer, unsigned int size)

		char* msg = "\r\nstart of file\r\n";
		unsigned int size = mmc_writeLine("test.txt", 0, msg, strlen(msg));

		//append
		char* msg2 = "\r\nAppend Line 2\r\n";
		size = mmc_writeLine("test.txt", 1, msg2, strlen(msg2));

		char* msg3 = "\r\nAppend Line 3\r\n";
		size = mmc_writeLine("test.txt", 2, msg3, strlen(msg3));

		//append a lot of information
		int n, i = 0;
		for (i = 3 ; i < 10 ; i++)
		{
			memset(rxBuffer, 0x00, 64);
			n = sprintf((char*)rxBuffer, "\r\nAppend Text, Count: %d\r\n", i);
			size = mmc_writeLine("test.txt", i, (char*)rxBuffer, n);
		}

	}

	return 0;
}


//////////////////////////////////////
//Configure IO - LEDs and Buttons
//See encoder, lcd, etc for other io
//configuration.

//P1.0 - red led, P1.3 user button
//P2.2 - input -- tx/rx mode.
//Default is high, enable pullup
//no interrupt on P2.2, simply polled
//in the display task
//
//Note: I2C conflicts with P1.6, so
//green led does not work.

void GPIO_init(void)
{
	//setup bit 0 as output
	P1DIR |= BIT0;
	P1OUT &=~ BIT0;		//turn off

	//set up the user button bit 3
	//as input with pull up enabled
	P1DIR &=~ BIT3;		//input
	P1REN |= BIT3;		//enable pullup/down
	P1OUT |= BIT3;		//resistor set to pull up
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
void LED_Red_Toggle(void)
{
	P1OUT ^= BIT0;
}

void LED_Red_On(void)
{
	P1OUT |= BIT0;
}

void LED_Red_Off(void)
{
	P1OUT &=~ BIT0;
}


/////////////////////////////////////
//P1 ISR for the user button
#pragma vector = PORT1_VECTOR
__interrupt void Port_1(void)
{

	//clear the interrupt flag - button
	P1IFG &=~ BIT3;
}







