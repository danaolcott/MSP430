/*
 * 1/4/19
 * NRF24L01 Transceiver Project
 *
 * Testing a set of functiosn to control the NRF24L01 IC.
 *
 * The following pins will be used:
 *
 * User Button - P1.3
 * Red LED - P1.0
 *
 * Uart - P1.1 / P1.2 (Use the HW Serial Port)
 *
 *
 * The spi interface uses SPIB located on pins
 * P1.5, P1.6, and P1.7 (clk, MISO, MOSI)
 * CS Pin = 2.4
 *
 * Control Lines for the NRF24L01:
 * IRQ - P1.4 - Falling edge, pullup
 * CE Pin - P2.0
 *
 * NOTE:  Need to disable the green led on P1.6
 *
 * NOTE: There is an error in the usart init function.
 * If you call it after spi init, the spi will not work
 * anymore.
 *
 *
 */

//includes
#include <msp430.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>

#include "timer.h"
#include "spi.h"
#include "usart.h"
#include "nrf24l01.h"

void GPIO_init(void);
void Interrupt_init(void);
void LED_Red_Toggle(void);
void LED_Red_On(void);
void LED_Red_Off(void);

//main program
int main(void)
{
	//disable the watchdog timer
	WDTCTL = WDTPW + WDTHOLD;
	
	GPIO_init();		//leds and buttons
	Interrupt_init();	//button interrupts
	usart_init();		//
	spi_init(SPI_SPEED_1MHZ);
	Timer_init();
	nrf24_init(NRF24_MODE_TX);

	int counter = 0;
	int n = 0x00;
	uint8_t txBuffer[8] = "Message1";

	while (1)
	{
		LED_Red_Toggle();
		nrf24_transmitData(txBuffer, 8);

		Timer_delay_ms(2000);

		counter++;
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

	//configure 1.4 as input with pullup
	P1DIR &=~ BIT4;		//input
	P1REN |= BIT4;		//enable pullup/down
	P1OUT |= BIT4;		//resistor set to pull up

}

///////////////////////////////////////////
//Enable interrupts on P1.3 and P1.4
//Each falling with pullup enabled
void Interrupt_init(void)
{
	//enable all the interrupts
	__bis_SR_register(GIE);

	P1IE |= BIT3;		//enable button interrupt
	P1IFG &=~ BIT3;		//clear the flag

	P1IE |= BIT4;		//enable button interrupt
	P1IFG &=~ BIT4;		//clear the flag


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
//P1.3 - User Button
//P1.4 - IRQ Pin
#pragma vector = PORT1_VECTOR
__interrupt void Port_1(void)
{
	//Get the interrupt flag....
	//User button
	if (P1IFG & BIT3)
	{
		//clear the interrupt flag - button
		LED_Red_Toggle();

		P1IFG &=~ BIT3;
	}

	else if (P1IFG & BIT4)
	{
		nrf24_ISR();

		P1IFG &=~ BIT4;
	}

}







