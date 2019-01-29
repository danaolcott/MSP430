/*
 * BMP280.h
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
 */

#ifndef SENSORS_BMP280_H_
#define SENSORS_BMP280_H_



#include <stddef.h>
#include <stdint.h>


//datatypes - for use in mfg compensation equations
#define BMP280_S32_t			long signed int
#define BMP280_U32_t			long unsigned int
#define BMP280_S64_t			long long signed int

//When used in I2C mode
//#define BMP280_I2C_ADDRESS		((uint8_t)(0x76 << 1))		//upshifted
//#define BMP280_I2C_ADDRESS		(uint8_t)(0x76)					//not upshifted

//When used in SPI mode - read/write bit - Bit7 on the
//first byte - "or" with the address
#define BMP280_SPI_WRITE_BIT	0x00
#define BMP280_SPI_READ_BIT		0x80


#define BMP280_REG_ID			0xD0		//0x60
#define BMP280_REG_RESET		0xE0

#define BMP280_REG_STATUS		0xF3
#define BMP280_REG_CTRL_MEAS	0xF4
#define BMP280_REG_CONFIG		0xF5

#define BMP280_REG_PRESS_XLSB	0xF9
#define BMP280_REG_PRESS_LSB	0xF8
#define BMP280_REG_PRESS_MSB	0xF7

#define BMP280_REG_TEMP_XLSB	0xFC
#define BMP280_REG_TEMP_LSB		0xFB
#define BMP280_REG_TEMP_MSB		0xFA


typedef struct
{
	int16_t cTemperatureCInt;
	int16_t cTemperatureCFrac;
	int16_t cTemperatureFInt;
	int16_t cTemperatureFFrac;
	uint32_t cPressure;				//pa
}BMP280_Data;


//Calibration data presented in Table 16
typedef struct
{
	uint16_t dig_T1;		//temperature
	int16_t dig_T2;
	int16_t dig_T3;
	uint16_t dig_P1;		//pressure
	int16_t dig_P2;
	int16_t dig_P3;
	int16_t dig_P4;
	int16_t dig_P5;
	int16_t dig_P6;
	int16_t dig_P7;
	int16_t dig_P8;
	int16_t dig_P9;

}BMP280_CalibrationData;


void BMP280_init(void);
void BMP280_reset(void);
uint8_t BMP280_readChipID(void);
BMP280_Data BMP280_read(void);
void BMP280_readRegArray(uint8_t startAddress, uint8_t* buffer, uint8_t len);
void BMP280_sleep(void);
void BMP280_wakeup(void);
uint8_t BMP280_getStatus(void);



BMP280_S32_t BMP280_compensate_T_int32(BMP280_S32_t adc_T);
BMP280_U32_t BMP280_compensate_P_int64(BMP280_S32_t adc_P);



#endif /* SENSORS_BMP280_H_ */
