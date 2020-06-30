/*
 * SimpleOS.c
 *
 *  Created on: Mar 18, 2019
 *      Author: danao
 *
 *  SimpleOS Controller File.
 *  Works with Task.h/.c.  Manages the OS and it's tasks.
 *  Requires at least one timer to run the scheduler.
 *
 */

#include <msp430g2553.h>
#include <stdio.h>
#include <stdint.h>
#include <stddef.h>
#include <string.h>

#include "SimpleOS.h"
#include "Task.h"

//////////////////////////////////////////
volatile uint16_t gStatusRegister = 0x00;        //enter/exit critical
TaskStruct* RunPt;

volatile uint32_t gTimerTick = 0x00;



//////////////////////////////////////////////
//SimpleOS Related Calls
/////////////////////////////////////////////
//SimpleOS_init
//Disable global interrupts, init the default task
void SimpleOS_init(void (*functionPtr)(void))
{
    __bic_SR_register(GIE);     //disable all interrupts

    //returns the head of the TaskList
    RunPt = Task_init(functionPtr);

}





///////////////////////////////////////////////////////
//SimpleOS_addThread.
//Append new task in the linked task list.  Update the
//RunPt to the head of the list.
//
void SimpleOS_addThread(void (*functionPtr)(void))
{
    SimpleOS_EnterCritical();

    RunPt = Task_appendTask(functionPtr);

    SimpleOS_ExitCritical();
}




/////////////////////////////////////////////////
//Scheduler
//Called in the SimpleOS_ISR.  Configure the
//RunPt to the next task to run.  Check that RunPt
//is not at the end of the list, if so, reset it to the head.
void SimpleOS_scheduler(void)
{
    gTimerTick++;

//    RunPt = RunPt;
    //Update RunPt - check not at the end of the list
    if ((RunPt->next) != NULL)
        RunPt = RunPt->next;

    else
        RunPt = Task_getHead();     //reset to the head
}


/////////////////////////////////////////////
//Start SimpleOS
void SimpleOS_launch(void)
{
    //configure stack and start first task
    SimpleOS_start();
}


/////////////////////////////////////////////////////
//Configure the stack to launch the first task
//map the stack pointer to RunPt-sp, pop the contents
//in the task block into R5-R15.  enable interrupts
void __attribute__((naked))
SimpleOS_start(void)
{
    //Set SP to the contents of RunPt->sp
    __asm("MOV &RunPt, SP\n");      //set SP = address of RunPt
    __asm("MOV @SP, SP\n");         //set SP = contents in SP

    __asm("POP R5\n");              //R5 = stack - 15
    __asm("POP R6\n");              //R6 = stack - 14
    __asm("POP R7\n");              //R7 = stack - 13
    __asm("POP R8\n");              //R8 = stack - 12
    __asm("POP R9\n");              //R9 = stack - 11
    __asm("POP R10\n");             //R10 = stack - 10
    __asm("POP R11\n");             //R11 = stack - 9
    __asm("POP R12\n");             //R12 = stack - 8
    __asm("POP R13\n");             //R13 = stack - 7
    __asm("POP R14\n");             //R14 = stack - 6
    __asm("POP R15\n");             //R15 = stack - 5

    //NOTE: This should result in the address of SP looking at
    //the contents of function pointer. - ie, set FunctionPtr = stack - 5
    //If it's off, adjust the location of the initial function ptr
    //or try the following:
    //   __asm("MOV  SP, R15\n");        //set R15 = SP (address and contents)
    //   __asm("INC R15\n");             //increment R15 - word aligned
    //   __asm("INC R15\n");             //increment R15 - word aligned
    //          ....
    //          ....
    //   __asm("MOV R15, SP\n");         //set SP = R15

   __bis_SR_register(GIE);          //enable global interrupts

   //PC loaded with contents of SP - the address we return to
   __asm("RET\n");
}




//////////////////////////////////////////////////
//SimpleOS_EnterCritical.
//Save the status register R2 and disable interrupts
//I'm guessing this goes in R4 if you want to return it.
void __attribute__((naked))
SimpleOS_EnterCritical(void)
{
    __asm("MOV SR, &gStatusRegister\n");
    __bic_SR_register(GIE);         //disable
    __asm("RET\n");
}



//////////////////////////////////////////////////
//SimpleOS_EnterCritical.
//Save the status register R2 and disable interrupts
//I'm guessing this goes in R4 if you want to return it.
//
//NOTE: Status shows up in R12.  Can't be for sure if it's
//always that way.
void __attribute__((naked))
SimpleOS_ExitCritical(void)
{
    __asm("MOV &gStatusRegister, SR\n");
    __asm("RET\n");
}


//////////////////////////////////////////////////////////
//SimpleOS_ISR
//Called from periodically from a timer interrupt.
//This function performs the context switch.  The process:
//
// - Disable interrupts
// - Push CPU registers onto the stack.
// - Save SP into the current RunPt->sp through use of R4.
// - Update the RunPt by calling the scheduler.
// - Set the new SP  = RunPt-sp through the use of R4.
// - Pop R5-R15 using stack values mapped to new RunPt-sp.
// - Enable interrupts
//
//
void __attribute__((naked))
SimpleOS_ISR(void)
{
    __bic_SR_register(GIE);         //disable interrupts

    //save R5 - R15 into the current RunPt stack
    __asm("PUSH R5\n");
    __asm("PUSH R6\n");
    __asm("PUSH R7\n");
    __asm("PUSH R8\n");
    __asm("PUSH R9\n");
    __asm("PUSH R10\n");
    __asm("PUSH R11\n");
    __asm("PUSH R12\n");
    __asm("PUSH R13\n");
    __asm("PUSH R14\n");
    __asm("PUSH R15\n");

    //Save the old stack pointer
    __asm("MOV  &RunPt, R4\n");             //set R4 = RunPt
    __asm("MOV SP, @R4\n");                 //save SP into contents of R4, setting RunPt->sp = SP

    //update the new RunPt and set the new SP
    __asm("CALL #SimpleOS_scheduler\n");    //update RunPt by calling the scheduler
    __asm("MOV  &RunPt, R4\n");             //set R4 = RunPt
    __asm("MOV @R4, SP");                   //set contents of R4 (RunPt->sp) to SP.  ie, set SP to the new RunPt-sp


    //update R5 - R15 with the contents of new RunPt stack.
    //ie, since SP = RunPt->sp, push / pop opperations
    //work on the new stack location
    __asm("POP R5\n");
    __asm("POP R6\n");
    __asm("POP R7\n");
    __asm("POP R8\n");
    __asm("POP R9\n");
    __asm("POP R10\n");
    __asm("POP R11\n");
    __asm("POP R12\n");
    __asm("POP R13\n");
    __asm("POP R14\n");
    __asm("POP R15\n");

    //NOTE: The timer counter is still counting
    //regardless if interrupts are disabled. Reset
    //the counter here
    __asm("MOV #0, &TA0R\n");

    //enable all the interrupts
    __bis_SR_register(GIE);

    //NOTE: The return from interrupt call is RETI
    __asm("RET\n");
}


/////////////////////////////////////////
//Delay as function of the timeslice.
void SimpleOS_delay(uint32_t delay)
{
    volatile uint32_t temp = delay + gTimerTick;



    while (temp > gTimerTick){};

}


//////////////////////////////////////////////
//Spinlock semaphore wait.
void SimpleOS_semaphoreWait(int16_t* signal)
{
    __bic_SR_register(GIE);         //disable

    while ((*signal) < 1)
    {
        __bis_SR_register(GIE);     //enable
        __bic_SR_register(GIE);     //disable
    }

    (*signal) = (*signal) - 1;
    __bis_SR_register(GIE);         //enable
}



////////////////////////////////////////////////
//Signal the semaphore
void SimpleOS_semaphoreSignal(int16_t* signal)
{
    __bic_SR_register(GIE);         //disable
    (*signal) = (*signal) + 1;
    __bis_SR_register(GIE);         //enable
}




