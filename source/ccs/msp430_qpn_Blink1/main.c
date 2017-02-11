/*
 * main.c
 *
 *  This file contains the following:
 *
 *  Event queues for each AO
 *  Array of control blocks for each AO in the system
 *  Some other stuff
 *  Main entry point that does the following:
 *  	init all low level hardware
 *  	init blinky
 *  	init button
 *  	run QPN
 *
 */

///////////////////////////////
//includes
#include "qpn_port.h"
#include "main.h"
#include "bsp.h"

#include "tft_lcd.h"

///////////////////////////////////
//Event Queues
//Define the event queue size for each AO
//
static QEvt l_blinkyQSto[5]; /* Event queue storage for Blinky */
static QEvt l_buttonQSto[5];		//button event queue

//////////////////////////////////
//Control Blocks for each AO in the system
//arranged in order of priority

/* QF_active[] array defines all active object control blocks --------------*/
//
//these are the relative priorities for various AOs in the
//system.  We only have one AO, so it's ok to put it here, but
//if you had more that one, you might want to make a diff
//file for this.  AO's are listed in the control block
//based on thier relative priorities.  The zero index
//is zero priority and reserved.  the highest index is the
//highest priority

QActiveCB const Q_ROM QF_active[] = {
    { (QActive *)0,           		(QEvt *)0,        	0U                      },	//this has to be zero for each entry
    { (QActive *)&AO_Button,   		l_buttonQSto,     	Q_DIM(l_buttonQSto)     },	//button - low
    { (QActive *)&AO_Blinky,   		l_blinkyQSto,     	Q_DIM(l_blinkyQSto)     }	//blinky - high
};

/* make sure that the QF_active[] array matches QF_MAX_ACTIVE in qpn_port.h */
Q_ASSERT_COMPILE(QF_MAX_ACTIVE == Q_DIM(QF_active) - 1);



//////////////////////////////////////////////////////////////////////////////
//main program
int main(void)
{
    BSP_init();           	//init hardware, lcd, etc
    Blinky_ctor();  		//init ao blinky
    Button_ctor();			//init ao button
    return QF_run();        //run
}

