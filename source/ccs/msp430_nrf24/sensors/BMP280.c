/*
 * BME280.c
 *
 *  Created on: Sep 16, 2018
 *      Author: danao
 *
 * Controller file for the BME280 Barometer / Humidity Sensor.
 * the purpose of this file is to provide a simple interface for
 * reading temperature, pressure, and humidity.  The values computed
 * use the equations published in the datasheet.
 *
 * Updated 1/23/19 - For use with SPI
 * MSP430 - CS pin - P2.3.  Configure in the init function
 *
 *
 */

#include <stddef.h>
#include <stdint.h>

#include <msp430.h>
#include "BMP280.h"
#include "spi.h"

/////////////////////////////////////////////////////////
static void BME280_dummyDelay(uint32_t delay);
static uint8_t BME280_readReg(uint8_t reg);
void BME280_readRegArray(uint8_t startAddress, uint8_t* buffer, uint8_t len);
static void BME280_writeReg(uint8_t reg, uint8_t value);
static void BME280_readCalibrationValues(void);

//helper functions for the BME280 when used in SPI mode
//shared with another SPI device
static void BME280_select(void);
static void BME280_deselect(void);


//////////////////////////////////////////////////////////
static volatile BME280_CalibrationData mCalibrationData;

BME280_S32_t t_fine;		//used in the temperature, pressure, humidity correction functions

//////////////////////////////////////////////////////////
//Configure for reading pressure, humidity, temp, I2C
//Follows Table 9 in the datasheet for best performance
//for use with indoor navigation.  Minimize noise in
//the pressure reading.
//
void BME280_init(void)
{
	//Configure P2.3 as CS pin
	P2DIR |= BIT3;
	P2OUT |= BIT3;		//disable



	t_fine = 0x00;				//global used in compensation equations.

	//wait a bit
	BME280_dummyDelay(20000);
	BME280_reset();
	BME280_dummyDelay(20000);

	//config the following:

	//#define BME280_REG_CTRL_HUM	0xF2 - humidity reading options
	//bits 0-2, oversampling.  Set to 000 to skip the reading and
	//constant outpput.  For 1x oversample, use 001
	BME280_writeReg(BME280_REG_CTRL_HUM, 0x01);			//1x oversample - enabled.

	//Based on the datasheet, the least amount of noise in the
	//pressure sensor is with 16x oversampling on pressure, 2x OS on
	//temperature, and IR filter coeff. of 16.
	//
	//#define BME280_REG_CTRL_MEAS	0xF4 - temp and pressure options.
	//bits 7-5 - osrs_t - temp 010 - 2x sampling
	//bits 4-2 - osrs_p - pressure 101 - 16x sampling
	//bits 1-0 - mode.  set to 11 for normal mode., set to 00 for sleep mode
	//result = 01010100  = 0x54 - sleep
	BME280_writeReg(BME280_REG_CTRL_MEAS, 0x54);		//enable and sleep

	//#define BME280_REG_CONFIG		0xF5
	//readings should be while in sleep mode or
	//else they are ignored.
	//standby 0.5ms - 000
	//filter settings - on - 16x -  100
	//disable SPI 3-wire - 0 - use 4 wire or i2c if bit 0 = 0
	//result - 0001 0000 = 0x10
	BME280_writeReg(BME280_REG_CONFIG, 0x10);

	//wakeup - bits 0 and 1 high for reg BME280_REG_CTRL_MEAS
	BME280_writeReg(BME280_REG_CTRL_MEAS, 0x57);

	//Load Calibration data into ram - loads mCalibrationData
	BME280_readCalibrationValues();

}


/////////////////////////////////////////////////////
//Software reset - write 0xB6 to reset register
//readback is always 0x00
void BME280_reset(void)
{
	BME280_writeReg(BME280_REG_RESET, 0xB6);
}


/////////////////////////////////////////////////////
//Chip ID = 0x60
uint8_t BME280_readChipID(void)
{
	uint8_t id = BME280_readReg(BME280_REG_ID);
	return id;
}


/////////////////////////////////////////////////////
//Read all sensors - 8 bytes starting at address
//0xF7.  Combine read bytes into values, apply correction
//factors.
BME280_Data BME280_read(void)
{
	uint8_t data[8] = {0x00};
	uint32_t msb, lsb, xlsb = 0x00;
	uint32_t uPressure, uTemperature, uHumidity = 0x00;

	BME280_Data result;

	BME280_readRegArray(BME280_REG_PRESS_MSB, data, 8);

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

	//humidity - 16 bits
	msb = ((uint32_t)data[6]) & 0xFF;
	lsb = ((uint32_t)data[7]) & 0xFF;
	uHumidity = ((msb << 8) | (lsb));


	result.uPressure = uPressure;
	result.uTemperature = uTemperature;
	result.uHumidity = uHumidity;

	//compute the corrected values using the raw data
	//and calibration values as described in the datasheet

	//compute the compensated temp
	BME280_S32_t cTemperature = BME280_compensate_T_int32((BME280_S32_t)uTemperature);	//div 100 to get int, % 100 for frac
	BME280_U32_t cPressure = BME280_compensate_P_int64((BME280_S32_t)uPressure);		//divide 256 for integer
	BME280_U32_t cHumidity = BME280_compensate_H_int32((BME280_S32_t)uHumidity);		//divide 1024 for integer

	//integer and fractional - celcius
	result.cTemperatureCInt = cTemperature / 100;
	result.cTemperatureCFrac = cTemperature % 100;

	BME280_S32_t cTemperatureF = cTemperature * 9 / 5;
	cTemperatureF += 3200;

	result.cTemperatureFInt = cTemperatureF / 100;
	result.cTemperatureFFrac = cTemperatureF % 100;

	result.cPressureInt = cPressure / 256;
	result.cPressureFrac = cPressure % 256;

	result.cHumidityInt = cHumidity / 1024;
	result.cHumidityFrac = cHumidity % 1024;

	//...................

	return result;
}








////////////////////////////////////////////////
//Put the sensor into sleep mode
void BME280_sleep(void)
{
	uint8_t value = BME280_readReg(BME280_REG_CTRL_MEAS);
	value &=~ 0x03;				//bits 0 and 1 low

	BME280_writeReg(BME280_REG_CTRL_MEAS, value);
}

///////////////////////////////////////////////////
//Wake up the sensor and put into continuous
//reading mode.  CTRL_MEAS register, bits 0:1, 11
void BME280_wakeup(void)
{
	uint8_t value = BME280_readReg(BME280_REG_CTRL_MEAS);
	value |= 0x03;				//bits 0 and 1 high

	BME280_writeReg(BME280_REG_CTRL_MEAS, value);
}




/////////////////////////////////////////////////////
//
void BME280_dummyDelay(uint32_t delay)
{
	volatile uint32_t temp = delay;
	while (temp > 0)
		temp--;
}


void BME280_select(void)
{
	P2OUT &=~ BIT3;		//enable
}


void BME280_deselect(void)
{
	P2OUT |= BIT3;		//disable
}


////////////////////////////////////////////////////
//Standard write/read I2C function
//
//When used in SPI mode - read/write bit - Bit7 on the
//first byte - "or" with the address
//#define BME280_SPI_WRITE_BIT	0x00
//#define BME280_SPI_READ_BIT		0x80
//
//SPI - send address as a read then read byte
uint8_t BME280_readReg(uint8_t reg)
{
	uint8_t result = 0x00;
	uint8_t address = BME280_SPI_READ_BIT | reg;

	BME280_select();
	SPI_tx(address);
	result = SPI_rx();
	BME280_deselect();

	return result;
}


//////////////////////////////////////////////////
//Write 2 bytes - address as a write, followed by value
//
void BME280_writeReg(uint8_t reg, uint8_t value)
{
	uint8_t address = BME280_SPI_WRITE_BIT | reg;

	BME280_select();
	SPI_tx(address);
	SPI_tx(value);
	BME280_deselect();
}

/////////////////////////////////////////////////////////
//Read continuous address range, starting at startAddress
//into buffer, length = len bytes.  Read address auto
//increments.
//
void BME280_readRegArray(uint8_t startAddress, uint8_t* buffer, uint8_t len)
{
	uint8_t i = 0x00;
	uint8_t address = BME280_SPI_READ_BIT | startAddress;

	BME280_select();
	SPI_tx(address);

	for (i = 0 ; i < len ; i++)
	{
		buffer[i] = SPI_rx();
	}

	BME280_deselect();
}







//////////////////////////////////////////////////////
//Clears all previous readings, reads registers containing
//calibration data for the ic, computes the signed/unsigned
//calibration data.  For use in computing the compensated
//humidity, temp, and pressure.
//
void BME280_readCalibrationValues(void)
{
	//read calibration registers
	uint8_t reg[24] = {0x00};
	uint8_t a1 = 0x00;
	uint8_t e1[7] = {0x00};

	BME280_readRegArray(0x88, reg, 24);			//read 24 bytes of cal data
	BME280_readRegArray(0xA1, &a1, 1);			//read 1 byte of cal data
	BME280_readRegArray(0xE1, e1, 7);			//read 3 bytes of cal data

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

	mCalibrationData.dig_H1 = a1;
	mCalibrationData.dig_H2 = ((int16_t)e1[0]) + (((int16_t)e1[1]) * 256);
	mCalibrationData.dig_H3 = e1[2];

	mCalibrationData.dig_H4 = ((int16_t)(e1[4] & 0x0F)) + (((int16_t)e1[3]) * 16);
	mCalibrationData.dig_H5 = ((int16_t)((e1[4] & 0xF0) >> 4)) + (((int16_t)e1[5]) * 16);
	mCalibrationData.dig_H6 = e1[6];

}








/////////////////////////////////////////////////////////////////////////////////
//Function Obtained Directly from the Datasheet
// Returns temperature in DegC, resolution is 0.01 DegC. Output value of “5123” equals 51.23 DegC.
// t_fine carries fine temperature as global value
//BME280_S32_t t_fine;
//t_fine is defined above - global used for pressure calcs as well.
//
BME280_S32_t BME280_compensate_T_int32(BME280_S32_t adc_T)
{
	BME280_S32_t var1, var2, T;

	var1 = ((((adc_T>>3) - ((BME280_S32_t)mCalibrationData.dig_T1 << 1))) * ((BME280_S32_t)mCalibrationData.dig_T2)) >> 11;
	var2 = (((((adc_T >> 4) - ((BME280_S32_t)mCalibrationData.dig_T1)) * ((adc_T >> 4) - ((BME280_S32_t)mCalibrationData.dig_T1))) >> 12) * ((BME280_S32_t)mCalibrationData.dig_T3)) >> 14;

	t_fine = var1 + var2;
	T  = (t_fine * 5 + 128) >> 8;
	return T;
}



/////////////////////////////////////////////////////////////////////////////////
//Function Obtained Directly from the Datasheet
// Returns pressure in Pa as unsigned 32 bit integer in Q24.8 format (24 integer bits and 8 fractional bits).
// Output value of “24674867” represents 24674867/256 = 96386.2 Pa = 963.862 hPa
BME280_U32_t BME280_compensate_P_int64(BME280_S32_t adc_P)
{
	BME280_S64_t var1, var2, p;
	var1 = ((BME280_S64_t)t_fine) - 128000;

	var2 = var1 * var1 * (BME280_S64_t)mCalibrationData.dig_P6;
	var2 = var2 + ((var1*(BME280_S64_t)mCalibrationData.dig_P5)<<17);
	var2 = var2 + (((BME280_S64_t)mCalibrationData.dig_P4)<<35);
	var1 = ((var1 * var1 * (BME280_S64_t)mCalibrationData.dig_P3)>>8) + ((var1 * (BME280_S64_t)mCalibrationData.dig_P2)<<12);
	var1 = (((((BME280_S64_t)1)<<47)+var1))*((BME280_S64_t)mCalibrationData.dig_P1)>>33;
	if (var1 == 0)
	{
		return 0; // avoid exception caused by division by zero
	}

	p = 1048576-adc_P;
	p = (((p<<31)-var2)*3125)/var1;
	var1 = (((BME280_S64_t)mCalibrationData.dig_P9) * (p>>13) * (p>>13)) >> 25;
	var2 = (((BME280_S64_t)mCalibrationData.dig_P8) * p) >> 19;  p = ((p + var1 + var2) >> 8) + (((BME280_S64_t)mCalibrationData.dig_P7)<<4);
	return (BME280_U32_t)p;
}

/////////////////////////////////////////////////////////////////////////////////
//Function Obtained Directly from the Datasheet
// Returns humidity in %RH as unsigned 32 bit integer in Q22.10 format (22 integer and 10 fractional bits).
// Output value of “47445” represents 47445/1024 = 46.333 %RH

BME280_U32_t BME280_compensate_H_int32(BME280_S32_t adc_H)
{
	BME280_S32_t v_x1_u32r;
	v_x1_u32r = (t_fine - ((BME280_S32_t)76800));
	v_x1_u32r = (((((adc_H << 14) - (((BME280_S32_t)mCalibrationData.dig_H4) << 20) - (((BME280_S32_t)mCalibrationData.dig_H5) * v_x1_u32r)) + ((BME280_S32_t)16384)) >> 15) * (((((((v_x1_u32r * ((BME280_S32_t)mCalibrationData.dig_H6)) >> 10) * (((v_x1_u32r *
		((BME280_S32_t)mCalibrationData.dig_H3)) >> 11) + ((BME280_S32_t)32768))) >> 10) + ((BME280_S32_t)2097152)) * ((BME280_S32_t)mCalibrationData.dig_H2) + 8192) >> 14));
	v_x1_u32r = (v_x1_u32r - (((((v_x1_u32r >> 15) * (v_x1_u32r >> 15)) >> 7) * ((BME280_S32_t)mCalibrationData.dig_H1)) >> 4));
	v_x1_u32r = (v_x1_u32r < 0 ? 0 : v_x1_u32r);
	v_x1_u32r = (v_x1_u32r > 419430400 ? 419430400 : v_x1_u32r);
	return (BME280_U32_t)(v_x1_u32r>>12);
}


