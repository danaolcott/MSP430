/*-------------------------------------------------------------------------*/
/* PFF - Low level disk control module for AVR            (C)ChaN, 2010    */
/*-------------------------------------------------------------------------*/

#include "pff.h"
#include "diskio.h"

/*-------------------------------------------------------------------------*/
/* Platform dependent macros and functions needed to be modified           */
/*-------------------------------------------------------------------------*/

#include <msp430.h>
#include <msp430g2553.h>
#include "spi.h"
#include "timer.h"
#include "usart.h"

#define SELECT()	P2OUT &=~ SPI_CS_PIN	/* write - CS = L */
#define	DESELECT()	P2OUT |= SPI_CS_PIN		/* write - CS = H */
#define	MMC_SEL		!(P2IN & SPI_CS_PIN)	/* read  - CS status (true:CS == L) */

//Forwarding read data from sd card out the usart port
#define	FORWARD(d)	usart_writeByte(d)		//send read data out the usart port

//////////////////////////////////////////
//I need to build these functions:
//
void init_spi (void);		/* Initialize SPI port (usi.S) */
void dly_100us (void);		/* Delay 100 microseconds (usi.S) */
void xmit_spi (BYTE d);		/* Send a byte to the MMC (usi.S) */
BYTE rcv_spi (void);		/* Send a 0xFF to the MMC and get the received byte (usi.S) */
//
//
//////////////////////////////////////////////

static FATFS fs;			/* File system object */






/*--------------------------------------------------------------------------

   Module Private Functions

---------------------------------------------------------------------------*/

/* Definitions for MMC/SDC command */
#define CMD0	(0x40+0)	/* GO_IDLE_STATE */
#define CMD1	(0x40+1)	/* SEND_OP_COND (MMC) */
#define	ACMD41	(0xC0+41)	/* SEND_OP_COND (SDC) */
#define CMD8	(0x40+8)	/* SEND_IF_COND */
#define CMD16	(0x40+16)	/* SET_BLOCKLEN */
#define CMD17	(0x40+17)	/* READ_SINGLE_BLOCK */
#define CMD24	(0x40+24)	/* WRITE_BLOCK */
#define CMD55	(0x40+55)	/* APP_CMD */
#define CMD58	(0x40+58)	/* READ_OCR */


/* Card type flags (CardType) */
#define CT_MMC				0x01	/* MMC ver 3 */
#define CT_SD1				0x02	/* SD ver 1 */
#define CT_SD2				0x04	/* SD ver 2 */
#define CT_BLOCK			0x08	/* Block addressing */


static
BYTE CardType;


/*-----------------------------------------------------------------------*/
/* Send a command packet to MMC                                          */
/*-----------------------------------------------------------------------*/

static
BYTE send_cmd (
	BYTE cmd,		/* 1st byte (Start + Index) */
	DWORD arg		/* Argument (32 bits) */
)
{
	BYTE n, res;


	if (cmd & 0x80) {	/* ACMD<n> is the command sequense of CMD55-CMD<n> */
		cmd &= 0x7F;
		res = send_cmd(CMD55, 0);
		if (res > 1) return res;
	}

	/* Select the card */
	DESELECT();
	rcv_spi();
	SELECT();
	rcv_spi();

	/* Send a command packet */
	xmit_spi(cmd);						/* Start + Command index */
	xmit_spi((BYTE)(arg >> 24));		/* Argument[31..24] */
	xmit_spi((BYTE)(arg >> 16));		/* Argument[23..16] */
	xmit_spi((BYTE)(arg >> 8));			/* Argument[15..8] */
	xmit_spi((BYTE)arg);				/* Argument[7..0] */
	n = 0x01;							/* Dummy CRC + Stop */
	if (cmd == CMD0) n = 0x95;			/* Valid CRC for CMD0(0) */
	if (cmd == CMD8) n = 0x87;			/* Valid CRC for CMD8(0x1AA) */
	xmit_spi(n);

	/* Receive a command response */
	n = 10;								/* Wait for a valid response in timeout of 10 attempts */
	do {
		res = rcv_spi();
	} while ((res & 0x80) && --n);

	return res;			/* Return with the response value */
}




/*--------------------------------------------------------------------------

   Public Functions

---------------------------------------------------------------------------*/

/*-----------------------------------------------------------------------*/
/* Initialize Disk Drive                                                 */
/*-----------------------------------------------------------------------*/

DSTATUS disk_initialize (void)
{
	BYTE n, cmd, ty, ocr[4];
	UINT tmr;

#if _USE_WRITE
	if (CardType && MMC_SEL) disk_writep(0, 0);	/* Finalize write process if it is in progress */
#endif
	init_spi();		/* Initialize ports to control MMC */
	DESELECT();
	for (n = 10; n; n--) rcv_spi();	/* 80 dummy clocks with CS=H */

	ty = 0;
	if (send_cmd(CMD0, 0) == 1) {			/* Enter Idle state */
		if (send_cmd(CMD8, 0x1AA) == 1) {	/* SDv2 */
			for (n = 0; n < 4; n++) ocr[n] = rcv_spi();		/* Get trailing return value of R7 resp */
			if (ocr[2] == 0x01 && ocr[3] == 0xAA) {			/* The card can work at vdd range of 2.7-3.6V */
				for (tmr = 10000; tmr && send_cmd(ACMD41, 1UL << 30); tmr--) dly_100us();	/* Wait for leaving idle state (ACMD41 with HCS bit) */
				if (tmr && send_cmd(CMD58, 0) == 0) {		/* Check CCS bit in the OCR */
					for (n = 0; n < 4; n++) ocr[n] = rcv_spi();
					ty = (ocr[0] & 0x40) ? CT_SD2 | CT_BLOCK : CT_SD2;	/* SDv2 (HC or SC) */
				}
			}
		} else {							/* SDv1 or MMCv3 */
			if (send_cmd(ACMD41, 0) <= 1) 	{
				ty = CT_SD1; cmd = ACMD41;	/* SDv1 */
			} else {
				ty = CT_MMC; cmd = CMD1;	/* MMCv3 */
			}
			for (tmr = 10000; tmr && send_cmd(cmd, 0); tmr--) dly_100us();	/* Wait for leaving idle state */
			if (!tmr || send_cmd(CMD16, 512) != 0)			/* Set R/W block length to 512 */
				ty = 0;
		}
	}
	CardType = ty;
	DESELECT();
	rcv_spi();

	return ty ? 0 : STA_NOINIT;
}



/*-----------------------------------------------------------------------*/
/* Read partial sector                                                   */
/*-----------------------------------------------------------------------*/

DRESULT disk_readp (
	BYTE *buff,		/* Pointer to the read buffer (NULL:Read bytes are forwarded to the stream) */
	DWORD lba,		/* Sector number (LBA) */
	WORD ofs,		/* Byte offset to read from (0..511) */
	WORD cnt		/* Number of bytes to read (ofs + cnt mus be <= 512) */
)
{
	DRESULT res;
	BYTE rc;
	WORD bc;


	if (!(CardType & CT_BLOCK)) lba *= 512;		/* Convert to byte address if needed */

	res = RES_ERROR;
	if (send_cmd(CMD17, lba) == 0) {		/* READ_SINGLE_BLOCK */

		bc = 40000;
		do {							/* Wait for data packet */
			rc = rcv_spi();
		} while (rc == 0xFF && --bc);

		if (rc == 0xFE) {				/* A data packet arrived */
			bc = 514 - ofs - cnt;

			/* Skip leading bytes */
			if (ofs) {
				do rcv_spi(); while (--ofs);
			}

			/* Receive a part of the sector */
			if (buff) {	/* Store data to the memory */
				do {
					*buff++ = rcv_spi();
				} while (--cnt);
			} else {	/* Forward data to the outgoing stream (depends on the project) */
				do {
					FORWARD(rcv_spi());
				} while (--cnt);
			}

			/* Skip trailing bytes and CRC */
			do rcv_spi(); while (--bc);

			res = RES_OK;
		}
	}

	DESELECT();
	rcv_spi();

	return res;
}



/*-----------------------------------------------------------------------*/
/* Write partial sector                                                  */
/*-----------------------------------------------------------------------*/

#if _USE_WRITE
DRESULT disk_writep (
	const BYTE *buff,	/* Pointer to the bytes to be written (NULL:Initiate/Finalize sector write) */
	DWORD sa			/* Number of bytes to send, Sector number (LBA) or zero */
)
{
	DRESULT res;
	WORD bc;
	static WORD wc;

	res = RES_ERROR;

	if (buff) {		/* Send data bytes */
		bc = (WORD)sa;
		while (bc && wc) {		/* Send data bytes to the card */
			xmit_spi(*buff++);
			wc--; bc--;
		}
		res = RES_OK;
	} else {
		if (sa) {	/* Initiate sector write process */
			if (!(CardType & CT_BLOCK)) sa *= 512;	/* Convert to byte address if needed */
			if (send_cmd(CMD24, sa) == 0) {			/* WRITE_SINGLE_BLOCK */
				xmit_spi(0xFF); xmit_spi(0xFE);		/* Data block header */
				wc = 512;							/* Set byte counter */
				res = RES_OK;
			}
		} else {	/* Finalize sector write process */
			bc = wc + 2;
			while (bc--) xmit_spi(0);	/* Fill left bytes and CRC with zeros */
			if ((rcv_spi() & 0x1F) == 0x05) {	/* Receive data resp and wait for end of write process in timeout of 500ms */
				for (bc = 5000; rcv_spi() != 0xFF && bc; bc--) dly_100us();	/* Wait ready */
				if (bc) res = RES_OK;
			}
			DESELECT();
			rcv_spi();
		}
	}

	return res;
}
#endif







///////////////////////////////////////////////////////
//init_spi
//do nothing...  already enabled
void init_spi (void)
{

}


//////////////////////////////////////
//delay - use timer delay
//use a dummy delay, not the timer
//might be messing things up
void dly_100us (void)
{
	volatile unsigned int temp = 10000;
	while (temp > 0)
		temp--;
}


/////////////////////////////////////
//spi_tx
void xmit_spi (BYTE d)
{
	spi_tx(d);
}

////////////////////////////////////
//
BYTE rcv_spi (void)
{
	BYTE value = spi_rx();
	return value;
}




///////////////////////////////////////
//mmc_init
//puts card into idle state, mounts
//drive, etc.
int mmc_init(void)
{
	FRESULT res;

	unsigned char result = mmc_GoIdleState();

	if (result == 1)
	{
		res = pf_mount(&fs);

		if (res == FR_OK)
		{
			spi_init(SPI_SPEED_4MHZ);
		}

		else
		{
			return -1;		//unable to mount drive
		}
	}

	else
	{
		return -2;			//unable to go idle state
	}

	return 1;
}

///////////////////////////////////////
//go idle - put sd card into idle state
//
unsigned char mmc_GoIdleState(void)
{
    //set the spi speed to 400khz
	spi_init(SPI_SPEED_400KHZ);

	Timer_delay_ms(100);

    unsigned char i, response, retry=0;

    spi_select();

    do
    {
        //send 80 clock cycles
        for(i=0 ; i < 10 ; i++)
        {
        	spi_tx(0xFF);
        }

        //send CMD0 - Go Idle state - continue
        //until the response is valid = 0x01

        response = send_cmd(CMD0, 0);

        retry++;

        if( retry > 0xfe)
        {
            //error, could not init the card,
            return 0;

        }//time out

    } while(response != 0x01);

    spi_deselect();

    return 1; //normal return
}



////////////////////////////////////////////////
//Write data to file
//process:
//open the file
//reset the file pointer
//write the data
//write null
unsigned int mmc_writeFile(char* name, char* buffer, unsigned int size)
{
	unsigned int bytesWritten = 0x00;
	unsigned int num = 0;

	Timer_stop();

	pf_open(name);					//open the file

	pf_lseek(0x00);						//reset the file pointer

	pf_write(buffer, size, &num);	//write data
	bytesWritten += num;

	pf_write(0, 0, &num);			//terminate

	Timer_start();

	return bytesWritten;
}

///////////////////////////////////////////////
//read data from a file into a buffer
//pass the file pointer
unsigned int mmc_readFile(char* name, char* buffer, unsigned int maxBytes)
{
	unsigned int bytesRead = 0x00;
	unsigned int num = 0;
	FRESULT res;

	Timer_stop();

	res = pf_open(name);						//open the file

	if (res == FR_OK)
	{
		res = pf_lseek(0);						//reset the file pointer

		//try reading max bytes, load num ptr
		//with bytes read
		res = pf_read(buffer, maxBytes, &num);	//write data
		bytesRead += num;						//bytes read
		res = pf_read(0, 0, &num);				//terminate
	}

	Timer_start();

	return bytesRead;
}



/////////////////////////////////////////////////////////
//writes a line to a file.
//lines are defined as 512 bytes each
//writes data and completes the line with 0x00.
//says it can't expand it but I'm thinking it did
//as long as it's not end of file?
//looks like notepad can only store 256 per line
unsigned int mmc_writeLine(char* name, unsigned int line, char* buffer, unsigned int size)
{
	unsigned int bytesWritten = 0x00;
	unsigned int num = 0;

	unsigned int offset = line * 512;

	Timer_stop();

	pf_open(name);					//open the file
	pf_lseek(offset);				//jump file ptr to line
	pf_write(buffer, size, &num);	//write data
	bytesWritten += num;
	pf_write(0, 0, &num);			//terminate to complete the line

	Timer_start();

	return bytesWritten;
}



/////////////////////////////////////////////////////
//appends data after the first instance of appendChar
//then writes 0x00 to complete a sector
unsigned int mmc_append(char* name, char appendChar, char* buffer, unsigned int size)
{
	unsigned int bytesWritten = 0x00;
	unsigned int num = 0;
	unsigned int offset = 0x00;
	unsigned int jump = 0x00;

	Timer_stop();

	char c;

	pf_open(name);					//open the file
	pf_lseek(0);					//reset
	offset = 0;
	pf_read(&c, 1, &num);
	while (c != appendChar)
	{
		offset++;
		pf_read(&c, 1, &num);
	}
	//read the rest
	pf_read(0, 0, &num);				//terminate

	//roundup to next sector
	jump = ((offset / 512) + 1) * 512;
	//jump to offset from start
	pf_lseek(jump);

	//append data
	pf_write(buffer, size, &num);	//write data
	bytesWritten += num;
	pf_write(0, 0, &num);			//terminate to complete the line

	Timer_start();

	return bytesWritten;
}
