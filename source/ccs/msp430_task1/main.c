#include <msp430.h> 

//////////////////////////////////////////////
/*
msp430_task1 - Tasking example project
Dana Olcott
11/25/17

The purpose of this project is to build a simple
tasker that runs two tasks at different frequencies.
The tasker is a table of TaskStruct, each containing
flags for enabled and run, a function pointer, timeout
value, current tick value.  The task controller/ evaluator
function is called from the Timer isr.  Run flag is polled
in the main loop.  This example follows along with with
one I saw in an embedded systems book for the Renesas XXXXX
processor.  Chapter 12??

https://webpages.uncc.edu/~jmconrad/ECGR4101-2012-01/notes/All_ES_Conrad_Final_Soft_Proof_Blk.pdf

For this case, tasks will simply toggle led red and led green.


 */

/////////////////////////////////
//includes
#include "task.h"

////////////////////////////////////////////////////////
//global variables
static volatile int TimeDelay;

//prototypes
void delay_ms(volatile int ticks);
void TimeDelay_Decrement(void);
void GPIO_init(void);
void TimerA_init(void);
void Interrupt_init(void);
void LedRed_Toggle(void);
void LedGreen_Toggle(void);
void LedRed_Set(void);
void LedGreen_Set(void);
void LedRed_Clear(void);
void LedGreen_Clear(void);

uint8_t i;

int main(void)
{
	//disable the watchdog timer
	WDTCTL = WDTPW + WDTHOLD;

	TimerA_init();
	GPIO_init();
	Interrupt_init();

	LedRed_Clear();
	LedGreen_Clear();

	/////////////////////////////////////////
	//configure the scheduler with some tasks
	Task_Init();
	Task_AddTask(LedRed_Toggle, 500, 0);
	Task_AddTask(LedGreen_Toggle, 500, 1);

	//start the scheduler
	Task_StartScheduler();

	//should never make it here


	while (1){};

	return 0;
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

//////////////////////////////////////////////
//Configure GPIO pins - led red and green
//located on P1.0 and P1.6.  Also configure
//the user button on P1.3 as input with pullup
//enabled.
//
void GPIO_init(void)
{
	//led red and green as output
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
	//set the clock frequency - 16mhz
	BCSCTL1 = CALBC1_16MHZ;
	DCOCTL = CALDCO_16MHZ;

	//set the clock frequency - 8mhz
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


///////////////////////////////////////////
void Interrupt_init(void)
{
	//enable all the interrupts
	__bis_SR_register(GIE);

	P1IE |= BIT3;		//enable button interrupt
	P1IFG &=~ BIT3;		//clear the flag
}

///////////////////////////////////////////
void LedRed_Toggle(void)
{
	P1OUT ^= BIT0;
}

///////////////////////////////////////////
void LedGreen_Toggle(void)
{
	P1OUT ^= BIT6;
}

void LedRed_Set(void)
{
	P1OUT |= BIT0;
}

void LedGreen_Set(void)
{
	P1OUT |= BIT6;
}

void LedRed_Clear(void)
{
	P1OUT &=~ BIT0;
}

void LedGreen_Clear(void)
{
	P1OUT &=~ BIT6;
}



//////////////////////////////////////
//TimerA ISR
#pragma vector = TIMER0_A0_VECTOR
__interrupt void Timer_A(void)
{
	//clear the timer interrupt
	TACTL &=~ BIT0;

	TimeDelay_Decrement();

	Task_TimerISRHandler();		//task table
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


