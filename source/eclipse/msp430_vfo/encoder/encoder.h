#ifndef _ENCODER__H
#define _ENCODER__H
//////////////////////////////////////////
//Dana Olott
//1/14/18
//
//Rotery encoder files

#include <stdint.h>

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

