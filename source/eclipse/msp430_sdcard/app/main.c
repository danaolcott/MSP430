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
	char txBuffer[64];
	char rxBuffer[64];

	memset(txBuffer, 0x00, 64);
	memset(rxBuffer, 0x00, 64);


	BYTE res = FR_OK;
	FATFS fs;			/* File system object */
//	uint8_t sdcardReady = 0;
	uint8_t result = 0;

	//disable the watchdog timer
	WDTCTL = WDTPW + WDTHOLD;
	
	GPIO_init();		//leds and buttons
	Interrupt_init();	//button interrupts
	Timer_init();
	spi_init(SPI_SPEED_400KHZ);
	usart_init();

	result = mmc_GoIdleState();

	if (result == 1)
	{
		res = pf_mount(&fs);

		if (res == FR_OK)
		{
			spi_init(SPI_SPEED_4MHZ);
		}

		else
		{
			while(1)
			{
				LED_Red_Toggle();
				Timer_delay_ms(50);
			}
		}
	}

	else
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


		//try writing hello to test.txt
		//unsigned int size = mmc_writeFile(&fs, "test.txt", "hello", 5);

		//read a file into a buffer
//		unsigned int size = mmc_readFile(&fs, "test.txt", rxBuffer, 64);

		//read file into usart output
		unsigned int size = mmc_readFile(&fs, "test.txt", NULL, 64);


/*

		if (size == 5)
		{
			LED_Red_Toggle();
			Timer_delay_ms(100);
			LED_Red_Toggle();
			Timer_delay_ms(100);
		}
		else
		{
			LED_Red_Toggle();
			Timer_delay_ms(10);
			LED_Red_Toggle();
			Timer_delay_ms(10);
		}
*/

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







