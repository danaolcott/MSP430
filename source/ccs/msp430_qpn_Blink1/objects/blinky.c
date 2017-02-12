/*****************************************************************************
* Product: Simple Blinky example
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
//from now on, Blinky is the red led.  it's going to

//post stuff to BlinkyGreen, another AO in the system
//
//so from now on, Blinky will not do anything with the
//green led, but it will contain basic states of on and off
//and a timer.



#include "qpn_port.h"
#include "bsp.h"
#include "main.h"
#include "tft_lcd.h"

#include <msp430g2553.h> /* MSP430 variant used on MSP-EXP430G2 LaunchPad */

//Q_DEFINE_THIS_FILE

///////////////////////////////////////////
//Local Objects
typedef struct BlinkyTag { /* the Blinky active object */
    QActive super;      /* derive from QActive */
    uint8_t count;
} Blinky;


///////////////////////////////////
//Global Object
Blinky AO_Blinky;


////////////////////////////////////////
/* hierarchical state machine ... */
static QState Blinky_initial(Blinky * const me);
static QState Blinky_off    (Blinky * const me);
static QState Blinky_on     (Blinky * const me);


/////////////////////////////////////////////
//constructor function
void Blinky_ctor(void)
{
	Blinky *me = &AO_Blinky;
    QActive_ctor(&me->super, Q_STATE_CAST(&Blinky_initial));
    me->count = 0;

}

/* HSM definition ----------------------------------------------------------*/
QState Blinky_initial(Blinky * const me) {

	//set up the initial state of leds
	P1OUT &=~ BIT0;
	P1OUT &=~ BIT6;

    return Q_TRAN(&Blinky_off);
}
/*..........................................................................*/
QState Blinky_off(Blinky * const me) {
    QState status;
    switch (Q_SIG(me)) {
        case Q_ENTRY_SIG: {

        	me->count++;		//index the counter

        	//arm the timer.
        	QActive_armX((QActive *)me, 0U, BSP_TICKS_PER_SEC/2U);

        	P1OUT &=~ BIT0;
        	P1OUT |= BIT6;

            status = Q_HANDLED();
            break;
        }

        case Q_EXIT_SIG:
        {
            status = Q_HANDLED();
            break;
        }

        //posted from the button
        case BUTTON_PRESS_SIG:
        {
            status = Q_TRAN(&Blinky_on);
            break;

        }
        case Q_TIMEOUT_SIG:
        {
        	//toggle one of the leds
        	P1OUT ^= BIT0;

        	//rearm the timer
        	QActive_armX((QActive *)me, 0U, BSP_TICKS_PER_SEC/2U);

            status = Q_HANDLED();
            break;
        }


        default: {
            status = Q_SUPER(&QHsm_top);
            break;
        }
    }
    return status;
}
/*..........................................................................*/
QState Blinky_on(Blinky * const me)
{
    QState status;
    switch (Q_SIG(me))
    {
        case Q_ENTRY_SIG:
        {
        	P1OUT |= BIT0;
        	P1OUT &=~BIT6;
        	//arm the timer
        	QActive_armX((QActive *)me, 0U, BSP_TICKS_PER_SEC/10U);

            status = Q_HANDLED();
            break;
        }

        case Q_EXIT_SIG:
        {
            status = Q_HANDLED();
            break;
        }

        case Q_TIMEOUT_SIG:
        {
        	P1OUT ^= BIT6;
        	//arm the timer
        	QActive_armX((QActive *)me, 0U, BSP_TICKS_PER_SEC/10U);

            status = Q_HANDLED();
            break;
        }

        //button signal from AO_Button contained a 32bit value
        //decode the lower 8 bits, number of button clicks
        case BUTTON_PRESS_SIG:
        {
        	uint8_t numClicks = (uint8_t)Q_PAR(me);

        	if (numClicks %2 == 1)
        	{
        		P1OUT ^= BIT0;
        	}

            status = Q_TRAN(&Blinky_off);
            break;

        }
        default: {
            status = Q_SUPER(&QHsm_top);
            break;
        }
    }

    return status;
}



