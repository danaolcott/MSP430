/*
 * 3/12/19
 * MSP430 - Simple OS
 *
 * The goal of this project is to create a super simple
 * OS, including context switching, round robin scheduler,
 * etc.
 *
 */
//includes
#include <msp430g2553.h>
#include <stdio.h>
#include <stdint.h>
#include <stddef.h>
#include <string.h>

#define STACK_SIZE      80


//prototypes
void GPIO_init(void);
void TimerA_init(void);
void LED_RED_TOGGLE(void);
void LED_GREEN_TOGGLE(void);

void TaskFunction1(void);
void TaskFunction2(void);




//////////////////////////////////////////
//Task Control Block
//Defines each task.  SP - stack pointer.
//SP has to be first.  When initializing and
//in the timer isr, it uses the first element to
//manage the task stacks.
//
struct taskBlock
{
    int16_t* sp;                        //SP has to be first!!!
    struct taskBlock* next;
    int16_t stack[STACK_SIZE];
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
void SimpleOS_check(void);


TaskBlock Task[2];
TaskBlock *RunPt;




int16_t* stackPtr1;
int16_t* stackPtr2;
int16_t* stackPtr3;



//main program
int main(void)
{
    //disable the watchdog timer
    WDTCTL = WDTPW + WDTHOLD;

    TimerA_init();
    GPIO_init();

    //os related calls
    SimpleOS_init();
    SimpleOS_addThreads(TaskFunction1, TaskFunction2);
    SimpleOS_launch();

    //while loop - should never make it here
    while(1){};

    return 0;
}


//////////////////////////////////////
//Note: SPIB conflicts with P1.6, so the green led does not work
void GPIO_init(void)
{
    //setup bit 0 and 6 as output
    P1DIR |= BIT0;      //red
    P1DIR |= BIT6;      //green

    P1OUT &=~ BIT0;     //turn off
    P1OUT &=~ BIT6;     //turn off
}

/////////////////////////////////////////
//Sets up the timer And the frequency
//of the clock, since the rate of overflow
//is connected to this

void TimerA_init(void)
{
    BCSCTL1 = CALBC1_16MHZ;
    DCOCTL = CALDCO_16MHZ;

    //Set up Timer A register TACTL
    TACTL = 0x0000;     //start from all all clear
    TACTL |= BIT9;      //use SMCLK as the source
    TACTL |= BIT7;      //use prescaler 8
    TACTL |= BIT6;
    TACTL |= BIT4;      //count up to TACCR0(set below)
    TACTL |= BIT1;      //enable the interrupt
    TACTL &=~ BIT0;     //clear all pending interrupts

    //set the countup value for 1ms delay
    TACCR0 = 2000;      //use 2000 for 16 mhz

    //TACCTL0 - compare capture control register.
    //this has to be set up along with the timer interrupt
    //bit 4 enables the interrupts for this register
    //also called CCIE

    TACCTL0 = CCIE;     //compare, capture interrupt enable

}


///////////////////////////////////////////
void LED_RED_TOGGLE(void)
{
    P1OUT ^= BIT0;
}

///////////////////////////////////////////
void LED_GREEN_TOGGLE(void)
{
    P1OUT ^= BIT6;
}


/////////////////////////////////////
void SimpleOS_init(void)
{
    //enable all the interrupts
//    __bis_SR_register(GIE);

    //disable all the interrupts
    __bic_SR_register(GIE);
}


void SimpleOS_scheduler(void)
{
    RunPt = RunPt->next;
}



//////////////////////////////////////////////////
//Set the initial stack values for each task block
//i is the index in the Task list
void SimpleOS_initStack(uint8_t i)
{
    /////////////////////////////////////////////////
    //PC  - stack_size - 1
    Task[i].stack[STACK_SIZE-1] = (0x1000 + i*(1u << 15));   // R0 - PC
    Task[i].stack[STACK_SIZE-2] = (0x0110 + i*(1u << 15));   // R1 - SP - contents at the address hold the next instruction - shows up in the PC register
    Task[i].stack[STACK_SIZE-3] = (0x0220 + i*(1u << 15));   // R2 - SR
    Task[i].stack[STACK_SIZE-4] = (0x0330 + i*(1u << 15));   // R3 - leave this alone!!! - dont pop anything into this
    Task[i].stack[STACK_SIZE-5] = (0x0FF0 + i*(1u << 15));   // R15 - this is the last pop - 0x0F0F into R15
    Task[i].stack[STACK_SIZE-6] = (0x0EE0 + i*(1u << 15));   // R14
    Task[i].stack[STACK_SIZE-7] = (0x0DD0 + i*(1u << 15));   // R13
    Task[i].stack[STACK_SIZE-8] = (0x0CC0 + i*(1u << 15));   // R12
    Task[i].stack[STACK_SIZE-9] = (0x0BB0 + i*(1u << 15));   // R11
    Task[i].stack[STACK_SIZE-10] = (0x0AA0 + i*(1u << 15));  // R10
    Task[i].stack[STACK_SIZE-11] = (0x0990 + i*(1u << 15));  // R9
    Task[i].stack[STACK_SIZE-12] = (0x0880 + i*(1u << 15));  // R8
    Task[i].stack[STACK_SIZE-13] = (0x0770 + i*(1u << 15));  // R7
    Task[i].stack[STACK_SIZE-14] = (0x0660 + i*(1u << 15));  // R6 - this is the second pop
    Task[i].stack[STACK_SIZE-15] = (0x0550 + i*(1u << 15));  // R5 - After setting the SP, 0404 is the first pop
    Task[i].stack[STACK_SIZE-16] = (0x0440 + i*(1u << 15));  // R4 - After setting the SP, 0404 is the first pop

    //set the stack pointer - see datasheet
    //This needs to be at 16 because on setup and the isr, we pop
    //into a higher address, so we start at the bottom set
    //of the registers
    Task[i].sp = &Task[i].stack[STACK_SIZE - 16]; //SP
}


///////////////////////////////////////////
void SimpleOS_addThreads(void (*functionPtr1)(void), void (*functionPtr2)(void))
{
//    long sr = 0x00;
//    sr = SimpleOS_enterCritical();

    //disable interrupts
   __bic_SR_register(GIE);          //clear the GIE bit in SR

    //configure the linked list of task control blocks
    Task[0].next = &Task[1];    // 0 points to 1
    Task[1].next = &Task[0];    // 1 points to 2

    SimpleOS_initStack(0);      //index of the TaskBlock
    //set the PC register to point to functionPtr1
//    Task[0].stack[STACK_SIZE - 1] = (int16_t)(functionPtr1);
    Task[0].stack[STACK_SIZE-2] = (int16_t)(functionPtr1);      //increment R15 4 times


    SimpleOS_initStack(1);      //index of the task block
    //set the PC register to the function to run
//    Task[1].stack[STACK_SIZE - 1] = (int16_t)(functionPtr2);
    Task[1].stack[STACK_SIZE-2] = (int16_t)(functionPtr2);

    RunPt = &Task[0];

//    SimpleOS_exitCritical(sr);
}

void SimpleOS_launch(void)
{
    //call the timer a config
    SimpleOS_start();
}


void __attribute__((naked))
SimpleOS_start(void)
{
//    __asm("MOV  &RunPt, R4\n");

    //This reuslts in the SP looking at the task SP.  Future pops from here
    //will pop values from the task SP into various target registers.
//    __asm("MOV  @R4, SP\n");

    __asm("MOV &RunPt, SP\n");      //set the location of SP = location of Run PT. Contents at SP = Contents of RunPt
    __asm("MOV @SP, SP\n");         //set the location of SP = contents of SP.

    __asm("POP R4\n");              //set location of R4 = stack - 16
    __asm("POP R5\n");              //set location of R5 = stack - 15
    __asm("POP R6\n");              //set location of R6 = stack - 14
    __asm("POP R7\n");              //set location of R7 = stack - 13
    __asm("POP R8\n");              //set location of R8 = stack - 12
    __asm("POP R9\n");              //set location of R9 = stack - 11
    __asm("POP R10\n");
    __asm("POP R11\n");
    __asm("POP R12\n");
    __asm("POP R13\n");
    __asm("POP R14\n");             //set location of R14 = stack - 6
    __asm("POP R15\n");             //set location of R15 = stack - 5

    __asm("MOV  SP, R15\n");        //SP = 2ae(0330), R15 = 0xFF0(0x5325)  ---- after SP same, R15 = 0x2ae(0x0330)
    __asm("INC R15\n");             //SP - same, R15 = 0x02AF(0x0330 - word aligned)
    __asm("INC R15\n");             //SP - same, R15 = 0x02b0 (0x0220)
    __asm("INC R15\n");
    __asm("INC R15\n");             //results: SP - same 0x02ae(0x0330), R15 = 0x02b2(0xc180)
    __asm("MOV R15, SP\n");         //SP = 0x2b2(0xC180), R15 = 0x2b2(0xc180)


    //This line here - SP is already at the address loaded with
    //a function to run.  Copy it into R15, increment as needed
    //and load it back into SP anyway.
//    __asm("MOV  SP, R15\n");

    //copy it back.  We can to an increment R15
    //ie, INC, R15 if needed.
    //we can increment the contented looked at by
    //R15 by doing a INC, @R15.
//    __asm("MOV R15, SP\n");

    //enable global interrupts - one instruction
    //no impact on the stack / registers
   __bis_SR_register(GIE);          //set the GIE bit in SR

   //After This instruction, PC is loaded with contents
   //of the SP.
    __asm("RET\n");
}




///////////////////////////////////////////////////
//Periodically call CheckStack -
//Push CPU registers onto the stack.
//save the stack pointer.
//push R4
//pop r4
//set the SP = RunPt.sp
//pop stack into CPU registers
//
//The idea is that there should be no change.
//to the stack, and resume where it left off
//
void __attribute__((naked))
SimpleOS_check(void)
{
    __asm("MOV SP, &stackPtr1\n");  //save SP on entry

    __asm("PUSH R5\n");     //sp=0x02b4(0xc3e8), r5 = 0x0550 -> stack grows 2 down, populates 0x02b2 with 0x0550
    __asm("PUSH R6\n");     //sp = 0x02b0 (0x0660)
    __asm("PUSH R7\n");
    __asm("PUSH R8\n");
    __asm("PUSH R9\n");
    __asm("PUSH R10\n");
    __asm("PUSH R11\n");
    __asm("PUSH R12\n");
    __asm("PUSH R13\n");
    __asm("PUSH R14\n");
    __asm("PUSH R15\n");

    __asm("MOV SP, &stackPtr2\n");

    //store the SP into RunPt.sp - before - R4 = 0x0440(0x0000), RunPt* = 0x218, sp = 0x029c
    __asm("MOV  &RunPt, R4\n"); //set R4 = RunPt!!!!   after - RunPt - no change, R4 = 0x218 (0x029c)

    //ie, we use R4 as the transfer mechanism to swap RunPT
    //store SP into RunPt - address and contents
    //SP should be stored in RunPt.sp
    __asm("MOV SP, @R4\n");         //move SP into contents of R4.  r4 now...... R4 = 0x0218(0x029E).  ie, contents at R4 = stackPtr2.  also, RunPt.sp = 0x029E

    __asm("PUSH R4\n");

    //this is where we swap - ie, R4 address should stay same 0x212, but contents should be new RunPt*

    __asm("POP R4\n");

    //set SP = RunPt-SP - this is reloading it from before -
    //R4 should conain RunPt
//    __asm("MOV R4, SP");        //error.  should be the contents of R4, this sets the SP = 218, should be 0x029E
    __asm("MOV @R4, SP");        //Move contents in R4 to SP - ie, set SP = memory contents stored at reg. R4.


    __asm("MOV SP, &stackPtr3\n");

    //pop r5 - r15

    __asm("POP R5\n");              //stack - 16
    __asm("POP R6\n");              //stack - 15
    __asm("POP R7\n");              //stack - 14
    __asm("POP R8\n");              //stack - 13
    __asm("POP R9\n");
    __asm("POP R10\n");
    __asm("POP R11\n");
    __asm("POP R12\n");
    __asm("POP R13\n");
    __asm("POP R14\n");             //stack - 7
    __asm("POP R15\n");             //stack - 7

    //enable all the interrupts
 //   __bis_SR_register(GIE);

    __asm("RET\n");

}












//interrupt routine for using GNU compiler
//void __attribute__((naked))
//__attribute__((interrupt(TIMER0_A0_VECTOR))) Timer_A(void)

__attribute__((interrupt(TIMER0_A0_VECTOR))) void Timer_A(void)
{
    //disable all the interrupts
    __bic_SR_register(GIE);

    //clear the timer interrupt
    TACTL &=~ BIT0;

    SimpleOS_check();


//    __asm("CALL     #SimpleOS_check\n");     //call the scheduler

    /*

    __asm("PUSH R5\n");              //stack - 16
    __asm("PUSH R6\n");              //stack - 15
    __asm("PUSH R7\n");              //stack - 14
    __asm("PUSH R8\n");              //stack - 13
    __asm("PUSH R9\n");
    __asm("PUSH R10\n");
    __asm("PUSH R11\n");
    __asm("PUSH R12\n");
    __asm("PUSH R13\n");
    __asm("PUSH R14\n");             //stack - 7

    __asm("MOV  &RunPt, R4\n");

    //This reuslts in the SP looking at the task SP.  Future pops from here
    //will pop values from the task SP into various target registers.
    __asm("MOV  @R4, SP\n");

    __asm("CALL     #SimpleOS_scheduler\n");     //call the scheduler

    __asm("MOV  @R4, SP\n");


    __asm("POP R5\n");              //stack - 16
    __asm("POP R6\n");              //stack - 15
    __asm("POP R7\n");              //stack - 14
    __asm("POP R8\n");              //stack - 13
    __asm("POP R9\n");
    __asm("POP R10\n");
    __asm("POP R11\n");
    __asm("POP R12\n");
    __asm("POP R13\n");
    __asm("POP R14\n");             //stack - 7

    //enable all the interrupts

    */

    __bis_SR_register(GIE);

    //__asm("RET\n");
}



void TaskFunction1(void)
{
    uint32_t counter = 0x00;

    while (1)
    {
        if (!(counter % 2))
        {
            LED_RED_TOGGLE();

         //   SimpleOS_check();
        }

        counter++;
    }
}

void TaskFunction2(void)
{
    while (1)
    {
        LED_GREEN_TOGGLE();
    }
}


