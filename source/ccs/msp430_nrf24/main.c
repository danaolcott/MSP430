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
 * LED Green = 2.2    (note: normally the green led is on 1.6)
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
 * When used as a transmitter, it interfaces with the
 * BME280 temperature / pressure sensor.  CS Pin - 2.3
 * SPI pins share with the radio.
 *
 *
 * Remaining Pins: 2.1- limited to GPIO - leds?
 *
 *
 * NOTE: Board uses a 3.3v regulator with Digikey PN: 497-1485-5-ND
 * LD1117AV33 - STM - Max input 15v.  Pinout (looking at the markings):
 * left - GND
 * middle - output (tab connected to output)
 * right - input
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
#include "BMP280.h"
#include "utility.h"

void GPIO_init(void);
void Interrupt_init(void);
void LED_Red_Toggle(void);
void LED_Red_On(void);
void LED_Red_Off(void);

void LED_Green_Toggle(void);
void LED_Green_On(void);
void LED_Green_Off(void);


//uint8_t outBuffer[64];
//int n = 0x00;
uint8_t rxBuffer[16];
uint8_t txBuffer[8] = {0x00};

//main program
int main(void)
{
	//disable the watchdog timer
	WDTCTL = WDTPW + WDTHOLD;
	
	GPIO_init();					//leds and buttons
	Interrupt_init();				//button and irq pin
	usart_init();					//9600 baud
	SPI_init(SPI_SPEED_1MHZ);
	Timer_init();
	nrf24_init(NRF24_MODE_TX);
	Timer_delay_ms(1000);			//wait a bit
	BMP280_init();					//spi mode - cs pin
	Timer_delay_ms(1000);			//wait a bit
	BMP280_wakeup();
	Timer_delay_ms(1000);			//wait a bit

	while (1)
	{
		LED_Green_On();
		Timer_delay_ms(200);			//wait a bit
		LED_Green_Off();

//		uint8_t id = BMP280_readChipID();		//reads 0x58 - BMP280 - NOT BME280
		BMP280_Data data = BMP280_read();

//		int cPressKPAInt = data.cPressure / 1000;
//		int cPressKPAFrac = data.cPressure % 1000;

//		n = sprintf((char*)outBuffer, "ID: 0x%02x, Temp: %d.%d\r\n", id, (int)data.cTemperatureFInt, (int)data.cTemperatureFFrac);
//		usart_writeStringLength(outBuffer, n);

//		n = sprintf((char*)outBuffer, "Pressure: %d.%d\r\n", cPressKPAInt, cPressKPAFrac);
//		usart_writeStringLength(outBuffer, n);


		//transmit pressure
		txBuffer[0] = 0xFE;
		txBuffer[1] = STATION_1;
		txBuffer[2] = MID_PRESS_BMP280;
		txBuffer[3] = (uint8_t)(data.cPressure & 0xFF);
		txBuffer[4] = (uint8_t)((data.cPressure >> 8) & 0xFF);
		txBuffer[5] = (uint8_t)((data.cPressure >> 16) & 0xFF);
		txBuffer[6] = (uint8_t)((data.cPressure >> 24) & 0xFF);
		txBuffer[7] = 0xFE;

		//send the data - toggle the red, so if it hangs, the red will stay on
		LED_Red_On();
		Timer_delay_ms(200);			//wait a bit
		nrf24_transmitData(0, txBuffer, 8);
//		LED_Red_Off();

		Timer_delay_ms(2300);


		//transmit temperature
		txBuffer[0] = 0xFE;
		txBuffer[1] = STATION_1;
		txBuffer[2] = MID_TEMP_BMP280;
		txBuffer[3] = data.cTemperatureFInt;
		txBuffer[4] = data.cTemperatureFFrac;
		txBuffer[5] = data.cTemperatureCInt;
		txBuffer[6] = data.cTemperatureCFrac;
		txBuffer[7] = 0xFE;

		//send the data
		LED_Red_On();
		Timer_delay_ms(200);			//wait a bit
		nrf24_transmitData(0, txBuffer, 8);
//		LED_Red_Off();

		Timer_delay_ms(2300);
	}

	return 0;
}


//////////////////////////////////////
//Configure IO - LEDs and Buttons
//
//P1.0 - red led - output
//P1.3 - user button - input, pullup, interrupted
//P1.4 - IRQ Pin - nrf24l01 - inpullup, interrupted
//
//NOTE: P1.6 conflicts with SPIB, so dont use it.
//
void GPIO_init(void)
{
	//setup bit 0 as output - led red
	P1DIR |= BIT0;
	P1OUT &=~ BIT0;		//turn off

	//setup bit 2, port 2 as output - led green
	P2DIR |= BIT2;
	P2OUT &=~ BIT2;		//turn off

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

void LED_Green_Toggle(void)
{
	P2OUT ^= BIT2;
}

void LED_Green_On(void)
{
	P2OUT |= BIT2;
}

void LED_Green_Off(void)
{
	P2OUT &=~ BIT2;
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

	if (P1IFG & BIT4)
	{
		nrf24_ISR();
		P1IFG &=~ BIT4;
	}

}







