#ifndef __TIMER_H
#define __TIMER_H

/*
Timer Header File
Contains timer init and timer ISR

*/

#include <stdint.h>



void Timer_init(void);
void Timer_DelayDecrement(void);
void Timer_delay_ms(uint16_t delay);

void Timer_start(void);
void Timer_stop(void);

//////////////////////////////////////
//For using countdown timers
void Timer_Counter1Set(uint16_t count);
uint16_t Timer_Counter1Get(void);
void Timer_Counter1Decrement(void);




#endif

