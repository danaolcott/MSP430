//////////////////////////////////////////////
/*
Dana Olott
1/14/18

Rotary Encoder Files
P2.0 and P2.1

Looking at the rotary encoder on the scope,
Here's the bit pattern - CCW
0	0
0	1
1	1
1	0
0	0...

Bit Pattern - CW
0	0
1	0
1	1
0	1
0	0...


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

//////////////////////////////////////////
//Rotary Encoder init
//Configure Pins 2.3, 2.4, 2.5 as input
//interrupted.  2.3/2.4 are used for the
//left and right bits, 2.5 is the push
//button.
void encoder_init(void)
{
	P2DIR &=~ ENCODER_BIT0_PIN;		//input
	P2REN |= ENCODER_BIT0_PIN;		//enable pullup/down
	P2OUT |= ENCODER_BIT0_PIN;		//resistor set to pull up

	P2DIR &=~ ENCODER_BIT1_PIN;		//input
	P2REN |= ENCODER_BIT1_PIN;		//enable pullup/down
	P2OUT |= ENCODER_BIT1_PIN;		//resistor set to pull up

	P2DIR &=~ ENCODER_BUTTON_PIN;		//input
	P2REN |= ENCODER_BUTTON_PIN;		//enable pullup/down
	P2OUT |= ENCODER_BUTTON_PIN;		//resistor set to pull up


    //encoder bits are on interrupts
	//enable all the interrupts
	__bis_SR_register(GIE);

	P2IE |= ENCODER_BIT0_PIN;		//enable bit 0 interrupt
	P2IE |= ENCODER_BIT1_PIN;		//enable bit 1 interrupt
	P2IE |= ENCODER_BUTTON_PIN;		//enable bit 1 interrupt

	P2IFG &=~ ENCODER_BIT0_PIN;		//clear the flag
	P2IFG &=~ ENCODER_BIT1_PIN;		//clear the flag
	P2IFG &=~ ENCODER_BUTTON_PIN;		//clear the flag

    encoderCurrentState = encoder_read();
    encoderLastState = encoder_read();

}

///////////////////////////////////
//reads the encoder bits 0 and 1 on
//port 2.  Located at bits 3 and 4
//
uint8_t encoder_read(void)
{
    uint8_t val = P2IN & BIT4;
    val |= (P2IN & BIT3);
    val = val >> 3;

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
		case 0x00:		//1, 0, 2
		{
			if(encoderCurrentState == 0x01)
				dir = ENCODER_DIR_LEFT;
			else if(encoderCurrentState == 0x02)
				dir = ENCODER_DIR_RIGHT;
			else
				dir = ENCODER_DIR_NONE;
			break;
		}
		case 0x01:		//3, 1, 0
		{
			if(encoderCurrentState == 0x03)
				dir = ENCODER_DIR_LEFT;
			else if(encoderCurrentState == 0x00)
				dir = ENCODER_DIR_RIGHT;
			else
				dir = ENCODER_DIR_NONE;
			break;
		}
		case 0x02:		//0, 2, 3
		{
			if(encoderCurrentState == 0x00)
				dir = ENCODER_DIR_LEFT;
			else if(encoderCurrentState == 0x03)
				dir = ENCODER_DIR_RIGHT;
			else
				dir = ENCODER_DIR_NONE;
			break;
		}
		case 0x03:		//2, 3, 1
		{
			if(encoderCurrentState == 0x02)
				dir = ENCODER_DIR_LEFT;
			else if(encoderCurrentState == 0x01)
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
//PORT2 ISR - Encoder pins P2.3, P2.4,
//and P2.5.
//any of these will trigger the
//interrupt.  Leave button unconnected
//for now.
//
#pragma vector = PORT2_VECTOR
__interrupt void Port_2(void)
{
	//dummy delay - kill a few cpu cycles
	encoder_delay(2000);

	//check the encoder button, if low...
	if (!(P2IN & BIT5))
	{
		TaskMessage msg;
		msg.signal = TASK_SIG_ENCODER_BUTTON;
		int index = Task_GetIndexFromName("rxTask");
		Task_SendMessage(index, msg);

	}

	else
	{
		//get the encoder direction - read bits
		//and compare with last location
		EncoderDirection_t dir = encoder_getDirection();

		uint8_t okFlag = 1;

		//send message to the receiver task with
		//TASK_SIG_###
		TaskMessage msg;
		msg.signal = TASK_SIG_ENCODER_LEFT;

		if (dir == ENCODER_DIR_LEFT)
			msg.signal = TASK_SIG_ENCODER_LEFT;
		else if (dir == ENCODER_DIR_RIGHT)
			msg.signal = TASK_SIG_ENCODER_RIGHT;
		else
			okFlag = 0;

		if (okFlag == 1)
		{
			int index = Task_GetIndexFromName("rxTask");
			Task_SendMessage(index, msg);
		}

	}

	//clear the interrupt flags
	P2IFG &=~ ENCODER_BIT0_PIN;
	P2IFG &=~ ENCODER_BIT1_PIN;
	P2IFG &=~ ENCODER_BUTTON_PIN;

}







