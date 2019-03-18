/*
 * SimpleOS.h
 *
 *  Created on: Mar 18, 2019
 *      Author: danao
 */

#ifndef SIMPLEOS_SIMPLEOS_H_
#define SIMPLEOS_SIMPLEOS_H_


#define STACK_SIZE      80


//////////////////////////////////////////////////////
//Task Control Block
//sp -    Stack Pointer - stores SP when task not running,
//        mapped to the task stack.
//next -  pointer to the next task to run.
//stack - task stack.  when task is running, CPU SP is
//        mapped here.  Push/pop operates on this stack.
//
struct taskBlock
{
    int16_t* sp;                        //stack pointer - needs to be first
    struct taskBlock* next;             //pointer to next task
    int16_t stack[STACK_SIZE];          //task stack
};

typedef struct taskBlock TaskBlock;

////////////////////////////////////
//OS Stuff
void SimpleOS_init(void);
void SimpleOS_scheduler(void);
void SimpleOS_initStack(uint8_t i);
void SimpleOS_addThreads(void (*functionPtr1)(void), void (*functionPtr2)(void));
void SimpleOS_launch(void);
void SimpleOS_start(void);
void SimpleOS_ISR(void);

void SimpleOS_EnterCritical(void);
void SimpleOS_ExitCritical(void);




#endif /* SIMPLEOS_SIMPLEOS_H_ */
