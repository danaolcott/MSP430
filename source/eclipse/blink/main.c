/* Dana Olcott
 * 1/11/18
 *
 * Simple Blink Application Using gcc-msp430
 * toolchain and eclipse.
 *
 * The purpose of this program is to setup
 * eclipse, mspdebug, gdb, and msp-gcc for
 * compile, upload, and debugging a program
 * on the MSP430G2553 processor.
 *
 * This application simply toggles P1.0 and P1.6
 * on a timer
 *
 * Notes:
 * Setup:
 * include <msp430.h> header
 * use #define  __MSP430G2553__
 *
 * Add the following to the linker options:
 * -mmcu=msp430g2553
 *
 * (if you dont, it will complain about can't find
 * memory.x linker file.

 *
 * If you don't want to use the debugger:
 * Compile the program as usual creating a myprogram.elf file.
 * (add elf extension?  not sure how necessary this is.)
 * Enter debug folder and run mspdebug:
 *
 * mspdebug rf2500		 - init the connection
 * prog myprogram.elf	 - flash the ic
 * run					 - start the program.(stops on a breakpoint if it exists)
 *
 * Helpful commands (too many to list all):
 * break <address>, ie, break 0x3452 - sets a breakpoint at 0x3452.
 *
 * More Notes:
 * There is a 32bit program msp-gdbproxy that starts the
 * gdb server.  From what I can tell, this migrated to
 * mspdebug. After running mspdebug, run gdb.
 *
 * The following links are very usefull:

 * http://mspgcc.sourceforge.net/manual/c1531.html
 * http://processors.wiki.ti.com/index.php/OLPC_XO-1
 * http://mspgcc.sourceforge.net/manual/book1.html
 *
 * link to more info on gdbproxy:
 * http://gdbproxy.sourceforge.net/
 *
 *
 * More info on debugging:
 * run mspdebug
 * run gdb to start the server.
 * should see "Bound to port 2000, waiting for a connection"
 *
 * This is when you launch debugger in eclipse...
 *
 * click on debug profiles / options - hardware debugging
 * use the tcp/ip... something, using port 2000.
 * if you try to debug without starting mspdebug-gdb, you will
 * get an error about timeout at localhost, port 2000.
 *
 * Initially, I got the following error after starting
 * debugging secession:
 *
 * Can't find a source file at "/build/buildd/gcc-msp430-4.6.3~mspgcc-20120406/
 * ./gcc-4.6.3/gcc/config/msp430/crt0.S"
 *
 * This was reported as a bug 2012:
 * https://bugs.launchpad.net/ubuntu/+source/gcc-msp430/+bug/1088250
 *
 * Work around:
 *
 * Load the binary ahead of time (see debug options) using mspdebug - prog, reset
 *
 * if you knew where main() function was located you could set a breakpoint
 * there.
 *
 * I also read somewhere about debugging level - set to default. (-g)
 *
 *
 *
 *
 */
//includes
#include <msp430.h>
#include <stdio.h>
#include <stdint.h>
#include <stddef.h>
#include <string.h>

//prototypes
void delay_ms(volatile int ticks);
void TimeDelay_Decrement(void);
void GPIO_init(void);
void TimerA_init(void);
void Interrupt_init(void);
void LED_RED_TOGGLE(void);
void LED_GREEN_TOGGLE(void);

//global variables
static volatile int TimeDelay;

//main program
int main(void)
{
	//disable the watchdog timer
	WDTCTL = WDTPW + WDTHOLD;

	TimerA_init();
	GPIO_init();
	Interrupt_init();

	//main loop
	while(1)
	{
		LED_RED_TOGGLE();
		LED_GREEN_TOGGLE();
		delay_ms(500);
	}

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

//////////////////////////////////////////////
//Configure time tick and the clock frequency
//Clock can be configured to run at 1, 8, or
//16mhz.

void TimerA_init(void)
{

	//set the clock frequency to 16 mhz
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
