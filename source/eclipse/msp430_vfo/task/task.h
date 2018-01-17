/*
 * task.c
 *
 *  Created on: Nov 25, 2017
 *      Author: danao
 *
 *  Extension of a simple tasker to include
 *  message handling from one task to another.
 *
 */
///////////////////////////////////////////////////////
/*
Simple Tasker using a timer and a while loop.  One should
be able to implement this on any processor that has a timer
with ability to produce an interrupt at a known tick rate.

How to use:

On the main program, initialize timer and other hardware
In the timer isr, call the following function: Task_TimerISRHandler()

Initialize tasks using Task_AddTask() - requires name, function, period, priority.
Tasks can signal one another using Task_SendMessage().  Update TaskSignal_t
with the appropriate messages.  Each task contains it's own task message "stack"?
, which is simply an array of TaskMessage structs.  Task message struct
can be updated as needed to contain additional members.  The 16bit value
in the task message struct is generic value and can be used for anything.

For sending messages, you need a sender and reciever.

//////////////////////////////////////////////////
Ex: Send TASK_SIG_TOGGLE to "taskRx"

Sender Task Code:

TaskMessage msg = {TASK_SIG_TOGGLE, 0x00};			//make a message
uint8_t index = Task_GetIndexFromName("taskRx");	//get the index of the rx task
Task_SendMessage(index, msg);						//send the message



Receiver Task Code:

TaskMessage msg = {TASK_SIG_NONE, 0x00};			//make a msg struct
uint8_t index = Task_GetIndexFromName("taskRx");	//get the index of the rx task
while (Task_GetNextMessage(index, &msg) > 0)		//get all messages
{
	//do something depending on the signal,
	//value, etc
	switch(msg.signal)
	{
		//blah, blah....
	}

}


Note: Receive messages all get processed the next
time the task runs.


 */
//////////////////////////////////////////////////////


#ifndef TASK_TASK_H_
#define TASK_TASK_H_

#include <stdint.h>			//uint32_t..etc

#define TASK_MAX_TASK		4		//max number of tasks
#define NULL_PTR			((void *)0)
#define TASK_NAME_LENGTH	8		//num chars in the name - max
#define TASK_MESSAGE_SIZE	8		//max messages in the msg queue



/////////////////////////////////
//Task Signal IDs.  This is a generic
//list that applies to all tasks.
//Ideally, it would be cool to change this
//so that each task contained a pointer
//to a message id list.
//
typedef enum
{
	TASK_SIG_NONE,			//do nothing
	TASK_SIG_ON,			//on
	TASK_SIG_OFF,			//off
	TASK_SIG_TOGGLE,		//toggle
	TASK_SIG_ENCODER_LEFT,	//encoder left
	TASK_SIG_ENCODER_RIGHT,	//encoder right
	TASK_SIG_USER_BUTTON,



	TASK_SIG_LAST,		//need this one???

}TaskSignal_t;


typedef struct
{
	TaskSignal_t signal;
}TaskMessage;


//task structure
typedef struct
{
	char name[TASK_NAME_LENGTH];//task name, null terminated for ref
	uint16_t initialTimeTick;	//countdown value
	uint16_t timer;				//frequency to run task
	uint8_t flagRun;			//running or not running
	uint8_t flagEnable;			//enable task
	uint8_t index;				//index in the task table

	//task functions, signals, etc
	void (* taskFunction) (void);				//function pointer - function to run
	uint8_t taskMessageWaiting;					//how many messages waiting
	TaskMessage taskMessage[TASK_MESSAGE_SIZE];	//task message queue - unwind when task runs

}TaskStruct;



//function prototypes
void Task_Init(void);
int Task_AddTask(char* name, void (*taskFunction) (void), uint16_t time, uint8_t priority);
int Task_RemoveTask(void (*taskFunction) (void));
void Task_EnableTask(uint8_t taskIndex);
void Task_DisableTask(uint8_t taskIndex);
void Task_RescheduleTask(uint8_t taskIndex, uint16_t updatedTime);

int Task_GetIndexFromName(char* name);

void Task_StartScheduler(void);
void Task_TimerISRHandler(void);

//messages
int Task_ClearAllMessages(uint8_t element);					//helper function on init/remove, etc
int Task_SendMessage(uint8_t index, TaskMessage message);
int Task_GetNumMessageWaiting(uint8_t index);
int Task_GetNextMessage(uint8_t index, TaskMessage *msg);




#endif /* TASK_TASK_H_ */
