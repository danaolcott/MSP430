/**
 * BMP280 Test project
 * The purpose of this project is to test the BMP280
 * breakout board in i2c mode.  The pinout is for SPI,
 * but it appears you can use it in i2c mode
 *
 *
 * main.c
 */

#include <msp430.h>
#include <stdio.h>
#include <stdint.h>
#include <stddef.h>
#include "i2c.h"
#include "BMP280.h"

//prototypes
void delay_ms(volatile int ticks);
void TimeDelay_Decrement(void);
void GPIO_init(void);
void TimerA_init(void);
void Interrupt_init(void);
void LED_RED_TOGGLE(void);

//global variables
static volatile int TimeDelay;
static uint8_t id = 0;
static BMP280_Data bmpData;

int main(void)
{
    //disable the watchdog timer
    WDTCTL = WDTPW + WDTHOLD;

    TimerA_init();
    GPIO_init();
    Interrupt_init();
    i2c_init(I2C_ADDRESS);         //attempt communicate with i2c device

    BMP280_init();

    while (1)
    {
        //read chip id
        id = BMP280_readChipID();

        //read temperature and pressure
        BMP280_read(&bmpData);

        LED_RED_TOGGLE();
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

///////////////////////////////////////////
//Configure P1.0 red led as output and
//user button on P1.3 as input interrupt
void GPIO_init(void)
{
    P1DIR |= BIT0;

    P1OUT &=~ BIT0;     //turn off

    //set up the user button bit 3
    //as input with pull up enabled
    P1DIR &=~ BIT3;     //input
    P1REN |= BIT3;      //enable pullup/down
    P1OUT |= BIT3;      //resistor set to pull up
}

/////////////////////////////////////////
//Configure timer to timeout at 1ms based on
//internal clock at 1mhz
//
void TimerA_init(void)
{
    BCSCTL1 = CALBC1_1MHZ;
    DCOCTL = CALDCO_1MHZ;

    //Set up Timer A register TACTL
    TACTL = 0x0000;     //start from all all clear
    TACTL |= BIT9;      //use SMCLK as the source
    TACTL |= BIT7;      //use prescaler 8
    TACTL |= BIT6;
    TACTL |= BIT4;      //count up to TACCR0(set below)
    TACTL |= BIT1;      //enable the interrupt
    TACTL &=~ BIT0;     //clear all pending interrupts

    //set the countup value.  for 1ms delay,
    //the countup value depends on the clock freq.
    TACCR0 = 125;       //use 125 for  1 mhz

    //TACCTL0 - compare capture control register.
    //this has to be set up along with the timer interrupt
    //bit 4 enables the interrupts for this register
    //also called CCIE

    TACCTL0 = CCIE;     //compare, capture interrupt enable

}

///////////////////////////////////////////
void Interrupt_init(void)
{
    //enable all the interrupts
    __bis_SR_register(GIE);

    P1IE |= BIT3;       //enable button interrupt
    P1IFG &=~ BIT3;     //clear the flag

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

    //do something
}



