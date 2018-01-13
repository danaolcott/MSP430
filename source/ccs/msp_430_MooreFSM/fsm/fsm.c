/*
 * fsm.c
 *
 *  Created on: Dec 22, 2017
 *      Author: danao
 *
 *  A Simple Moore State Machine.
 *
 *  The fsm contains 4 states.  Each state has an
 *  entry and continuous run function.  The current
 *  state is updated from the outside world using
 *  FSM_SetState function.  The state table is defined
 *  below.
 */
#include <stdint.h>
#include <stddef.h>


#include "fsm.h"
#include "main.h"


static volatile StateName_t flashState = STATE0;

//////////////////////////////////
//State Functions - entry - run
//only 1 time on each entry event
static void LED_Flash0_entry(void);
static void LED_Flash1_entry(void);
static void LED_Flash2_entry(void);
static void LED_Flash3_entry(void);

///////////////////////////////////
//State functions - continuous
static void LED_Flash0(void);
static void LED_Flash1(void);
static void LED_Flash2(void);
static void LED_Flash3(void);




static const State_t fsm[FSM_NUM_STATES] =
{
		{STATE0, 10, LED_Flash0, LED_Flash0_entry, {STATE1, STATE2, STATE3, STATE0}},
		{STATE1, 10, LED_Flash1, LED_Flash1_entry, {STATE2, STATE3, STATE3, STATE0}},
		{STATE2, 10, LED_Flash2, LED_Flash2_entry, {STATE3, STATE0, STATE1, STATE2}},
		{STATE3, 10, LED_Flash3, LED_Flash3_entry, {STATE0, STATE1, STATE2, STATE3}},
};


//////////////////////////////////////
//Pass input value into state machine
//
void FSM_SetInputValue(uint8_t value)
{
	if (value < FSM_NUM_STATES)
		flashState = (StateName_t)value;
}

void FSM_Run(void)
{
	State_t currentState = fsm[flashState];
	State_t lastState = fsm[FSM_NUM_STATES -1];	//set to force update on first run

	//main loop
	while(1)
	{
		StateName_t next = currentState.nextState[flashState];
		currentState = fsm[next];

		//state change?
		if(lastState.name != currentState.name)
			currentState.entryPtr();

		//run the state function
		currentState.fptr();
		delay_ms(currentState.delay);

		//update last state
		lastState = fsm[next];
	}
}





////////////////////////////////
//State Function - State0 entry
void LED_Flash0_entry(void)
{
	int i;
	for (i = 0 ; i < 10; i++)
	{
		LedRed_Toggle();
		delay_ms(50);
	}
}

////////////////////////////////
//State Function - State1 entry
//
void LED_Flash1_entry(void)
{
	int i;
	for (i = 0 ; i < 10; i++)
	{
		LedGreen_Toggle();
		delay_ms(50);
	}
}

////////////////////////////////
//State Function - State2 entry
//
void LED_Flash2_entry(void)
{
	int i;
	for (i = 0 ; i < 10; i++)
	{
		LedRed_Toggle();
		LedGreen_Toggle();

		delay_ms(50);
	}
}

////////////////////////////////
//State Function - State3 entry
//
void LED_Flash3_entry(void)
{
	int i;
	for (i = 0 ; i < 10; i++)
	{
		LedRed_Toggle();
		LedGreen_Toggle();
		delay_ms(50);
	}
}




///////////////////////////////
//State function - state0
void LED_Flash0(void)
{
	LedRed_Set();
	delay_ms(100);
	LedRed_Clear();
	delay_ms(100);
}

///////////////////////////////
//State function - state1
void LED_Flash1(void)
{
	LedGreen_Set();
	delay_ms(100);

	LedGreen_Clear();
	delay_ms(100);
}

///////////////////////////////
//State function - state2
void LED_Flash2(void)
{
	LedRed_Set();
	LedGreen_Set();
	delay_ms(100);

	LedRed_Clear();
	LedGreen_Clear();
	delay_ms(100);
}

///////////////////////////////
//State function - state3
void LED_Flash3(void)
{
	LedRed_Set();
	LedGreen_Set();
	delay_ms(500);

	LedRed_Clear();
	LedGreen_Clear();
	delay_ms(500);
}

