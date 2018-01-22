////////////////////////////////////////////////////////
//SPI Driver for SPI3 on PD0 - PD3
//PD1 is the cs pin for the lcd.  PA5 is the
//CS pin for the sd card.  CS pin for PA5 is configured
//
//
#ifndef SPI_DRIVER__H
#define SPI_DRIVER__H


#include <stdio.h>
#include <stdint.h>

#include "main.h"


void SPI_init(void);
void SPI_WriteData(uint8_t data);
void SPI_WriteReg(uint8_t reg, uint8_t data);
void SPI_WriteArray(uint8_t* data, uint32_t length);

uint8_t SPIReadData(uint8_t reg);

void SPI_SetSpeedHz(uint32_t speed);

#endif

