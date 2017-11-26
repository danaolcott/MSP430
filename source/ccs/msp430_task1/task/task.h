/*
 * task.h
 *
 *  Created on: Nov 25, 2017
 *      Author: danao
 *
 *
 */

#ifndef TASK_TASK_H_
#define TASK_TASK_H_

#include <stdint.h>			//uint32_t..etc

#define TASK_MAX_TASK		10
#define NULL				((void *)0)


//task structure
typedef struct
{
	uint16_t initialTimeTick;	//countdown value
	uint16_t timer;				//frequency to run task
	uint8_t flagRun;			//running or not running
	uint8_t flagEnable;			//enable task
	uint8_t index;				//index in the task table
	void (* taskFunction) (void);		//function pointer

}TaskStruct;



//function prototypes
void Task_Init(void);
int Task_AddTask(void (*taskFunction) (void), uint16_t time, uint8_t priority);
int Task_RemoveTask(void (*taskFunction) (void));
void Task_EnableTask(uint8_t taskIndex);
void Task_DisableTask(uint8_t taskIndex);
void Task_RescheduleTask(uint8_t taskIndex, uint16_t updatedTime);
void Task_StartScheduler(void);
void Task_TimerISRHandler(void);



#endif /* TASK_TASK_H_ */
