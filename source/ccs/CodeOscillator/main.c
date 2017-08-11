#include <msp430.h> 
//
/////////////////////////////////////////////////////
//Simple CodeOscillator Project
//The purpose of this project is to make a code
//practice oscillator for use with practicing sending CW using
//the bug key or other key  When the key is connected and contacts
//closed, output pins P1.0 and P2.1 will toggle at a rate of 580hz +/-.
//If the contact is open the output pins are low.
//P1.6 is connected to an led - green.  If the contact is open, the
//green led is on, if closed, the green led is off.  P1.0 is connected
//to red led that is off when contact is open and on with the contact
//closed.  The input contact P2.0 should be connected to the key.
//the other key contact is connected to ground.  Internal pullup
//is enabled for P2.0.
//The whole program will run in a single while loop with internal
//clock configured at 16mhz.
//
//function prototypes
void GPIO_init(void);			//general pins, LEDs

void delay_ms(volatile int ticks);
void TimerA_init(void);
void Interrupt_init(void);
void TimeDelay_Decrement(void);

static volatile int TimeDelay;





int main(void) {
	//disable the watchdog timer
	WDTCTL = WDTPW + WDTHOLD;

	//initialize the GPIO, timer, interrupts
	GPIO_init();		//leds and user button
	TimerA_init();		//timer for delay function
	Interrupt_init();	//global interrupt - timer

	while (1)
	{
		//read the control pin - if grounded, toggle the green led
		//one time and wait 600 hz
		if (!(P2IN & BIT0))
		{
			P2OUT ^= BIT1;			//toggle speaker pin
			P1OUT |= BIT0;			//red is high
			P1OUT &=~ BIT6;			//green is off
			delay_ms(1);
		}
		else
		{
			P2OUT &=~BIT1;			//speaker pin is off
			P1OUT &=~BIT0;			//red is off
			P1OUT |= BIT6;			//green is on
		}
	}

	return 0;
}


////////////////////////////////////
//function definitions
//set up the pins for the leds and
//the user button.
//P2.0 as input.  This will be the trigger for
//setting off the 600hz signal
void GPIO_init(void)
{
	P1DIR |= BIT0;		//led red output
	P1DIR |= BIT6;		//led green output

	P1OUT &=~ BIT0;		//off
	P1OUT &=~ BIT6;		//off

	//P2.0 as input
	P2DIR &=~ BIT0;		//input
	P2REN |= BIT0;		//pull up/down enabled
	P2OUT |= BIT0;		//pull up

	//P2.1 is the output to drive the speaker
	P2DIR |= BIT1;
	P2OUT &=~BIT1;

}



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

void TimerA_init(void)
{
	//set the clock frequency to 16 mhz
	BCSCTL1 = CALBC1_16MHZ;
	DCOCTL = CALDCO_16MHZ;

	//Set up Timer A register TACTL
	//set bits 1, 4, 6, 7, 9
	//0b0000001011010010
	//0x02D2
	TACTL = 0x0000;		//start from all all clear
	TACTL |= 0x02D2;
	TACTL &=~ BIT0;		//clear all pending interrupts

	//set the countup value.  for 1ms delay,
	//the countup value depends on the clock freq.

//	TACCR0 = 125;		//use 125 for  1 mhz
//	TACCR0 = 1000;		//use 1000 for 8 mhz
//	TACCR0 = 2000;		//use 2000 for 16 mhz


	//special counter value for 600hz configuration
	//ie, normalize timeout freq at 1200 hz for a toggle freq
	//of 600 hz.  slow it down by 20percent, or 1600
	//and make minor adjustments
	//1600 = 625hz
//	TACCR0 = 1647;		//exactly 600hz
	TACCR0 = 1770;


	//TACCTL0 - compare capture control register.
	//this has to be set up along with the timer interrupt
	//bit 4 enables the interrupts for this register
	//also called CCIE

	TACCTL0 = CCIE;		//compare, capture interrupt enable

}


///////////////////////////////
//enable all necessary interrupts
void Interrupt_init(void)
{
	__bis_SR_register(GIE);
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

