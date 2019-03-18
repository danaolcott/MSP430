/*
 * 3/12/19
 * MSP430 - Simple OS
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


////////////////////////////////////////////////
//Timer ISR Handler
void (*gTimerISRHandler)(void);
void TimerISR_DefaultHandler(void);

///////////////////////////////////////////////
//Task Counters
volatile uint32_t counter1 = 0x00;
volatile uint32_t counter2 = 0x00;

//main program
int main(void)
{
    //disable the watchdog timer
    WDTCTL = WDTPW + WDTHOLD;

    //Pass Timer handler into the timer config as function to run on timeout
    TimerA_init(SimpleOS_ISR);
    GPIO_init();

    //Configure the tasks and start the OS
    SimpleOS_init();
    SimpleOS_addThreads(TaskFunction1, TaskFunction2);
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
}

//////////////////////////////////////////////
//TimerA_init
//Set the CPU speed to 16mhz and configure the
//timer to timeout and interrupt at 1khz.
//Pass pointer
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
    TACCR0 = 2000;      //use 2000 for 16 mhz

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
void TaskFunction1(void)
{
    while (1)
    {
        if (!(counter1 % 2000))
        {
            SimpleOS_EnterCritical();
            LED_RED_TOGGLE();
            SimpleOS_ExitCritical();
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








