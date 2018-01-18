/////////////////////////////////////////////
/////////////////////////////////////////////
//Si5351 VFO Control IC Control Functions.
//Dana Olcott
//1/13/18
//
//Function definitions for setting the output
//frequency, channel enable, disable, checking
//the pll state, etc.  Also shown in list of 
//output frequencies comparted to register
//set values.  Output frequency is not exact,
//it was read by a yeasu757 transceiver that I 
//know the frequency is off a bit.  Adjustment
//is made in the frequency set function
//
//Note:
//
//PLL and Multisynth functions are not mine,
//I didn't write them.  These appear to be
//from the following source: 
// Author: Hans Summers, 2015
// Website: http://www.hanssummers.com
// Also found on QRP Labs Website and a few
// other websites.
//
//
//////////////////////////////
/*
target			actual

y = 1.076x + 29010
f = initial input
1 * f   +  76/1000 * f + 29010

//////////////////////////
input			actual
7710000			7137400
7700000			7127100
7690000			7117900
7680000			7188000
7670000			7099600
7660000			7090000
7650000			7080800
7640000			7071800
7630000			7062500
7620000			7053000
7610000			7043800
7600000			7034300
7590000			7025300
7580000			7016000
7570000			7006800
///////////////////////////

*/

#include <stdint.h>

#include "si5351.h"
#include "i2c.h"

///////////////////////////////////////////////////
static uint32_t gClockFrequency = 0x00;
static uint16_t gVFOIncrement = 1;

/////////////////////////////////////////////////
//helper functions
static void vfo_writeReg(uint8_t reg, uint8_t data);
static void vfo_readReg(uint8_t reg, uint8_t *data);
static void vfo_resetPLL(void);
static void SetupPLL(uint8_t pll, uint8_t mult, uint32_t num, uint32_t denom);
static void SetupMultisynth(uint8_t synth, uint32_t divider, uint8_t rDiv);



/////////////////////////////////////////
//initialize the si5351 according to the 
//flowchart listed in the datasheet.
//
void vfo_init(void)
{
	gClockFrequency = 7000000;
	gVFOIncrement = 1;



	//set CLKx_DIS high, reg 0x03 = 0xFF
	vfo_writeReg(3,  0xFF);

	//reg 9 - OEB pin controls enable/disable
	vfo_writeReg(3,  0x00);

	//power down all output drivers
	vfo_writeReg(16, 0x80);
	vfo_writeReg(17, 0x80);
	vfo_writeReg(18, 0x80);
	vfo_writeReg(19, 0x80);
	vfo_writeReg(20, 0x80);
	vfo_writeReg(21, 0x80);
	vfo_writeReg(22, 0x80);
	vfo_writeReg(23, 0x80);

	//set interrupt masks - see register 2
	vfo_writeReg(2, 0xF0);

	//reg 15- pll input source - xtal input as ref clock
	vfo_writeReg(15, 0x00);	//default

	/////////////////////////////////////////
	//use integer inputs:
	//reg 16 - clk 0 - 01001100
	//pwr up, integer mode, plla, non-inverted,
	//si5351 as source, 8ma drive
	vfo_writeReg(16, 0x4F);		//clk0
	vfo_writeReg(17, 0x4F);		//clk1
	vfo_writeReg(18, 0x4F);		//clk2
	///////////////////////////////////////////////////

	//reg 24 - clk states 0-3 - low when disabled
	vfo_writeReg(24, 0x00);

	//reset the pll
	vfo_resetPLL();

	//enable outputs - register 3
	vfo_SetChannelState(VFO_CHANNEL_0, VFO_STATE_ENABLE);
	vfo_SetChannelState(VFO_CHANNEL_1, VFO_STATE_ENABLE);
	vfo_SetChannelState(VFO_CHANNEL_2, VFO_STATE_ENABLE);
	vfo_SetChannelDrive(VFO_DRIVE_STRENGTH_8MA);

	//vfo_SetChannel0Frequency(gClockFrequency);

}


/////////////////////////////////////////////////////////////
// Set CLK0 output ON and to the specified frequency
// Frequency is in the range 1MHz to 150MHz
// Example: si5351aSetFrequency(10000000);
// will set output CLK0 to 10MHz
// Function Source (ie, I didn't write it):
// Author: Hans Summers, 2015
// Website: http://www.hanssummers.com, also found
// on QRP labs and a handful of other sites.
//
// This example sets up PLL A
// and MultiSynth 0
// and produces the output on CLK0
//
void vfo_SetChannel0Frequency(uint32_t frequency)
{
	uint32_t pllFreq;
	uint32_t xtalFreq = XTAL_FREQ;
	uint32_t l;
	float f;
	uint8_t mult;
	uint32_t num;
	uint32_t denom;
	uint32_t divider;

	gClockFrequency = frequency;

	///////////////////////////////////////////////
	//Scale the input frequency based on radio
	//receiver readings.  use the following
	//and add a bit extra
	//	1 * f   +  76/1000 * f + 29010

	frequency = frequency + (76 * frequency / 1000) + 2000 + 29010;
//	uint32_t extra = 2000;
//	uint32_t fUpdated = frequency + (76 * frequency / 1000) + extra + 29010;

//	frequency = fUpdated;

	divider = 900000000 / frequency;// Calculate the division ratio. 900,000,000 is the maximum internal
									// PLL frequency: 900MHz
	if (divider % 2) divider--;		// Ensure an even integer division ratio

	pllFreq = divider * frequency;	// Calculate the pllFrequency: the divider * desired output frequency

	mult = pllFreq / xtalFreq;		// Determine the multiplier to get to the required pllFrequency
	l = pllFreq % xtalFreq;			// It has three parts:
	f = l;							// mult is an integer that must be in the range 15..90
	f *= 1048575;					// num and denom are the fractional parts, the numerator and denominator
	f /= xtalFreq;					// each is 20 bits (range 0..1048575)
	num = f;						// the actual multiplier is  mult + num / denom
	denom = 1048575;				// For simplicity we set the denominator to the maximum 1048575

	// Set up PLL A with the calculated multiplication ratio
	SetupPLL(SI_SYNTH_PLL_A, mult, num, denom);

	// Set up MultiSynth divider 0, with the calculated divider.
	// The final R division stage can divide by a power of two, from 1..128.
	// reprented by constants SI_R_DIV1 to SI_R_DIV128 (see si5351a.h header file)
	// If you want to output frequencies below 1MHz, you have to use the
	// final R division stage
	SetupMultisynth(SI_SYNTH_MS_0, divider, SI_R_DIV_1);

	// Reset the PLL. This causes a glitch in the output. For small changes to
	// the parameters, you don't need to reset the PLL, and there is no glitch
	vfo_resetPLL();

}








/*
///////////////////////////////////////////////////
frequency function - tested and works on the
stm32f411 processor. Calling this function on
the msp430 seems to crash it.  stack size?

void vfo_SetChannel0Frequency(uint32_t frequency)
{
	uint32_t pllFreq;
	uint32_t xtalFreq = XTAL_FREQ;
	uint32_t l;
	float f;
	uint8_t mult;
	uint32_t num;
	uint32_t denom;
	uint32_t divider;

	gClockFrequency = frequency;

	///////////////////////////////////////////////
	//Scale the input frequency based on radio
	//receiver readings.  use the following
	//and add a bit extra
	//	1 * f   +  76/1000 * f + 29010
	uint32_t extra = 2000;
	uint32_t fUpdated = frequency + (76 * frequency / 1000) + extra + 29010;
	frequency = fUpdated;

	divider = 900000000 / frequency;// Calculate the division ratio. 900,000,000 is the maximum internal
									// PLL frequency: 900MHz
	if (divider % 2) divider--;		// Ensure an even integer division ratio

	pllFreq = divider * frequency;	// Calculate the pllFrequency: the divider * desired output frequency

	mult = pllFreq / xtalFreq;		// Determine the multiplier to get to the required pllFrequency
	l = pllFreq % xtalFreq;			// It has three parts:
	f = l;							// mult is an integer that must be in the range 15..90
	f *= 1048575;					// num and denom are the fractional parts, the numerator and denominator
	f /= xtalFreq;					// each is 20 bits (range 0..1048575)
	num = f;						// the actual multiplier is  mult + num / denom
	denom = 1048575;				// For simplicity we set the denominator to the maximum 1048575

	// Set up PLL A with the calculated multiplication ratio
	SetupPLL(SI_SYNTH_PLL_A, mult, num, denom);

	// Set up MultiSynth divider 0, with the calculated divider.
	// The final R division stage can divide by a power of two, from 1..128.
	// reprented by constants SI_R_DIV1 to SI_R_DIV128 (see si5351a.h header file)
	// If you want to output frequencies below 1MHz, you have to use the
	// final R division stage
	SetupMultisynth(SI_SYNTH_MS_0, divider, SI_R_DIV_1);

	// Reset the PLL. This causes a glitch in the output. For small changes to
	// the parameters, you don't need to reset the PLL, and there is no glitch
	vfo_resetPLL();

}


*/




///////////////////////////////////////////
//returns the current frequency stored.
uint32_t vfo_GetChannel0Frequency(void)
{
	return gClockFrequency;
}


/////////////////////////////////////////
//increment value changed in increment
//increase/ decrease
void vfo_IncreaseChannel0Frequency(void)
{
	gClockFrequency += gVFOIncrement;
	vfo_SetChannel0Frequency(gClockFrequency);
}

void vfo_DecreaseChannel0Frequency(void)
{
	gClockFrequency -= gVFOIncrement;
	vfo_SetChannel0Frequency(gClockFrequency);
}



/////////////////////////////////////////////////
//Set the drive strength for all channels
//Registers 16, 17, 18 - bits 0 and 1
//Drive strength can be the following:
//VFO_DRIVE_STRENGTH_2MA
//VFO_DRIVE_STRENGTH_4MA
//VFO_DRIVE_STRENGTH_6MA
//VFO_DRIVE_STRENGTH_8MA
//
void vfo_SetChannelDrive(VFODriveStrength_t drive)
{
	uint8_t bits = 0x00;
	uint8_t ch0 = 0x00;
	uint8_t ch1 = 0x00;
	uint8_t ch2 = 0x00;

	vfo_readReg(VFO_CH0_CONTROL_REG, &ch0);
	vfo_readReg(VFO_CH1_CONTROL_REG, &ch1);
	vfo_readReg(VFO_CH2_CONTROL_REG, &ch2);

	//clear bits 01
	ch0 &=~ 0x03;
	ch1 &=~ 0x03;
	ch2 &=~ 0x03;

	switch(drive)
	{
		case VFO_DRIVE_STRENGTH_2MA:	bits = 0x00;	break;
		case VFO_DRIVE_STRENGTH_4MA:	bits = 0x01;	break;
		case VFO_DRIVE_STRENGTH_6MA:	bits = 0x10;	break;
		case VFO_DRIVE_STRENGTH_8MA:	bits = 0x11;	break;
		default:	break;
	}

	ch0 |= bits;
	ch1 |= bits;
	ch2 |= bits;

	vfo_writeReg(VFO_CH0_CONTROL_REG, ch0);
	vfo_writeReg(VFO_CH1_CONTROL_REG, ch1);
	vfo_writeReg(VFO_CH2_CONTROL_REG, ch2);
}





//////////////////////////////////////////////////////////
//Enable / Disable VFO Channel 0-2
//Args: VFO Channel and state
//VFO Channel: VFO_CHANNEL_0, VFO_CHANNEL_1, VFO_CHANNEL_2
//State: VFO_STATE_ENABLE, VFO_STATE_DISABLE
//
//From datasheet, clear bit to enable
//
void vfo_SetChannelState(VFOChannel_t ch, VFOState_t state)
{
	uint8_t current = 0x00;
	vfo_readReg(VFO_CH_ENABLE_REG, &current);

	//clear the bit, enabling the channel
	switch(ch)
	{
		case VFO_CHANNEL_0:	current &=~ VFO_CH0_ENABLE_BIT;	break;
		case VFO_CHANNEL_1:	current &=~ VFO_CH1_ENABLE_BIT;	break;
		case VFO_CHANNEL_2:	current &=~ VFO_CH2_ENABLE_BIT;	break;
		default:	break;
	}

	//do something only if we  are disabling a channel (ie, |=)
	if (state == VFO_STATE_DISABLE)
	{
		switch(ch)
		{
			case VFO_CHANNEL_0:	current |= VFO_CH0_ENABLE_BIT;	break;
			case VFO_CHANNEL_1:	current |= VFO_CH1_ENABLE_BIT;	break;
			case VFO_CHANNEL_2:	current |= VFO_CH2_ENABLE_BIT;	break;
			default:	break;
		}
	}

	//write modified value
	vfo_writeReg(VFO_CH_ENABLE_REG, current);
}


/////////////////////////////////////
//returns the frequency tuning increment
uint16_t vfo_GetVFOIncrement(void)
{
	return gVFOIncrement;
}


///////////////////////////////////////
//increase frequency tuning increment
//
uint16_t vfo_IncreaseVFOIncrement(void)
{
	//set the value
	switch(gVFOIncrement)
	{
		case 1:			gVFOIncrement = 10;		break;
		case 10:		gVFOIncrement = 100;	break;
		case 100:		gVFOIncrement = 1000;	break;
		case 1000:		gVFOIncrement = 10000;	break;
		case 10000:		gVFOIncrement = 1;		break;
		default:		gVFOIncrement = 1;		break;
	}

	return gVFOIncrement;
}


/////////////////////////////////////
//decrease frequency tuning increment
//
uint16_t vfo_DecreaseVFOIncrement(void)
{
	//set the value
	switch(gVFOIncrement)
	{
		case 1:			gVFOIncrement = 10000;	break;
		case 10:		gVFOIncrement = 1;		break;
		case 100:		gVFOIncrement = 10;		break;
		case 1000:		gVFOIncrement = 100;	break;
		case 10000:		gVFOIncrement = 1000;	break;
		default:		gVFOIncrement = 1;		break;
	}

	return gVFOIncrement;
}


/////////////////////////////////
//poll this function in the beginning
//before calling init
//returns system initialization
//status - 0 = ready, 1 = init
//in progress.  reg 0 , bit 7
uint8_t vfo_GetInitStatus(void)
{
	uint8_t val = 0x00;
	vfo_readReg(VFO_STATUS_REG, &val);
	val = (val >> 7) & 0x01;
	return val;
}

uint8_t vfo_GetPLLAStatus(void)
{
	uint8_t val = 0x00;
	vfo_readReg(VFO_STATUS_REG, &val);
	return (val & VFO_PLLA_BIT);
}


uint8_t vfo_GetPLLBStatus(void)
{
	uint8_t val = 0x00;
	vfo_readReg(VFO_STATUS_REG, &val);
	return (val & VFO_PLLB_BIT);
}



///////////////////////////////////////////////
//Display lock state of PLLA and PLLB
//What LEDs are we going to use for this?
//
void vfo_DisplayLockState(void)
{
	//check the pll lock on a and b.  if both are
	//locked, turn on green.  if a not locked, turn on
	//red, and b not locked
	uint8_t aLock = vfo_GetPLLAStatus();
	uint8_t bLock = vfo_GetPLLBStatus();

	//a and b locked
	if ((!aLock) && (!bLock))
	{

	}
	//a not locked
	else if ((aLock) && (!bLock))
	{

	}
	//b not locked
	else if ((!aLock) && (bLock))
	{

	}
	//both out of lock
	else
	{

	}
}




///////////////////////////////////////////////
//i2c write
void vfo_writeReg(uint8_t reg, uint8_t data)
{
	uint8_t tx[2] = {reg, data};
    i2c_write(tx, 2);
}

///////////////////////////////////////////////
//i2c read
void vfo_readReg(uint8_t reg, uint8_t *data)
{
	uint8_t result = 0x00;

	//write
	uint8_t tx[1] = {reg};

    i2c_write(tx, 1);

    i2c_read(&result, 1);

	*data = result;
}



/////////////////////////////////////////////////
//reset pll a and b - write 0xA0 to
//VFO_PLL_RESET_REG
void vfo_resetPLL(void)
{
	vfo_writeReg(VFO_PLL_RESET_REG, 0xA0);
}






/////////////////////////////////////////////////
// Configure PLL
// Function Source (ie, I didn't write it)
// Author: Hans Summers, 2015
// Website: http://www.hanssummers.com
// Also found on QPR Labs Website, www.qrp-labs.com
// and a handful of other sites.
//
// Set up specified PLL with mult, num and denom
// mult is 15..90
// num is 0..1,048,575 (0xFFFFF)
// denom is 0..1,048,575 (0xFFFFF)
//
void SetupPLL(uint8_t pll, uint8_t mult, uint32_t num, uint32_t denom)
{
	uint32_t P1;					// PLL config register P1
	uint32_t P2;					// PLL config register P2
	uint32_t P3;					// PLL config register P3

	P1 = (uint32_t)(128 * ((float)num / (float)denom));
	P1 = (uint32_t)(128 * (uint32_t)(mult) + P1 - 512);
	P2 = (uint32_t)(128 * ((float)num / (float)denom));
	P2 = (uint32_t)(128 * num - denom * P2);
	P3 = denom;

	vfo_writeReg(pll + 0, (P3 & 0x0000FF00) >> 8);
	vfo_writeReg(pll + 1, (P3 & 0x000000FF));
	vfo_writeReg(pll + 2, (P1 & 0x00030000) >> 16);
	vfo_writeReg(pll + 3, (P1 & 0x0000FF00) >> 8);
	vfo_writeReg(pll + 4, (P1 & 0x000000FF));
	vfo_writeReg(pll + 5, ((P3 & 0x000F0000) >> 12) | ((P2 & 0x000F0000) >> 16));
	vfo_writeReg(pll + 6, (P2 & 0x0000FF00) >> 8);
	vfo_writeReg(pll + 7, (P2 & 0x000000FF));
}


/////////////////////////////////////////////////
// SetupMultisynth
// Function Source (ie, I didn't write it)
// Author: Hans Summers, 2015
// Website: http://www.hanssummers.com
// Also found on QPR Labs Website, www.qrp-labs.com
// and a handful of other sites.
//
// Set up MultiSynth with integer divider and R divider
// R divider is the bit value which is OR'ed onto the
// appropriate register, it is a #define in si5351a.h
//
void SetupMultisynth(uint8_t synth, uint32_t divider, uint8_t rDiv)
{
	uint32_t P1;					// Synth config register P1
	uint32_t P2;					// Synth config register P2
	uint32_t P3;					// Synth config register P3

	P1 = 128 * divider - 512;
	P2 = 0;							// P2 = 0, P3 = 1 forces an integer value for the divider
	P3 = 1;

	vfo_writeReg(synth + 0,   (P3 & 0x0000FF00) >> 8);
	vfo_writeReg(synth + 1,   (P3 & 0x000000FF));
	vfo_writeReg(synth + 2,   ((P1 & 0x00030000) >> 16) | rDiv);
	vfo_writeReg(synth + 3,   (P1 & 0x0000FF00) >> 8);
	vfo_writeReg(synth + 4,   (P1 & 0x000000FF));
	vfo_writeReg(synth + 5,   ((P3 & 0x000F0000) >> 12) | ((P2 & 0x000F0000) >> 16));
	vfo_writeReg(synth + 6,   (P2 & 0x0000FF00) >> 8);
	vfo_writeReg(synth + 7,   (P2 & 0x000000FF));
}





