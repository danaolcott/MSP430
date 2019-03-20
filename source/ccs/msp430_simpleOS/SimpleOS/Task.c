/*
 * Task.c
 *
 *  Created on: Mar 18, 2019
 *      Author: danao
 *
 *  Task controller file for use with SimpleOS.
 */

#include <stdio.h>
#include <stdint.h>
#include <stddef.h>
#include <string.h>
#include "Task.h"



/////////////////////////////////////////////////////
//Globals
///////////////////////////////////////////
//Task Array and pointer to current task
TaskStruct TaskList[MAXTHREADS];
TaskStruct* pTaskHead;


//////////////////////////////////////////////////////////
//Task_init
//Initializes all tasks in the allocated array of  task
//blocks to inactivec with no functions to run.
//Allocates one task as the default task
TaskStruct* Task_init(void (*functionPtr)(void))
{
    TaskStruct* myTask;
    pTaskHead = TaskList;

    for (int i = 0 ; i < MAXTHREADS ; i++)
    {
        TaskList[i].sp = NULL;
        TaskList[i].next = NULL;
        memset(TaskList[i].stack, 0x00, STACK_SIZE);
        TaskList[i].index = i;
        TaskList[i].alive = 0;
    }

    //Allocate the default task and initialize the task stack
    myTask = Task_allocateTask();
    Task_initStack(myTask, functionPtr);

    return pTaskHead;
}

///////////////////////////////////////////////////////
//Task_appendTask
//Add a new task to the linked list of TaskStructs.
//Allocate a new task, link the new task, set
//the function to run pointer, return the task head.
TaskStruct* Task_appendTask(void (*functionPtr)(void))
{
    TaskStruct* ptr = pTaskHead;                //head
    TaskStruct* myTask = Task_allocateTask();   //pointer to new task

    //jump to the end of the list...
    while (ptr->next != NULL)
        ptr = ptr->next;

    //link the new node and configure
    ptr->next = myTask;
    myTask->next = NULL;

    //init the stack and set the task function
    Task_initStack(myTask, functionPtr);

    return pTaskHead;
}

//////////////////////////////////////////////////
//Task_allocateTask
//Find the first inactive task in the TaskList,
//set it to active, initialize it.
//Return the address of the allocated task.  If
//none are available, return NULL.
//
TaskStruct* Task_allocateTask(void)
{
    TaskStruct* ptr = NULL;
    for (int i = 0 ; i < MAXTHREADS ; i++)
    {
        if(!(TaskList[i].alive))
        {
            ptr = &TaskList[i];             //set the return address
            TaskList[i].alive = 1;
            TaskList[i].index = i;
            TaskList[i].next = NULL;
            TaskList[i].sp = NULL;
            memset(TaskList[i].stack, 0x00, STACK_SIZE);
            //break out of loop
            i = MAXTHREADS;
            break;
        }
    }

    return ptr;     //returns NULL if cannot allocate
}


/////////////////////////////////////////////
//Traverse the list of active task blocks
//counting the number of active linked tasks.
uint8_t Task_getNumTasks(void)
{
    uint8_t count = 0x00;
    TaskStruct* ptr = pTaskHead;

    if (!(ptr->alive))
        return 0;

    while (ptr->next != NULL)
    {
        count++;
        ptr = ptr->next;
    }

    return count;
}

/////////////////////////////////////////////////////////
//Init the stack
//set the function to run in a position such that after
//pushes/pops in the OS start / context switch routines, it
//lands on the function to run.
void Task_initStack(TaskStruct* task, void (*functionPtr)(void))
{
    //set the function pointer address.
    task->stack[STACK_SIZE-5] = (int16_t)functionPtr;

    /////////////////////////////////////////////////
    task->stack[STACK_SIZE-1] = (0x1000);   // R0 - PC
    task->stack[STACK_SIZE-2] = (0x0110);   // R1 - SP - address of SP holds contents of next instruction when returning from a function
    task->stack[STACK_SIZE-3] = (0x0220);   // R2 - SR - holds the GIE bit
    task->stack[STACK_SIZE-4] = (0x0330);   // R3 - Leave this alone.
    task->stack[STACK_SIZE-6] = (0x0EE0);   // R14 - working reg
    task->stack[STACK_SIZE-7] = (0x0DD0);   // R13 -  ....
    task->stack[STACK_SIZE-8] = (0x0CC0);   // R12
    task->stack[STACK_SIZE-9] = (0x0BB0);   // R11
    task->stack[STACK_SIZE-10] = (0x0AA0);  // R10
    task->stack[STACK_SIZE-11] = (0x0990);  // R9
    task->stack[STACK_SIZE-12] = (0x0880);  // R8
    task->stack[STACK_SIZE-13] = (0x0770);  // R7
    task->stack[STACK_SIZE-14] = (0x0660);  // R6 - working reg
    task->stack[STACK_SIZE-15] = (0x0550);  // R5 - R5 is the last pop/push
    task->stack[STACK_SIZE-16] = (0x0440);  // R4 - R4 - used for args.

    //set the initial stack pointer location.  This should be set
    //to allow push/pop operations to not overflow the stack.
    //pop - increment the SP, push - decrement the sp.
    //stack - 16 works with the pop/push routines in initial setup
    //and the context switch.
    task->sp = &task->stack[STACK_SIZE - 16]; //SP
}

TaskStruct* Task_getHead(void)
{
    return pTaskHead;
}





