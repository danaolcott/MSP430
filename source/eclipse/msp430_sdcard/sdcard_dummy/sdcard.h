////////////////////////////////////////////
//SD Card Driver File
//SPI Interface using SPI3, CS pin
//PD1 defined as normal GPIO.
//
//
#ifndef __SDCARD__H
#define __SDCARD__H

#include <stdint.h>
#include <stdlib.h>

#include "pff.h"         //FRESULT typdedefs
//////////////////////////////////////////


////////////////////////////////////////
//Function prototypes
//

int SD_Init(void);
int SD_BuildDirectory(char* name);
void SD_ErrorHandler(FRESULT result);       
char* SD_GetStringFromFatCode(FRESULT result);


//lowest level spi functions
void SD_CS_Assert(void);
void SD_CS_Deassart(void);
void SPI_transmit(uint8_t data);
uint8_t SPI_receive(void);
unsigned char SD_GoIdleState(void);
unsigned char SD_sendCommand(unsigned char cmd, unsigned long arg);

//functions and commands from fat fs example
void CS_HIGH(void);		/* Set CS# high */
void CS_LOW(void);		/* Set CS# low */
void deselect(void);
int select(void);

int wait_ready(uint32_t wt);
void disk_timerproc (void);

void init_spi(void);
void FCLK_SLOW(void);
void FCLK_FAST(void);
uint8_t xchg_spi(uint8_t data);
uint8_t send_cmd(BYTE cmd, DWORD arg);
void rcvr_spi_multi (BYTE *buff, UINT btr);
#if _USE_WRITE
void xmit_spi_multi (const BYTE *buff, UINT btx);
#endif
int rcvr_datablock (BYTE *buff,	UINT btr);
#if _USE_WRITE
int xmit_datablock (const BYTE *buff, BYTE token);
#endif

char *dec32(unsigned long i);






#endif

