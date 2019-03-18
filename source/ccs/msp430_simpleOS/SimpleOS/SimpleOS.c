/*
 * SimpleOS.c
 *
 *  Created on: Mar 18, 2019
 *      Author: danao
 */

#include <msp430g2553.h>
#include <stdio.h>
#include <stdint.h>
#include <stddef.h>
#include <string.h>

#include "SimpleOS.h"





//////////////////////////////////////////
//Temporary storage - not required
volatile int16_t* stackPtr1;
volatile int16_t* stackPtr2;
volatile int16_t* stackPtr3;

volatile uint16_t gStatusRegister = 0x00;        //enter/exit critical


///////////////////////////////////////////
//Task Array and pointer to current task
TaskBlock Task[2];
TaskBlock *RunPt;





//////////////////////////////////////////////
//SimpleOS Related Calls
/////////////////////////////////////////////
//SimpleOS_init
//
void SimpleOS_init(void)
{
    __bic_SR_register(GIE);     //disable all interrupts
}


/////////////////////////////////////////////////
//Scheduler
//Called in the SimpleOS_ISR.
void SimpleOS_scheduler(void)
{
    RunPt = RunPt->next;
}



//////////////////////////////////////////////////
//SimpleOS_initStack()
//Set up initial stack values such that when pop
//opperation is called, they align with the appropriate
//registers.
//NOTE: function to run ptr is set in another function,
//but it aligns with Stack_size - 5 using the sequence in
//SimpleOS_start and the context switch.
//i = index in the task array
void SimpleOS_initStack(uint8_t i)
{
    /////////////////////////////////////////////////
    Task[i].stack[STACK_SIZE-1] = (0x1000 + i*(1u << 15));   // R0 - PC
    Task[i].stack[STACK_SIZE-2] = (0x0110 + i*(1u << 15));   // R1 - SP - address of SP holds contents of next instruction when returning from a function
    Task[i].stack[STACK_SIZE-3] = (0x0220 + i*(1u << 15));   // R2 - SR - holds the GIE bit
    Task[i].stack[STACK_SIZE-4] = (0x0330 + i*(1u << 15));   // R3 - Leave this alone.
    Task[i].stack[STACK_SIZE-5] = (0x0FF0 + i*(1u << 15));   // R15 - working reg
    Task[i].stack[STACK_SIZE-6] = (0x0EE0 + i*(1u << 15));   // R14 - working reg
    Task[i].stack[STACK_SIZE-7] = (0x0DD0 + i*(1u << 15));   // R13 -  ....
    Task[i].stack[STACK_SIZE-8] = (0x0CC0 + i*(1u << 15));   // R12
    Task[i].stack[STACK_SIZE-9] = (0x0BB0 + i*(1u << 15));   // R11
    Task[i].stack[STACK_SIZE-10] = (0x0AA0 + i*(1u << 15));  // R10
    Task[i].stack[STACK_SIZE-11] = (0x0990 + i*(1u << 15));  // R9
    Task[i].stack[STACK_SIZE-12] = (0x0880 + i*(1u << 15));  // R8
    Task[i].stack[STACK_SIZE-13] = (0x0770 + i*(1u << 15));  // R7
    Task[i].stack[STACK_SIZE-14] = (0x0660 + i*(1u << 15));  // R6 - working reg
    Task[i].stack[STACK_SIZE-15] = (0x0550 + i*(1u << 15));  // R5 - R5 is the last pop/push
    Task[i].stack[STACK_SIZE-16] = (0x0440 + i*(1u << 15));  // R4 - R4 - used for args.

    //set the initial stack pointer location.  This should be set
    //to allow push/pop operations to not overflow the stack.
    //pop - increment the SP, push - decrement the sp.
    //stack - 16 works with the pop/push routines in initial setup
    //and the context switch.
    Task[i].sp = &Task[i].stack[STACK_SIZE - 16]; //SP
}


///////////////////////////////////////////////////////
//SimpleOS_addThreads.  Add two function to run pointers
void SimpleOS_addThreads(void (*functionPtr1)(void), void (*functionPtr2)(void))
{
    SimpleOS_EnterCritical();

    //disable interrupts
   __bic_SR_register(GIE);          //clear the GIE bit in SR

    //configure linked list of task control blocks
    Task[0].next = &Task[1];    // 0 points to 1
    Task[1].next = &Task[0];    // 1 points to 2

    //initialize the top elements of the stack and set the
    //initial function pointer such that SP lands on it after
    //initialization and/or context switch.

    SimpleOS_initStack(0);
    Task[0].stack[STACK_SIZE-5] = (int16_t)(functionPtr1);

    SimpleOS_initStack(1);
    Task[1].stack[STACK_SIZE-5] = (int16_t)(functionPtr2);

    RunPt = &Task[0];

    SimpleOS_ExitCritical();
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

    __asm("MOV SP, &stackPtr1\n");  //save SP on entry

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
    __asm("MOV SP, &stackPtr2\n");          //save SP
    __asm("MOV  &RunPt, R4\n");             //set R4 = RunPt
    __asm("MOV SP, @R4\n");                 //save SP into contents of R4, setting RunPt->sp = SP

    //update the new RunPt and set the new SP
    __asm("CALL #SimpleOS_scheduler\n");    //update RunPt by calling the scheduler
    __asm("MOV  &RunPt, R4\n");             //set R4 = RunPt
    __asm("MOV @R4, SP");                   //set contents of R4 (RunPt->sp) to SP.  ie, set SP to the new RunPt-sp

    __asm("MOV SP, &stackPtr3\n");          //save the SP

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

    //enable all the interrupts
    __bis_SR_register(GIE);

    //NOTE: The return from interrupt call is RETI
    __asm("RET\n");

}



