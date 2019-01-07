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



void SPI_init(SPISpeed_t speed);



void SPI_select(void);
void SPI_deselect(void);

uint8_t SPI_tx(uint8_t data);
uint8_t SPI_rx(void);


void SPI_write(uint8_t);
void SPI_writeArray(uint8_t* buffer, uint8_t length);
uint8_t SPI_read(void);




#endif

