# MSP430

The purpose of this repository is to present several projects built on the MSP430 Launchpad.  All projects were built using Code Composer Studio and the MSP430G2553 processor.  The following is a list of projects with a short description.  Source code for each is located in source/ccs/project.


Project Listing
---------------

- Code Oscillator:  For practice sending CW using a straight key, sideswiper, or bug.  Ground Pin P2.0 to generate a 580 hz square wave out of Pin P2.1.  Green and red leds alternate on key-up and -down.  

- msp430_blink1:  Initializes leds, button as interrupt, spi, and uart interface.  Implements a very simple command handler for uart data.

- msp430_qpn:  A project that uses QP Nano.

- msp430_task: A tasking project that makes use of a single timer and a while loop to create a simple tasker.  The timer is configured to interrupt every 1ms.  Task states are evaluated in the timer isr.  The task files, task.c/.h are portable and can be reused in other applications.

- msp430_tasksig: Tasking project that uses a timer, a while loop, and two tasks.  The project contains a sender task (TXTask) and a receiver task (RXTask).  The sender tasks posts messages to the receiver task via a TaskMessage array, which is a member of the generic task structure.  The receiver task reads all messages in the array when the task runs (ie, clears all messages).  This approach for posting messsages seems to work pretty good, as long as sender tasks don't jam up the receiver for too long.  So far, there are no checks in the timer isr if any receiver tasks are busy processing messages.

- msp_430_MooreFSM:  A project that uses a simple Moore finite state machine.  The project contains 4 states and an input value that takes a value from 0 to 3.  Toggle the state by pressing the user button.  State pattern: input 0 - state0, input 1 - state 1.. etc.  Each state runs the cooresponding state function.  State_t defintion also contains an entry function pointer to run on transition from one state to the next.
