#ifndef _SPI__H
#define _SPI__H


/*
SPI Device Driver File for SPIB

P1.5, P1.6, P1.7
CS = P2.4 - configure as regular io

Note: SPI prescale values assume a 16mhz clock


*/


#include <stdint.h>

//pin defines - Chip select P2.4
#define SPI_CS_PIN		BIT4

//commands - dummy commands
#define SPI_CMD_WRITE	0xAA
#define SPI_CMD_READ	0xBB



////////////////////////////////
//
typedef enum
{
	SPI_SPEED_400KHZ,
	SPI_SPEED_1MHZ,
	SPI_SPEED_2MHZ,
	SPI_SPEED_4MHZ,

}SPISpeed_t;



void spi_init(SPISpeed_t speed);



void spi_select(void);
void spi_deselect(void);

uint8_t spi_tx(uint8_t data);
uint8_t spi_rx(void);


void spi_write(uint8_t);
uint8_t spi_read(void);




#endif

