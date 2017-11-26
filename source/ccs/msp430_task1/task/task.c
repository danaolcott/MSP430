/*
 * task.c
 *
 *  Created on: Nov 25, 2017
 *      Author: danao
 */

#include <stdint.h>

#include "task.h"

static TaskStruct TaskTable[TASK_MAX_TASK];

///////////////////////////////////////
//Init task table with default values
void Task_Init(void)
{
	uint8_t i;
	for (i = 0 ; i < TASK_MAX_TASK ; i++)
	{
		TaskTable[i].taskFunction = NULL;
		TaskTable[i].timer = 0;
		TaskTable[i].flagEnable = 0;
		TaskTable[i].flagRun = 0;
		TaskTable[i].index = i;
		TaskTable[i].initialTimeTick = 0;
	}
}

/////////////////////////////////////////////
//Task_AddTask
//Add a task to the task table.  needs to fit
//within the size, priority = index in the table
//*task is the task to run
int Task_AddTask(void (*taskFunction) (void), uint16_t time, uint8_t priority)
{
	//check priority and if the task function is set up
	if (priority < TASK_MAX_TASK)
	{
		if (TaskTable[priority].taskFunction == NULL)
		{
			TaskTable[priority].flagEnable = 1;					//task enable
			TaskTable[priority].flagRun = 0;					//set on timeout, polled in main
			TaskTable[priority].index = priority;				//
			TaskTable[priority].initialTimeTick = time;			//set the timeout
			TaskTable[priority].taskFunction = taskFunction;	//function pointer
			TaskTable[priority].timer = time;					//timeout

			return 1;
		}
	}

	return -1;
}

///////////////////////////////////////
//loop through the task table until
//we hit taskFunction and reset all the
//values for that entry
//pass the task function as arg
int Task_RemoveTask(void (*taskFunction) (void))
{
	uint8_t i;
	for (i = 0 ; i < TASK_MAX_TASK ; i++)
	{
		if (taskFunction == TaskTable[i].taskFunction)
		{
			TaskTable[i].taskFunction = NULL;
			TaskTable[i].timer = 0;
			TaskTable[i].flagEnable = 0;
			TaskTable[i].flagRun = 0;
			TaskTable[i].index = i;
			TaskTable[i].initialTimeTick = 0;

			return i;
		}
	}

	return -1;		//invalid function
}

void Task_EnableTask(uint8_t taskIndex)
{
	if (taskIndex < TASK_MAX_TASK)
	{
		//check null function
		if (TaskTable[taskIndex].taskFunction != NULL)
		{
			TaskTable[taskIndex].flagEnable = 1;
			TaskTable[taskIndex].flagRun = 0;
			TaskTable[taskIndex].initialTimeTick = TaskTable[taskIndex].timer;
		}
	}
}

void Task_DisableTask(uint8_t taskIndex)
{
	if (taskIndex < TASK_MAX_TASK)
	{
		//check null function
		if (TaskTable[taskIndex].taskFunction != NULL)
		{
			TaskTable[taskIndex].flagEnable = 0;
			TaskTable[taskIndex].flagRun = 0;
			TaskTable[taskIndex].initialTimeTick = 0;
		}
	}
}

////////////////////////////////////////////////
//update the timeout value and the initial tick
void Task_RescheduleTask(uint8_t taskIndex, uint16_t updatedTime)
{
	if (taskIndex < TASK_MAX_TASK)
	{
		//check null function
		if (TaskTable[taskIndex].taskFunction != NULL)
		{
			TaskTable[taskIndex].timer = updatedTime;
			TaskTable[taskIndex].initialTimeTick = updatedTime;
		}
	}
}


/////////////////////////////////////////
//Main loop routine.  should never
//stop.  called in main.
void Task_StartScheduler(void)
{
	uint8_t i = 0;

	while (1)
	{
		//run the tasker
		for (i = 0; i < TASK_MAX_TASK ; i++)
		{
			if (TaskTable[i].taskFunction != NULL)
			{
				if (TaskTable[i].flagEnable == 1)
				{
					if (TaskTable[i].flagRun == 1)
					{
						//task to run
						TaskTable[i].flagRun = 0;

						TaskTable[i].taskFunction();
						break;
					}
				}
			}
		}
	}

}


//////////////////////////////////////
//Call this function in the timer isr
//Configure the timer to rollover
//at same speed, 2x, 3x... as the rate
//we want to run the tasker.

//Loop over the task table, decrementing
//the timeTick and set the run flag for
//those timers that have rolled over

void Task_TimerISRHandler(void)
{
	uint8_t i;
	for (i = 0 ; i < TASK_MAX_TASK ; i++)
	{
		//check task enabled, function != NULL
		if ((TaskTable[i].flagEnable == 1) && (TaskTable[i].taskFunction != NULL))
		{
			if (!TaskTable[i].initialTimeTick)
			{
				//set the run flag polled in main,
				//reset the timer tick value
				TaskTable[i].flagRun = 1;
				TaskTable[i].initialTimeTick = TaskTable[i].timer;
			}

			else
				TaskTable[i].initialTimeTick--;
		}
	}
}
