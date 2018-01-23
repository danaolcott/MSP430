/*TimerA Configuration
 * Dana Olcott
 * 1/20/18
 *
 *
 *
 */

#include <stdint.h>

#include <msp430.h>
#include <msp430g2553.h>
#include "timer.h"


static volatile uint16_t gTimeTick;
static volatile uint16_t gCounter1Tick;

void Timer_init(void)
{
    gTimeTick = 0x00;
    gCounter1Tick = 0x00;



	//set the clock frequency 16 mhz
	BCSCTL1 = CALBC1_16MHZ;
	DCOCTL = CALDCO_16MHZ;

//	BCSCTL1 = CALBC1_8MHZ;
//	DCOCTL = CALDCO_8MHZ;

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

	TACCR0 = 2000;		//use 2000 for 16 mhz
	//	TACCR0 = 125;		//use 125 for  1 mhz
	//	TACCR0 = 1000;		//use 1000 for 8 mhz

	//TACCTL0 - compare capture control register.
	//this has to be set up along with the timer interrupt
	//bit 4 enables the interrupts for this register
	//also called CCIE

	TACCTL0 = CCIE;		//compare, capture interrupt enable
}


/////////////////////////////////////
//countdown to 0
void Timer_DelayDecrement(void)
{
	if (gTimeTick != 0x00)
		gTimeTick--;
}

/////////////////////////////////////
void Timer_delay_ms(uint16_t delay)
{
    gTimeTick = delay;

    //gTimeTick decremented in timer isr
	while (gTimeTick !=0);

}



////////////////////////////////////////
void Timer_start(void)
{
	TACTL |= BIT1;		//enable the interrupt
	TACTL &=~ BIT0;		//clear all pending interrupts
}
void Timer_stop(void)
{
	TACTL &=~ BIT1;			//disable interrupts
	TACTL &=~ BIT0;		//clear all pending interrupts
}


void Timer_Counter1Set(uint16_t count)
{
	gCounter1Tick = count;
}

uint16_t Timer_Counter1Get(void)
{
	return gCounter1Tick;
}

void Timer_Counter1Decrement(void)
{
	if (gCounter1Tick != 0x00)
		gCounter1Tick--;
}



//////////////////////////////////////
//TimerA ISR
#pragma vector = TIMER0_A0_VECTOR
__interrupt void Timer_A(void)
{
	//clear the timer interrupt
	TACTL &=~ BIT0;

    Timer_DelayDecrement();
    Timer_Counter1Decrement();
}



