#ifndef _I2C_H
#define _I2C_H

/////////////////////////////////////////
/*
I2C Driver File Function Definitions
1/13/18
Dana Olcott

Low level register settings from Texas
Instruments files with some minor changes.
The following are instended to work with 
the si5351 i2c communcation interface.

*/
/////////////////////////////////////////


#include <stdint.h>

/////////////////////////////////////////////////////
#define I2C_ADDRESS				(uint8_t)0x60

//i2c bus prescale for a 16mhz clock
#define I2C_BUS_PRESCALE		((uint8_t)(0x28))	//400khz
//#define I2C_BUS_PRESCALE		((uint8_t)(0xA0))	//100khz


void i2c_init(void);
void i2c_write(uint8_t* data, uint8_t len);
void i2c_read(uint8_t* data, uint8_t len);


#endif

