#ifndef SI5351_H
#define SI5351_H

#include <stdint.h>

#define VFO_I2C_ADDRESS			((uint8_t)(0x60 << 1))

#define VFO_REV_ID_REG			(0x00)		//bits 01

#define VFO_STATUS_REG			(0x00)
#define VFO_STATUS_BIT			(1 << 7)
#define VFO_PLLA_BIT			(1 << 5)
#define VFO_PLLB_BIT			(1 << 6)


#define VFO_CH_ENABLE_REG		(3)
#define VFO_CH0_ENABLE_BIT		(1 << 0)
#define VFO_CH1_ENABLE_BIT		(1 << 1)
#define VFO_CH2_ENABLE_BIT		(1 << 2)

#define VFO_CH0_CONTROL_REG		(16)
#define VFO_CH1_CONTROL_REG		(17)
#define VFO_CH2_CONTROL_REG		(18)
#define VFO_PLL_RESET_REG		(177)




#define SI_CLK0_CONTROL	16			// Register definitions
#define SI_CLK1_CONTROL	17
#define SI_CLK2_CONTROL	18
#define SI_SYNTH_PLL_A	26
#define SI_SYNTH_PLL_B	34
#define SI_SYNTH_MS_0		42
#define SI_SYNTH_MS_1		50
#define SI_SYNTH_MS_2		58
#define SI_PLL_RESET		177

#define SI_R_DIV_1		0b00000000			// R-division ratio definitions
#define SI_R_DIV_2		0b00010000
#define SI_R_DIV_4		0b00100000
#define SI_R_DIV_8		0b00110000
#define SI_R_DIV_16		0b01000000
#define SI_R_DIV_32		0b01010000
#define SI_R_DIV_64		0b01100000
#define SI_R_DIV_128		0b01110000

#define SI_CLK_SRC_PLL_A	0b00000000
#define SI_CLK_SRC_PLL_B	0b00100000

#define XTAL_FREQ	27000000			// Crystal frequency

//freq. offset adjustments - press the encoder button and adjust
#define VFO_FREQ_OFFSET_CENTER		1000	//center freq
#define VFO_FREQ_OFFSET_INC			10
#define VFO_MIN_FREQ_OFFSET			0
#define VFO_MAX_FREQ_OFFSET			2000
#define VFO_DEFAULT_FREQ_OFFSET		1600


typedef enum
{
	VFO_CHANNEL_0,
	VFO_CHANNEL_1,
	VFO_CHANNEL_2,
}VFOChannel_t;

typedef enum
{
	VFO_STATE_DISABLE,
	VFO_STATE_ENABLE,
}VFOState_t;

typedef enum
{
	VFO_DRIVE_STRENGTH_2MA,
	VFO_DRIVE_STRENGTH_4MA,
	VFO_DRIVE_STRENGTH_6MA,
	VFO_DRIVE_STRENGTH_8MA,
}VFODriveStrength_t;



void vfo_init(void);
void vfo_SetChannel0Frequency(uint32_t frequency);
uint32_t vfo_GetChannel0Frequency(void);

void vfo_IncreaseChannel0Frequency(void);
void vfo_DecreaseChannel0Frequency(void);

uint16_t vfo_GetFreqOffset(void);
void vfo_IncreaseFreqOffset(void);
void vfo_DecreaseFreqOffset(void);


void vfo_SetChannelDrive(VFODriveStrength_t drive);
void vfo_SetChannelState(VFOChannel_t ch, VFOState_t state);

uint16_t vfo_GetVFOIncrement(void);
uint16_t vfo_IncreaseVFOIncrement(void);
uint16_t vfo_DecreaseVFOIncrement(void);

uint8_t vfo_GetInitStatus(void);
uint8_t vfo_GetPLLAStatus(void);
uint8_t vfo_GetPLLBStatus(void);
void vfo_DisplayLockState(void);



#endif //SI5351_H

