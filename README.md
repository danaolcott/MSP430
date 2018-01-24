# MSP430

The purpose of this repository is to present several projects built on the MSP430 Launchpad.  All projects were built using Code Composer Studio or Eclipse using msp430-gcc toolchain.  The target processor is the MSP430G2553.  The following is a list of projects with a short description.  Source code for each is located in source/ccs/project or source/eclipse/project.   


Project Listing (CCS)
---------------------

- Code Oscillator:  For practice sending CW using a straight key, sideswiper, or bug.  Ground Pin P2.0 to generate a 580 hz square wave out of Pin P2.1.  Green and red leds alternate on key-up and -down.  

- msp430_blink1:  Initializes leds, button as interrupt, spi, and uart interface.  Implements a very simple command handler for uart data.

- msp430_qpn:  A project that uses QP Nano.

- msp430_task: A tasking project that makes use of a single timer and a while loop to create a simple tasker.  The timer is configured to interrupt every 1ms.  Task states are evaluated in the timer isr.  The task files, task.c/.h are portable and can be reused in other applications.

- msp430_tasksig: Tasking project that uses a timer, a while loop, and two tasks.  The project contains a sender task (TXTask) and a receiver task (RXTask).  The sender tasks posts messages to the receiver task via a TaskMessage array, which is a member of the generic task structure.  The receiver task reads all messages in the array when the task runs (ie, clears all messages).  This approach for posting messsages seems to work pretty good, as long as sender tasks don't jam up the receiver for too long.  So far, there are no checks in the timer isr if any receiver tasks are busy processing messages.

- msp_430_MooreFSM:  A project that uses a simple Moore finite state machine.  The project contains 4 states and an input value that takes a value from 0 to 3.  Toggle the state by pressing the user button.  State pattern: input 0 - state0, input 1 - state 1.. etc.  Each state runs the cooresponding state function.  State_t defintion also contains an entry function pointer to run on transition from one state to the next.

Project Listing (Eclipse)
-------------------------
The purpose of building projects in Eclipse was to try a diffrent approach that what I was doing before.  For these projects, I used the msp430-gcc toolchain and programmed the processor using mspdebug.  A few important settings / switches need to be set up in the Eclipse project, including the following:
- linker flags:  -mmcu=msp430g2553
- preprocessor defines:  __ MSP430G2553__

After building the project, go into debug directory and flash the processor using mspdebug using the following commands (via commandline):
  - mspdebug rf2500
  - prog myprogram.elf
  - run
 
See mspdebug -help for many more options.  Routing the debugger through Eclipse is a bit tricky.  While connected via mspdebug, run gdb (ie, command gdb).  This will search for a connection.  In Eclipse, start the hardware debugger through Debug Configurations, Hardware Debugging, Local Host (Port 2000).  Stepping through the code works... sort of , not as well as using CCS.  I got mixed results, sometimes setting break points, stepping through code worked well, other times not so well.

The following is a list of projects in Eclipse:

- msp430_vfo: Variable frequency oscillator (vfo) project.  The purpose of this project was to create a vfo using the si5351 ic.  The project contains the Nokia5110 LCD, rotary encoder, user buttons and leds.  The hope is that it will someday be a part of a QRP radio project.  The MSP430 interfaces with the si5351 via the i2c interface.  The LCD is controlled through the spi interface.  The project is a work in progress.  The encoder still needs work (it reads the wrong direction sometimes) and only outputs on channel 0.  I added an RIT switch that allows quick change between two frequencies so that tx and rx can be offset from one another.

- msp430_sdcard: Project that uses light-weight FatFs (PetiteFS).  I wanted to get an sdcard working via spi interface and build a simple datalogger.  This is pretty straightforwared using the full FatFs library, but a bit strange using the trimmed down version.  Reads/writes have to end on a sector boundary, which makes for 512 byte reads/writes.  It's a work in progress.

