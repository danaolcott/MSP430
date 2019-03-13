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



struct taskBlock
{
    struct taskBlock* next;
    int16_t* sp;
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




TaskBlock Task[2];
TaskBlock *RunPt;



//main program
int main(void)
{
    //disable the watchdog timer
    WDTCTL = WDTPW + WDTHOLD;

    TimerA_init();
    GPIO_init();

    //os related calls
    SimpleOS_init();
    SimpleOS_addThreads(LED_RED_TOGGLE, LED_GREEN_TOGGLE);
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
    //PC  - stack_size - 1
    Task[i].sp = &Task[i].stack[STACK_SIZE - 2]; //SP

    Task[i].stack[STACK_SIZE-3] = 0x1414;   // R2 - SR
    Task[i].stack[STACK_SIZE-4] = 0x1212;   // R3
    Task[i].stack[STACK_SIZE-5] = 0x0303;   // R4
    Task[i].stack[STACK_SIZE-6] = 0x0202;   // R5
    Task[i].stack[STACK_SIZE-7] = 0x0101;   // R6
    Task[i].stack[STACK_SIZE-8] = 0x0000;   // R7
    Task[i].stack[STACK_SIZE-9] = 0x1111;   // R8
    Task[i].stack[STACK_SIZE-10] = 0x1010;  // R9
    Task[i].stack[STACK_SIZE-11] = 0x0909;  // R10
    Task[i].stack[STACK_SIZE-12] = 0x0808;  // R11
    Task[i].stack[STACK_SIZE-13] = 0x0707;  // R12
    Task[i].stack[STACK_SIZE-14] = 0x0606;  // R13
    Task[i].stack[STACK_SIZE-15] = 0x0505;  // R14
    Task[i].stack[STACK_SIZE-16] = 0x0404;  // R15
}


///////////////////////////////////////////
void SimpleOS_addThreads(void (*functionPtr1)(void), void (*functionPtr2)(void))
{
//    long sr = 0x00;
//    sr = SimpleOS_enterCritical();

    //configure the linked list of task control blocks
    Task[0].next = &Task[1];    // 0 points to 1
    Task[1].next = &Task[0];    // 1 points to 2

    SimpleOS_initStack(0);      //index of the TaskBlock
    //set the PC register to point to functionPtr1
    Task[0].stack[STACK_SIZE - 1] = (int16_t)(functionPtr1);


    SimpleOS_initStack(1);      //index of the task block
    //set the PC register to the function to run
    Task[1].stack[STACK_SIZE - 1] = (int16_t)(functionPtr2);

    RunPt = &Task[0];

//    SimpleOS_exitCritical(sr);
}

void SimpleOS_launch(void)
{
    //call the timer a config
    SimpleOS_start();
}


void SimpleOS_start(void)
{
    __asm("MOV  &RunPt, R4\n");
    __asm("MOV  @R4, R5\n");
    __asm("MOV  @R5, SP\n");

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
    __asm("ADD SP, SP,#2\n");
    __asm("POP SR\n");
    __asm("ADD SP, SP,#2\n");

    //enable all the interrupts
    __bis_SR_register(GIE);

    __asm("RET\n");
}


/*
 *
 * void __attribute__((naked))
SimpleOS_start(void)
{
    __asm(  "LDR     R0, =RunPt\n"  //set R0 to the current task.  tcb element 0 is the sp
            "LDR     R2, [R0]\n"    //set R2 with the contents of R0
            "LDR     SP, [R2]\n"    //set StackPointer (SP) to RunPt (ie, stored tcb.sp) - remapping SP to different stack??
            "POP     {R4-R11}\n"    //R4-R11 showup where?
            "POP     {R0-R3}\n"
            "POP     {R12}\n"
            "ADD     SP,SP,#4\n"
            "POP     {LR}\n"
            "ADD     SP,SP,#4\n"
            "CPSIE   I\n"           //enable interrupts
            "BX      LR\n");        //the PC should be pointing to task function 1
}

 *
 *
 */



///////////////////////////////////////////////////////
//Systick_Handler
//
/*
void __attribute__((naked))
SysTick_Handler(void)
{
    __asm(  "CPSID   I\n"
            "PUSH    {R4-R11}\n"
            "LDR     R0, =RunPt\n"
            "LDR     R1, [R0]\n"
            "STR     SP, [R1]\n"
            "PUSH    {R0, LR}\n"
            "BL      SimpleOS_scheduler\n"
            "POP     {R0, LR}\n"
            "LDR     R1, [R0]\n"
            "LDR     SP, [R1]\n"
            "POP     {R4-R11}\n"
            "CPSIE   I\n"
            "BX      LR\n");
}
*/



//interrupt routine for using GNU compiler
__attribute__((interrupt(TIMER0_A0_VECTOR))) void Timer_A(void)
{
    //clear the timer interrupt
    TACTL &=~ BIT0;

    __asm("PUSH    R6\n");
    __asm("PUSH    R7\n");
    __asm("PUSH    R8\n");
    __asm("PUSH    R9\n");
    __asm("PUSH    R10\n");
    __asm("PUSH    R11\n");

    __asm("MOV     &RunPt, R4\n");      //move address of RunPt into R4
    __asm("MOV     @R4, R5\n");         //move contents of R4 into R5
    __asm("MOV     @R5, SP\n");         //move contents in R5 into SP

    __asm("PUSH    R4\n");              //save R4
    __asm("PUSH    SR\n");              //save SR

    __asm("CALL     #SimpleOS_scheduler\n");     //call the scheduler

    __asm("POP    SR\n");               //restore SR
    __asm("POP    R4\n");               //restore R4

    __asm("MOV     @R4, R5\n");         //move the contents in R4 into R5
    __asm("MOV     @R5, SP\n");         //move the contents in R5 into the stack pointer

    __asm("POP    R6\n");           //pop
    __asm("POP    R7\n");
    __asm("POP    R8\n");
    __asm("POP    R9\n");
    __asm("POP    R10\n");
    __asm("POP    R11\n");

}


