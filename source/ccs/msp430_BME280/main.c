/*
 * 9/21/18
 * MSP430 BME280 Project
 * This is a simple project that reads
 * temperature, pressure, and humidity and
 * displays the results on a 128x64 oled lcd.
 *
 * The BME280 and LCD both have an i2c interface
 *
 */
//includes
#include <msp430.h>
#include <stdio.h>
#include <stdint.h>
#include <stddef.h>
#include <string.h>

#include "TI_USCI_I2C_master.h"
#include "i2c.h"
#include "adc.h"
#include "BME280.h"
#include "ssd1306_lcd.h"
#include "temp_sensor.h"
/////////////////////////////////////////////////////
/*
 * Notes:
 * Use the real i2c address - not the upshifted address
 * Uses P1.6 (SCL) and P1.7 (SDA).  Remove the jumper on the launchpad board.
 *
 */
/////////////////////////////////////////////////////
//


//prototypes
void delay_ms(volatile int ticks);
void TimeDelay_Decrement(void);
void GPIO_init(void);
void TimerA_init(void);
void Interrupt_init(void);
void LED_RED_TOGGLE(void);

//global variables
static volatile int TimeDelay;

//main program
void main(void)
{
	//disable the watchdog timer
	WDTCTL = WDTPW + WDTHOLD;
	
	TimerA_init();
	GPIO_init();
	Interrupt_init();

	//toggle the red led P1.0 a few times
	int i = 0;
	for (i = 0; i < 10; i++)
	{
		LED_RED_TOGGLE();
		delay_ms(100);
	}

	i2c_init(BME280_I2C_ADDRESS);		//check for the BME280
	delay_ms(200);
	i2c_init(LCD_I2C_ADDRESS);			//check for the lcd
	delay_ms(200);
	BME280_init();
	delay_ms(200);
	adc_init();							//channel 5, P1.5 as ADC
	temp_sensor_init();					//

	LCD_Init();							//init the LCD
	LCD_Clear(0x00);

	//////////////////////////////////////////////////
	while(1)
	{
		LED_RED_TOGGLE();

		//read the pressure, temperature, humidity
		BME280_Data bmeData = BME280_read();

		//read the external temp sensor
		float extTemp = temp_sensor_readF();
		int extTempInt = (int)extTemp;

		int extTempFrac = (int)(extTemp * 10);
		extTempFrac = extTempFrac % 10;

		//output the results to the LCD
		uint8_t buffer[64] = {0x00};
		LCD_DrawStringKern(0, 3, "Temp (EXT)");
		LCD_DrawStringKern(3, 4, "Press(kPa)");
		LCD_DrawStringKern(6, 5, "Humidity");

		int n = sprintf((char*)buffer, "%d.%d(%d.%d)   ", (int)bmeData.cTemperatureFInt, (int)bmeData.cTemperatureFFrac,
				extTempInt, extTempFrac);

		LCD_DrawStringKernLength(1, 3, buffer, n);

		int cPressKPAInt = bmeData.cPressureInt / 1000;
		int cPressKPAFrac = bmeData.cPressureInt % 1000;

		n = sprintf((char*)buffer, "%d.%d   ", cPressKPAInt, cPressKPAFrac);
		LCD_DrawStringKernLength(4, 3, buffer, n);

		n = sprintf((char*)buffer, "%d.%d   ", (int)bmeData.cHumidityInt, (int)bmeData.cHumidityFrac);
		LCD_DrawStringKernLength(7, 3, buffer, n);

		delay_ms(1000);
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
//Configure red led as output
//Note: green led conflicts with i2c
//
void GPIO_init(void)
{
	//setup bit 0 and 6 as output

	P1DIR |= BIT0;
	P1OUT &=~ BIT0;		//turn off

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
	BCSCTL1 = CALBC1_16MHZ;
	DCOCTL = CALDCO_16MHZ;

//	BCSCTL1 = CALBC1_8MHZ;
//	DCOCTL = CALDCO_8MHZ;

	//use this with uart at 9600
//	BCSCTL1 = CALBC1_1MHZ;
//	DCOCTL = CALDCO_1MHZ;

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
	TACCR0 = 2000;		//use 2000 for 16 mhz
//	TACCR0 = 125;		//use 125 for  1 mhz
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
	P1OUT ^= BIT0;
}





