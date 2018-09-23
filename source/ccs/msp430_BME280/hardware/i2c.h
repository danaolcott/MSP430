#ifndef __I2C_H
#define __I2C_H


#include <stdint.h>
#include <stddef.h>


void i2c_init(uint8_t address7bit);
void i2c_write(uint8_t address7bit, uint8_t* data, uint8_t len);
void i2c_read(uint8_t address7bit, uint8_t* data, uint8_t len);

void i2c_writeAddress(uint8_t address7bit, uint8_t address, uint8_t* data, uint8_t len);
void i2c_readAddress(uint8_t address7bit, uint8_t address, uint8_t* data, uint8_t len);



#endif
