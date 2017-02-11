/*****************************************************************************
* Product: BSP for the Simple Blinky example, MSP-EXP430G2, Vanilla kernel
* Last updated for version 5.3.0
* Last updated on  2014-04-18
*
*                    Q u a n t u m     L e a P s
*                    ---------------------------
*                    innovating embedded systems
*
* Copyright (C) Quantum Leaps, www.state-machine.com.
*
* This program is open source software: you can redistribute it and/or
* modify it under the terms of the GNU General Public License as published
* by the Free Software Foundation, either version 3 of the License, or
* (at your option) any later version.
*
* Alternatively, this program may be distributed and modified under the
* terms of Quantum Leaps commercial licenses, which expressly supersede
* the GNU General Public License and are specifically designed for
* licensees interested in retaining the proprietary status of their code.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with this program. If not, see <http://www.gnu.org/licenses/>.
*
* Contact information:
* Web:   www.state-machine.com
* Email: info@state-machine.com
*****************************************************************************/
//
/////////////////////////////////////////////////////
//bsp.c / bsp.h.
//These files are used to init all the low level hardware
//peripherals, gpio, uart, spi, etc.
//See main.c, main.h for init of all AOs
//

#include "qpn_port.h"
#include "bsp.h"
#include "tft_lcd.h"		//Adafruit tft lcd.

#include <msp430g2553.h> /* MSP430 variant used on MSP-EXP430G2 LaunchPad */

/*--------------------------------------------------------------------------*/
#define BSP_SMCLK   1100000UL

/* pin assignments to LEDs */
#define LED1   (1U << 0)
#define LED2   (1U << 6)

#define LED1_on()   (P1OUT |= (uint8_t)LED1)
#define LED1_off()  (P1OUT &= (uint8_t)~LED1)
#define LED2_on()   (P1OUT |= (uint8_t)LED2)
#define LED2_off()  (P1OUT &= (uint8_t)~LED2)

/*..........................................................................*/
#pragma vector = TIMER0_A0_VECTOR
__interrupt void timerA_ISR(void) {
#ifdef NDEBUG
    __low_power_mode_off_on_exit(); /* disable low-power mode on exit, NOTE1*/
#endif
    QF_tickXISR(0U);  /* process all time events at clock tick rate 0 */

    //need to read up on this
    //this might be related to using timer 0 timer 1, etc.
    //ie, use 1u for timeout sig1

    //added to see if this is how you add time evets
    //other than tick rate of 0, tick rate 1
 //   QF_tickXISR(1U);  /* process all time events at clock tick rate 0 */
    //that didn't work
}
/*..........................................................................*/
#pragma vector = NMI_VECTOR
__interrupt void nmi_ISR(void) {
    WDTCTL = (WDTPW | WDTHOLD);  /* Stop WDT */
}
/*..........................................................................*/
void BSP_init(void) {
    WDTCTL = (WDTPW | WDTHOLD); /* Stop WDT */
    P1DIR |= LED1 | LED2; /* configure LED1 and LED2 as outputs */
    CCR0 = (BSP_SMCLK + BSP_TICKS_PER_SEC/2) / BSP_TICKS_PER_SEC;
    TACTL = (TASSEL_2 | MC_1 | TACLR); /* SMCLK, upmode, clear timer */

    //configure the button as input
	P1DIR &=~ BIT3;
	P1REN |= BIT3;		//pull up/down enabled
	P1OUT |= BIT3;		//pull up

	//call functions to initialize the lcd
    SPIA_init();		//setup for SPI
    LCD_init();			//write ini commands

}
/*..........................................................................*/
void BSP_ledOff(void) {
    LED2_on();
}
/*..........................................................................*/
void BSP_ledOn(void) {
    LED2_off();
}

/*--------------------------------------------------------------------------*/
void QF_onStartup(void) {
    CCTL0 = CCIE; /* CCR0 interrupt enabled */
}
/*..........................................................................*/
void QF_onIdle(void) {
//      LED1_on();
//      LED1_off();

#ifdef NDEBUG
    /* adjust the low-power mode to your application */
    __low_power_mode_1(); /* Enter LPM1 */
#else
    QF_INT_ENABLE();
#endif
}
/*..........................................................................*/
void Q_onAssert(char const Q_ROM * const file, int line) {
    (void)file;       /* avoid compiler warning */
    (void)line;       /* avoid compiler warning */
    QF_INT_DISABLE();  /* make sure that interrupts are disabled */
    for (;;) {
    }
}

/*****************************************************************************
* NOTE1:
* The MSP430 interrupt processing restores the CPU status register upon
* exit from the ISR, so if any low-power mode has been set before the
* interrupt, it will be restored upon the ISR exit. This is not what
* you want in QP, because it will stop the CPU. Therefore, to prevent
* this from happening, the macro __low_power_mode_off_on_exit() clears
* any low-power mode in the *stacked* CPU status register, so that the
* low-power mode won't be restored upon the ISR exit.
*/
