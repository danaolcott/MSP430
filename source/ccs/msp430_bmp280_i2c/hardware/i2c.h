#ifndef __I2C_H
#define __I2C_H


#include <stdint.h>
#include <stddef.h>

//BMP280 address 0x76 - SDO connected to ground.?
#define I2C_ADDRESS             (uint8_t)0x76
#define I2C_BUS_PRESCALE        ((uint8_t)(0x28))   //400khz

void i2c_init(uint8_t address7bit);
void i2c_write(uint8_t address7bit, uint8_t* data, uint8_t len);
void i2c_read(uint8_t address7bit, uint8_t* data, uint8_t len);

void i2c_writeAddress(uint8_t address7bit, uint8_t address, uint8_t* data, uint8_t len);
void i2c_readAddress(uint8_t address7bit, uint8_t address, uint8_t* data, uint8_t len);



#endif
