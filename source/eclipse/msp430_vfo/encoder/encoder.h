#ifndef _ENCODER__H
#define _ENCODER__H
//////////////////////////////////////////
//Dana Olott
//1/14/18
//
//Rotery encoder files

#include <stdint.h>

#define ENCODER_BIT0_PIN		BIT3
#define ENCODER_BIT1_PIN		BIT4
#define ENCODER_BUTTON_PIN		BIT5




typedef enum
{
    ENCODER_DIR_LEFT,
    ENCODER_DIR_RIGHT,
    ENCODER_DIR_NONE,

}EncoderDirection_t;


void encoder_delay(uint16_t delay);
void encoder_init(void);
uint8_t encoder_read(void);
EncoderDirection_t encoder_getDirection(void);



#endif

