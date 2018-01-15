#ifndef _SPI__H
#define _SPI__H


/*
SPI Device Driver File for SPIA


*/


#include <stdint.h>





//pin defines - Chip select P1.5
#define SPI_CS_PIN		BIT5



void spi_init(void);

void spi_select(void);
void spi_deselect(void);
void spi_write(uint8_t);
void spi_writeData(uint16_t address, uint8_t data);
uint8_t spi_readData(uint16_t address);




#endif

