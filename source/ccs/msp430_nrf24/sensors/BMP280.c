/*
 * BMP280.c
 *
 *  Created on: Sep 16, 2018
 *      Author: danao
 *
 * Controller file for the BMP280 Barometer / Humidity Sensor.
 * the purpose of this file is to provide a simple interface for
 * reading temperature, pressure, and humidity.  The values computed
 * use the equations published in the datasheet.
 *
 * Note: Much of this is similar to the BMP280.  The BMP280 only
 * reads temperature and pressure.  The BMP280 also reads humidity.
 *
 * Updated 1/23/19 - For use with SPI
 * MSP430 - CS pin - P2.3.  Configure in the init function
 * refactor all the BMP280 to BMP280, update register settings
 * as needed.
 *
 *
 */

#include <stddef.h>
#include <stdint.h>

#include <msp430.h>
#include "BMP280.h"
#include "spi.h"

/////////////////////////////////////////////////////////
static void BMP280_dummyDelay(uint32_t delay);
static uint8_t BMP280_readReg(uint8_t reg);
//void BMP280_readRegArray(uint8_t startAddress, uint8_t* buffer, uint8_t len);
static void BMP280_writeReg(uint8_t reg, uint8_t value);
static void BMP280_readCalibrationValues(void);

//helper functions for the BMP280 when used in SPI mode
//shared with another SPI device
static void BMP280_select(void);
static void BMP280_deselect(void);


//////////////////////////////////////////////////////////
static volatile BMP280_CalibrationData mCalibrationData;

BMP280_S32_t t_fine;		//used in the temperature, pressure, humidity correction functions

//////////////////////////////////////////////////////////
//Configure for reading pressure, humidity, temp, I2C
//Follows Table 9 in the datasheet for best performance
//for use with indoor navigation.  Minimize noise in
//the pressure reading.
//
void BMP280_init(void)
{
	//Configure P2.3 as CS pin
	P2DIR |= BIT3;
	P2OUT |= BIT3;		//disable

	t_fine = 0x00;				//global used in compensation equations.

	//wait a bit
	BMP280_dummyDelay(20000);
	BMP280_reset();
	BMP280_dummyDelay(20000);

	//config the following:
	//Based on the datasheet, the least amount of noise in the
	//pressure sensor is with 16x oversampling on pressure, 2x OS on
	//temperature, and IR filter coeff. of 16.
	//
	//#define BMP280_REG_CTRL_MEAS	0xF4 - temp and pressure options.
	//bits 7-5 - osrs_t - temp 010 - 2x sampling
	//bits 4-2 - osrs_p - pressure 101 - 16x sampling
	//bits 1-0 - mode.  set to 11 for normal mode., set to 00 for sleep mode
	//result = 01010100  = 0x54 - sleep
	BMP280_writeReg(BMP280_REG_CTRL_MEAS, 0x54);		//enable and sleep

	//#define BMP280_REG_CONFIG		0xF5
	//readings should be while in sleep mode or
	//else they are ignored.
	//standby 0.5ms - 000
	//filter settings - on - 16x -  100
	//disable SPI 3-wire - 0 - use 4 wire or i2c if bit 0 = 0
	//result - 0001 0000 = 0x10
	BMP280_writeReg(BMP280_REG_CONFIG, 0x10);

	//wakeup - bits 0 and 1 high for reg BMP280_REG_CTRL_MEAS
	BMP280_writeReg(BMP280_REG_CTRL_MEAS, 0x57);

	//Load Calibration data into ram - loads mCalibrationData
	BMP280_readCalibrationValues();

}


/////////////////////////////////////////////////////
//Software reset - write 0xB6 to reset register
//readback is always 0x00
void BMP280_reset(void)
{
	BMP280_writeReg(BMP280_REG_RESET, 0xB6);
}


/////////////////////////////////////////////////////
//Chip ID = 0x60
uint8_t BMP280_readChipID(void)
{
	uint8_t id = BMP280_readReg(BMP280_REG_ID);
	return id;
}


/////////////////////////////////////////////////////
//Read all sensors - 6 bytes starting at address
//0xF7.  Combine read bytes into values, apply correction
//factors.
BMP280_Data BMP280_read(void)
{
	uint8_t data[6] = {0x00};
	uint32_t msb, lsb, xlsb = 0x00;
	uint32_t uPressure, uTemperature = 0x00;

	BMP280_Data result;

	BMP280_readRegArray(BMP280_REG_PRESS_MSB, data, 6);

	//Pressure - 20bits
	msb = ((uint32_t)data[0]) & 0xFF;
	lsb = ((uint32_t)data[1]) & 0xFF;
	xlsb = ((uint32_t)(data[2] >> 4)) & 0x0F;
	uPressure = ((msb << 12) | (lsb << 4) | (xlsb));

	//temperature - 20bits
	msb = ((uint32_t)data[3]) & 0xFF;
	lsb = ((uint32_t)data[4]) & 0xFF;
	xlsb = ((uint32_t)(data[5] >> 4)) & 0x0F;
	uTemperature = ((msb << 12) | (lsb << 4) | (xlsb));

	result.uPressure = uPressure;
	result.uTemperature = uTemperature;

	//compute the corrected values using the raw data
	//and calibration values as described in the datasheet

	//compute the compensated temp
	BMP280_S32_t cTemperature = BMP280_compensate_T_int32((BMP280_S32_t)uTemperature);	//div 100 to get int, % 100 for frac
	BMP280_U32_t cPressure = BMP280_compensate_P_int64((BMP280_S32_t)uPressure);		//divide 256 for integer

	//integer and fractional - celcius
	result.cTemperatureCInt = cTemperature / 100;
	result.cTemperatureCFrac = cTemperature % 100;

	BMP280_S32_t cTemperatureF = cTemperature * 9 / 5;
	cTemperatureF += 3200;

	result.cTemperatureFInt = cTemperatureF / 100;
	result.cTemperatureFFrac = cTemperatureF % 100;

	result.cPressureInt = cPressure / 256;
	result.cPressureFrac = cPressure % 256;

	//...................

	return result;
}



////////////////////////////////////////////////
//Put the sensor into sleep mode
void BMP280_sleep(void)
{
	uint8_t value = BMP280_readReg(BMP280_REG_CTRL_MEAS);
	value &=~ 0x03;				//bits 0 and 1 low

	BMP280_writeReg(BMP280_REG_CTRL_MEAS, value);
}

///////////////////////////////////////////////////
//Wake up the sensor and put into continuous
//reading mode.  CTRL_MEAS register, bits 0:1, 11
void BMP280_wakeup(void)
{
	uint8_t value = BMP280_readReg(BMP280_REG_CTRL_MEAS);
	value |= 0x03;				//bits 0 and 1 high

	BMP280_writeReg(BMP280_REG_CTRL_MEAS, value);
}




/////////////////////////////////////////////////////
//
void BMP280_dummyDelay(uint32_t delay)
{
	volatile uint32_t temp = delay;
	while (temp > 0)
		temp--;
}


void BMP280_select(void)
{
	P2OUT &=~ BIT3;		//enable
}


void BMP280_deselect(void)
{
	P2OUT |= BIT3;		//disable
}


////////////////////////////////////////////////////
//Read Register.
//SPI mode - only 7 bits of the reg address are used,
//with Bit 7 replaced with read/write bit.
//#define BMP280_SPI_WRITE_BIT	0x00
//#define BMP280_SPI_READ_BIT		0x80
//
//SPI - send address as a read then read byte
uint8_t BMP280_readReg(uint8_t reg)
{
	uint8_t result = 0x00;
	uint8_t address = BMP280_SPI_READ_BIT | (reg & 0x7F);		//clear bit 7 of register

	BMP280_select();
	SPI_tx(address);
	result = SPI_rx();
	BMP280_deselect();

	return result;
}


//////////////////////////////////////////////////
//Write 2 bytes - address as a write, followed by value
//
void BMP280_writeReg(uint8_t reg, uint8_t value)
{
	uint8_t address = BMP280_SPI_WRITE_BIT | (reg & 0x7F);

	BMP280_select();
	SPI_tx(address);
	SPI_tx(value);
	BMP280_deselect();
}

/////////////////////////////////////////////////////////
//Read continuous address range, starting at startAddress
//into buffer, length = len bytes.  Read address auto
//increments.
//
void BMP280_readRegArray(uint8_t startAddress, uint8_t* buffer, uint8_t len)
{
	uint8_t i = 0x00;
	uint8_t address = BMP280_SPI_READ_BIT | (startAddress & 0x7F);

	BMP280_select();
	SPI_tx(address);

	for (i = 0 ; i < len ; i++)
	{
		buffer[i] = SPI_rx();
	}

	BMP280_deselect();
}







//////////////////////////////////////////////////////
//Clears all previous readings, reads registers containing
//calibration data for the ic, computes the signed/unsigned
//calibration data.  For use in computing the compensated
//temp, and pressure.
//Calibration data stored in registers 0x88 - 0xA1
//values at 0xA0 and 0xA1 reserved.
//
void BMP280_readCalibrationValues(void)
{
	//read calibration registers
	uint8_t reg[24] = {0x00};					//temp and pressure

	BMP280_readRegArray(0x88, reg, 24);			//read 24 bytes of cal data

	mCalibrationData.dig_T1 = ((uint16_t)reg[0]) + (((uint16_t)reg[1]) * 256);
	mCalibrationData.dig_T2 = ((int16_t)reg[2]) + (((int16_t)reg[3]) * 256);
	mCalibrationData.dig_T3 = ((int16_t)reg[4]) + (((int16_t)reg[5]) * 256);

	mCalibrationData.dig_P1 = ((uint16_t)reg[6]) + (((uint16_t)reg[7]) * 256);
	mCalibrationData.dig_P2 = ((int16_t)reg[8]) + (((int16_t)reg[9]) * 256);
	mCalibrationData.dig_P3 = ((int16_t)reg[10]) + (((int16_t)reg[11]) * 256);
	mCalibrationData.dig_P4 = ((int16_t)reg[12]) + (((int16_t)reg[13]) * 256);
	mCalibrationData.dig_P5 = ((int16_t)reg[14]) + (((int16_t)reg[15]) * 256);
	mCalibrationData.dig_P6 = ((int16_t)reg[16]) + (((int16_t)reg[17]) * 256);
	mCalibrationData.dig_P7 = ((int16_t)reg[18]) + (((int16_t)reg[19]) * 256);
	mCalibrationData.dig_P8 = ((int16_t)reg[20]) + (((int16_t)reg[21]) * 256);
	mCalibrationData.dig_P9 = ((int16_t)reg[22]) + (((int16_t)reg[23]) * 256);

}








/////////////////////////////////////////////////////////////////////////////////
//Function Obtained Directly from the Datasheet
// Returns temperature in DegC, resolution is 0.01 DegC. Output value of “5123” equals 51.23 DegC.
// t_fine carries fine temperature as global value
//BMP280_S32_t t_fine;
//t_fine is defined above - global used for pressure calcs as well.
//
BMP280_S32_t BMP280_compensate_T_int32(BMP280_S32_t adc_T)
{
	BMP280_S32_t var1, var2, T;

	var1 = ((((adc_T>>3) - ((BMP280_S32_t)mCalibrationData.dig_T1 << 1))) * ((BMP280_S32_t)mCalibrationData.dig_T2)) >> 11;
	var2 = (((((adc_T >> 4) - ((BMP280_S32_t)mCalibrationData.dig_T1)) * ((adc_T >> 4) - ((BMP280_S32_t)mCalibrationData.dig_T1))) >> 12) * ((BMP280_S32_t)mCalibrationData.dig_T3)) >> 14;

	t_fine = var1 + var2;
	T  = (t_fine * 5 + 128) >> 8;
	return T;
}



/////////////////////////////////////////////////////////////////////////////////
//Function Obtained Directly from the Datasheet
// Returns pressure in Pa as unsigned 32 bit integer in Q24.8 format (24 integer bits and 8 fractional bits).
// Output value of “24674867” represents 24674867/256 = 96386.2 Pa = 963.862 hPa
BMP280_U32_t BMP280_compensate_P_int64(BMP280_S32_t adc_P)
{
	BMP280_S64_t var1, var2, p;
	var1 = ((BMP280_S64_t)t_fine) - 128000;

	var2 = var1 * var1 * (BMP280_S64_t)mCalibrationData.dig_P6;
	var2 = var2 + ((var1*(BMP280_S64_t)mCalibrationData.dig_P5)<<17);
	var2 = var2 + (((BMP280_S64_t)mCalibrationData.dig_P4)<<35);
	var1 = ((var1 * var1 * (BMP280_S64_t)mCalibrationData.dig_P3)>>8) + ((var1 * (BMP280_S64_t)mCalibrationData.dig_P2)<<12);
	var1 = (((((BMP280_S64_t)1)<<47)+var1))*((BMP280_S64_t)mCalibrationData.dig_P1)>>33;
	if (var1 == 0)
	{
		return 0; // avoid exception caused by division by zero
	}

	p = 1048576-adc_P;
	p = (((p<<31)-var2)*3125)/var1;
	var1 = (((BMP280_S64_t)mCalibrationData.dig_P9) * (p>>13) * (p>>13)) >> 25;
	var2 = (((BMP280_S64_t)mCalibrationData.dig_P8) * p) >> 19;  p = ((p + var1 + var2) >> 8) + (((BMP280_S64_t)mCalibrationData.dig_P7)<<4);
	return (BMP280_U32_t)p;
}

