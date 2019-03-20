/*
 * 3/12/19
 * MSP430 - SimpleOS
 * Dana Olcott - Dana's Boatshop.com
 *
 * The goal of this project is to create a super simple
 * opperating system to run on the MSP430G2553.  It will include
 * context switching, round robin scheduler, etc.  The concepts
 * used in this project follow along with information presented in an
 * online embedded systems class for the ARM Cortex processor
 * (see edx.org).  The main differences include:
 * - assembly instruction set for the MSP430.
 * - stack is a member of the task block struct.
 * - statically allocated array of task blocks with option to add as
 *   many as the memory allows.
 * - others, but not sure since the project is not completed.
 *
 * After going through this exercise, I found you can get about 4 tasks
 * running with a 2ms timeslice.  This is using a 48-word task stack.  Since
 * the main stack size is not defined, increasing the task stack size has
 * strange effects.
 *
 * SimpleOS uses one time to run the scheduler.
 *
 */
//includes
#include <msp430g2553.h>
#include <stdio.h>
#include <stdint.h>
#include <stddef.h>
#include <string.h>

#include "SimpleOS.h"


//prototypes
void GPIO_init(void);
void TimerA_init(void (*functionPtr)(void));        //pass isr handler ptr
void LED_RED_TOGGLE(void);
void LED_GREEN_TOGGLE(void);

void TaskFunction1(void);
void TaskFunction2(void);
void TaskFunction3(void);
void TaskFunction4(void);



////////////////////////////////////////////////
//Timer ISR Handler
void (*gTimerISRHandler)(void);
void TimerISR_DefaultHandler(void);

///////////////////////////////////////////////
//Task Counters
volatile uint32_t counter1 = 0x00;
volatile uint32_t counter2 = 0x00;
volatile uint32_t counter3 = 0x00;
volatile uint32_t counter4 = 0x00;


//main program
int main(void)
{
    //disable the watchdog timer
    WDTCTL = WDTPW + WDTHOLD;

    //disable interupts
    __bic_SR_register(GIE);

    //Pass Timer handler into the timer config as function to run on timeout
    GPIO_init();
    TimerA_init(&SimpleOS_ISR);

    //Configure the tasks and start the OS
    SimpleOS_init(TaskFunction1);
    SimpleOS_addThread(TaskFunction2);
    SimpleOS_addThread(TaskFunction3);
    SimpleOS_addThread(TaskFunction4);

    SimpleOS_launch();

    //Should never make it here
    while(1){};

    return 0;
}


///////////////////////////////////////////////////
//GPIO_init()
//Configure the onboard leds
void GPIO_init(void)
{
    //setup bit 0 and 6 as output
    P1DIR |= BIT0;      //red
    P1DIR |= BIT6;      //green

    P1OUT &=~ BIT0;     //turn off
    P1OUT &=~ BIT6;     //turn off

    //Configure 2 more pins - P2.0 and P2.1
    //for toggling in Tasks 3 and 4
    P2DIR |= BIT0;
    P2DIR |= BIT1;
    P2DIR |= BIT2;

    P2OUT &=~ BIT0;
    P2OUT &=~ BIT1;
    P2OUT &=~ BIT2;
}

/////////////////////////////////////////////////////
//TimerA_init
//Set the CPU speed to 16mhz and configure the timer
//to timeout and interrupt at 100hz, 10ms timeslices.
//NOTE:  Keep an eye on the timer reload value.  Although
//2000 will result in 1ms time slice, the counter keeps running
//during the ISR (strange).  If it overflows, it will generate
//another interrupt right on exit.  This will mess up the context
//switch.
//
//Argument: Pointer to function to run in the ISR
//
void TimerA_init(void (*functionPtr)(void))
{
    if (functionPtr != NULL)
        gTimerISRHandler = functionPtr;
    else
        gTimerISRHandler = &TimerISR_DefaultHandler;

    BCSCTL1 = CALBC1_16MHZ;
    DCOCTL = CALDCO_16MHZ;

    //Set up Timer A register TACTL
    TACTL = 0x0000;     //start from all all clear
    TACTL |= BIT9;      //use SMCLK as the source
    TACTL |= BIT7;      //use prescaler 8
    TACTL |= BIT6;
    TACTL |= BIT4;      //count up to TACCR0(set below)
    TACTL |= BIT1;      //enable the interrupt
    TACTL &=~ BIT0;     //clear all pending interrupts

    //set the countup value for 1ms delay
    //TACCR0 = 2000;      //use 2000 for 16 mhz

    ///////////////////////////////////////////////////
    //This value sets the timeout frequency.
    //For a 16mhz clock, use the following values:
    //1ms = 2000;
    //2ms = 4000;
    //4ms = 8000;
    //10ms = 20000;
    //... etc
    TACCR0 = 4000;

    //reset the timer counter
    TA0R = 0x00;

    //TACCTL0 - compare capture control register.
    //this has to be set up along with the timer interrupt
    //bit 4 enables the interrupts for this register
    //also called CCIE

    TACCTL0 = CCIE;     //compare, capture interrupt enable
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

////////////////////////////////////////////////////////////
//Timer A - ISR for TIMER0_A0_VECTOR
//Format used with the GNU compiler
__attribute__((interrupt(TIMER0_A0_VECTOR))) void Timer_A(void)
{
    TACTL &=~ BIT0;     //clear the timer interrupt flag
    (*gTimerISRHandler)();
}

/////////////////////////////////////////////
//Default Timer ISR
//This function is set to the TimerISR when
//initialization routine passes NULL as target ISR.
//
void TimerISR_DefaultHandler(void)
{
    TACTL &=~ BIT0;     //clear the timer interrupt flag
    //do something
}



///////////////////////////////////////////
//TaskFunction1
//Since there are no delays, the frequency
//should not depend on the value of the timeslice
//
void TaskFunction1(void)
{
    while (1)
    {
        if (!(counter1 % 2000))
        {
            LED_RED_TOGGLE();
        }

        counter1++;
    }
}

//////////////////////////////////////////////
//TaskFunction2
void TaskFunction2(void)
{
    while (1)
    {
        if (!(counter2 % 2000))
        {
            LED_GREEN_TOGGLE();
        }

        counter2++;
    }
}



//////////////////////////////////////////////
//TaskFunction2
void TaskFunction3(void)
{
    while (1)
    {
        if (!(counter3 % 2000))
        {
            P2OUT ^= BIT0;
        }

        counter3++;
    }
}

//////////////////////////////////////////////
//TaskFunction2
void TaskFunction4(void)
{
    while (1)
    {
        if (!(counter4 % 2000))
        {
            P2OUT ^= BIT1;
        }

        counter4++;
    }
}




