/*
 * fsm.h
 *
 *  Created on: Dec 22, 2017
 *      Author: danao
 */

#ifndef FSM_FSM_H_
#define FSM_FSM_H_



//////////////////////////////////////////
//FSM State Definitions
//
#define FSM_NUM_STATES			4

//state names - value cooresponds to
//the index of the fsm state table.
typedef enum
{
	STATE0 = 0,
	STATE1 = 1,
	STATE2 = 2,
	STATE3 = 3,
}StateName_t;

typedef struct
{
	StateName_t name;							//state id
	uint16_t delay;								//frequency to run
	void (*fptr)(void);							//function to run
	void (*entryPtr)(void);						//entry function to run
	StateName_t nextState[FSM_NUM_STATES];		//array of next states
}State_t;


//functions available to outside world
void FSM_SetInputValue(uint8_t value);
void FSM_Run(void);




#endif /* FSM_FSM_H_ */
