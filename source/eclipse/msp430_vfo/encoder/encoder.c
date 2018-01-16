//////////////////////////////////////////////
/*
Dana Olott
1/14/18

Rotary Encoder Files
P2.0 and P2.1

*/
////////////////////////////////////////////

#include <stdint.h>
#include <msp430.h>
#include "encoder.h"
#include "si5351.h"
#include "task.h"


static uint8_t encoderCurrentState;
static uint8_t encoderLastState;

void encoder_delay(uint16_t delay)
{
	volatile uint16_t time = delay;
	while (time > 0)
		time--;
}


void encoder_init(void)
{
    //configure pins - p2.0, p2.1 as input, pullup
	P2DIR &=~ BIT0;		//input
	P2REN |= BIT0;		//enable pullup/down
	P2OUT |= BIT0;		//resistor set to pull up

	P2DIR &=~ BIT1;		//input
	P2REN |= BIT1;		//enable pullup/down
	P2OUT |= BIT1;		//resistor set to pull up

    //encoder bits are on interrupts
	//enable all the interrupts
	__bis_SR_register(GIE);

	P2IE |= BIT0;		//enable bit 0 interrupt
	P2IE |= BIT1;		//enable bit 1 interrupt

	P2IFG &=~ BIT0;		//clear the flag
	P2IFG &=~ BIT1;		//clear the flag

    encoderCurrentState = encoder_read();
    encoderLastState = encoder_read();

}

///////////////////////////////////
//reads the encoder bits 0 and 1 on
//port 2.
//
uint8_t encoder_read(void)
{
    uint8_t val = P2IN & BIT0;
    val |= (P2IN & BIT1);

    return val;
}


///////////////////////////////////////////
//Get the encoder direction.  The direction
//should be based on the last state and
//the current state.  Store last state 
//Encoders should track sequentially
EncoderDirection_t encoder_getDirection(void)
{
	EncoderDirection_t dir = ENCODER_DIR_NONE;

    //read current state
    encoderCurrentState = encoder_read();

    switch(encoderLastState)
    {
		case 0x00:		//3, 0, 1
		{
			if(encoderCurrentState == 0x03)
				dir = ENCODER_DIR_LEFT;
			else if(encoderLastState == 0x01)
				dir = ENCODER_DIR_RIGHT;
			else
				dir = ENCODER_DIR_NONE;
			break;
		}
		case 0x01:		//0, 1, 2
		{
			if(encoderCurrentState == 0x00)
				dir = ENCODER_DIR_LEFT;
			else if(encoderLastState == 0x02)
				dir = ENCODER_DIR_RIGHT;
			else
				dir = ENCODER_DIR_NONE;
			break;
		}
		case 0x02:		//1, 2, 3
		{
			if(encoderCurrentState == 0x01)
				dir = ENCODER_DIR_LEFT;
			else if(encoderLastState == 0x03)
				dir = ENCODER_DIR_RIGHT;
			else
				dir = ENCODER_DIR_NONE;
			break;
		}
		case 0x03:		//2, 3, 0
		{
			if(encoderCurrentState == 0x02)
				dir = ENCODER_DIR_LEFT;
			else if(encoderLastState == 0x00)
				dir = ENCODER_DIR_RIGHT;
			else
				dir = ENCODER_DIR_NONE;
			break;
		}
		default:
		{
			dir = ENCODER_DIR_NONE;
			break;
		}
    }

    encoderLastState = encoderCurrentState;

    return dir;

}



////////////////////////////////////
//Interrupt Service Routines
//PORT2 ISR - P2.0 and P2.1
//
#pragma vector = PORT2_VECTOR
__interrupt void Port_2(void)
{
	//dummy delay - kill a few cpu cycles
	encoder_delay(1000);

	//get the encoder direction - read bits
	//and compare with last location
	EncoderDirection_t dir = encoder_getDirection();

	//send message to the receiver task with
	//TASK_SIG_###
	TaskMessage msg;
	msg.signal = TASK_SIG_ENCODER_LEFT;
	msg.value = 0x00;

	if (dir == ENCODER_DIR_LEFT)
		msg.signal = TASK_SIG_ENCODER_LEFT;
	else if (dir == ENCODER_DIR_RIGHT)
		msg.signal = TASK_SIG_ENCODER_RIGHT;

	int index = Task_GetIndexFromName("rxTask");

	Task_SendMessage(index, msg);



	//clear the interrupt flags
	P2IFG &=~ BIT0;
	P2IFG &=~ BIT1;

}







