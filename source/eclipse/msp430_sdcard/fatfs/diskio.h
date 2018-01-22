/*-----------------------------------------------------------------------
/  PFF - Low level disk interface modlue include file    (C)ChaN, 2014
/-----------------------------------------------------------------------*/

#ifndef _DISKIO_DEFINED
#define _DISKIO_DEFINED

#ifdef __cplusplus
extern "C" {
#endif

#include "integer.h"


/* Status of Disk Functions */
typedef BYTE	DSTATUS;


/* Results of Disk Functions */
typedef enum {
	RES_OK = 0,		/* 0: Function succeeded */
	RES_ERROR,		/* 1: Disk error */
	RES_NOTRDY,		/* 2: Not ready */
	RES_PARERR		/* 3: Invalid parameter */
} DRESULT;


/*---------------------------------------*/
/* Prototypes for disk control functions */

DSTATUS disk_initialize (void);

//old prototype
//DRESULT disk_readp (BYTE* buff, DWORD sector, UINT offset, UINT count);


DRESULT disk_readp (BYTE *buff,	DWORD lba, WORD ofs, WORD cnt);



DRESULT disk_writep (const BYTE* buff, DWORD sc);

#define STA_NOINIT		0x01	/* Drive not initialized */
#define STA_NODISK		0x02	/* No medium in the drive */

/* Card type flags (CardType) */
#define CT_MMC				0x01	/* MMC ver 3 */
#define CT_SD1				0x02	/* SD ver 1 */
#define CT_SD2				0x04	/* SD ver 2 */
#define CT_SDC				(CT_SD1|CT_SD2)	/* SD */
#define CT_BLOCK			0x08	/* Block addressing */


/////////////////////////////////////
//extra functions - defined in mmc.c
unsigned char mmc_GoIdleState(void);
unsigned int mmc_writeFile(FATFS *fs, char* name, char* buffer, unsigned int size);
unsigned int mmc_readFile(FATFS *fs, char* name, char* buffer, unsigned int maxBytes);


#ifdef __cplusplus
}
#endif

#endif	/* _DISKIO_DEFINED */
