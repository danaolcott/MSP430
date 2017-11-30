#include <msp430.h> 

//////////////////////////////////////////////
/*
msp430_task2 - Example project that uses tasks.h/.c
Dana Olcott
11/28/17

See task.c/.h for details.  Uses TimerA as the
timebase for running tasks.

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

//Task functions
void TaskFunction_TX(void);
void TaskFunction_RX(void);
void TaskFunction_LedGreen(void);

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
	Task_AddTask("taskTx", TaskFunction_TX, 500, 0);	//name, function, period, priority
	Task_AddTask("taskRx", TaskFunction_RX, 100, 1);	//name, function, period, priority
	Task_AddTask("green", TaskFunction_LedGreen, 100, 2);	//name, function, period, priority

	Task_StartScheduler();								//Timer takes over

	//never makes it here
	while (1){};

	return 0;
}


////////////////////////////////////////////
//TaskFunction_TX
//Sends a toggle signal to a receiver task to
//toggle the red led.  Sends the signal multiple
//times to test the task message queue.  All
//items are handled when the receiver task runs.
//
void TaskFunction_TX(void)
{
	//value = 0 is red led
	uint8_t i = 0;
	TaskMessage msg = {TASK_SIG_TOGGLE, 0x00};
	uint8_t index = Task_GetIndexFromName("taskRx");

	//odd number so the red led toggles
	for (i = 0 ; i < 5 ; i++)
		Task_SendMessage(index, msg);

}

////////////////////////////////////////
//Receiver Task
//Read all messages in the queue and run.
//
void TaskFunction_RX(void)
{
	TaskMessage msg = {TASK_SIG_NONE, 0x00};
	uint8_t index = Task_GetIndexFromName("taskRx");

	while (Task_GetNextMessage(index, &msg) > 0)
	{
		switch(msg.signal)
		{
			case TASK_SIG_ON:
			{
				if (msg.value == 0)
					LedRed_Set();
				else if (msg.value == 1)
					LedGreen_Set();
				break;
			}

			case TASK_SIG_OFF:
			{
				if (msg.value == 0)
					LedRed_Clear();
				else if (msg.value == 1)
					LedGreen_Clear();
				break;
			}

			case TASK_SIG_TOGGLE:
			{
				if (msg.value == 0)
					LedRed_Toggle();
				else if (msg.value == 1)
					LedGreen_Toggle();
				break;
			}

			default:
				break;
		}
	}
}


/////////////////////////////////////
//TaskFunction_LedGreen
//No messages, just toggle the led
void TaskFunction_LedGreen(void)
{
	LedGreen_Toggle();
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


