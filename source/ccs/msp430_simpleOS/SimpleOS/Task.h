/*
 * Task.h
 *
 *  Created on: Mar 18, 2019
 *      Author: danao
 *
 *  Task controller file for use with SimpleOS
 *  General approach:
 *  Allocate array of task control blocks in initialization.
 *  Append tasks as active as needed using a call to append task.
 *
 */

#ifndef SIMPLEOS_TASK_H_
#define SIMPLEOS_TASK_H_

#include <stdio.h>
#include <stdint.h>
#include <stddef.h>
#include <string.h>


#define MAXTHREADS      4
#define STACK_SIZE     48


//////////////////////////////////////////////////////
//Task Control Block
//sp -    Stack Pointer - stores SP when task not running,
//        mapped to the task stack.
//next -  pointer to the next task to run.
//stack - task stack.  when task is running, CPU SP is
//        mapped here.  Push/pop operates on this stack.
//
struct taskStruct
{
    int16_t* sp;                        //stack pointer - needs to be first
    struct taskStruct* next;             //pointer to next task
    int16_t stack[STACK_SIZE];          //task stack
    uint8_t index;                      //index in the task list
    uint8_t alive;                      //allocated / not allocated
};

typedef struct taskStruct TaskStruct;


TaskStruct* Task_init(void (*functionPtr)(void));
TaskStruct* Task_appendTask(void (*functionPtr)(void));
TaskStruct* Task_allocateTask(void);

uint8_t Task_getNumTasks(void);
void Task_initStack(TaskStruct* task, void (*functionPtr)(void));
TaskStruct* Task_getHead(void);




#endif /* SIMPLEOS_TASK_H_ */
