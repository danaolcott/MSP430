/*
 * SimpleOS.h
 *
 *  Created on: Mar 18, 2019
 *      Author: danao
 *
 *  SimpleOS header file.  Works with Task.h/.c
 *  Manages the OS as tasks.  Requires at least one
 *  timer to run the schduler.
 */

#ifndef SIMPLEOS_SIMPLEOS_H_
#define SIMPLEOS_SIMPLEOS_H_


////////////////////////////////////
//OS Stuff
void SimpleOS_init(void (*functionPtr)(void));
void SimpleOS_addThread(void (*functionPtr)(void));

void SimpleOS_scheduler(void);
void SimpleOS_launch(void);
void SimpleOS_start(void);
void SimpleOS_ISR(void);

void SimpleOS_EnterCritical(void);
void SimpleOS_ExitCritical(void);




#endif /* SIMPLEOS_SIMPLEOS_H_ */
