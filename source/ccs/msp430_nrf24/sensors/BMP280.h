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
 * Note: Much of this is similar to the BME280.
 */

#ifndef SENSORS_BMP280_H_
#define SENSORS_BMP280_H_



#include <stddef.h>
#include <stdint.h>


//datatypes - for use in mfg compensation equations
#define BME280_S32_t			long signed int
#define BME280_U32_t			long unsigned int
#define BME280_S64_t			long long signed int

//When used in I2C mode
//#define BME280_I2C_ADDRESS		((uint8_t)(0x76 << 1))		//upshifted
//#define BME280_I2C_ADDRESS		(uint8_t)(0x76)					//not upshifted

//When used in SPI mode - read/write bit - Bit7 on the
//first byte - "or" with the address
#define BME280_SPI_WRITE_BIT	0x00
#define BME280_SPI_READ_BIT		0x80


#define BME280_REG_HUM_LSB		0xFE
#define BME280_REG_HUM_MSB		0xFD

#define BME280_REG_TEMP_XLSB	0xFC
#define BME280_REG_TEMP_LSB		0xFB
#define BME280_REG_TEMP_MSB		0xFA

#define BME280_REG_PRESS_XLSB	0xF9
#define BME280_REG_PRESS_LSB	0xF8
#define BME280_REG_PRESS_MSB	0xF7

#define BME280_REG_CONFIG		0xF5
#define BME280_REG_CTRL_MEAS	0xF4
#define BME280_REG_STATUS		0xF3
#define BME280_REG_CTRL_HUM		0xF2

#define BME280_REG_RESET		0xE0
#define BME280_REG_ID			0xD0		//0x60


typedef struct
{
	uint32_t uPressure;
	uint32_t uTemperature;
	uint32_t uHumidity;

	int32_t cTemperatureCInt;
	int32_t cTemperatureCFrac;
	int32_t cTemperatureFInt;
	int32_t cTemperatureFFrac;

	uint32_t cPressureInt;
	uint32_t cPressureFrac;
	uint32_t cHumidityInt;
	uint32_t cHumidityFrac;

}BME280_Data;


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
	uint8_t dig_H1;			//humidity
	int16_t dig_H2;
	uint8_t dig_H3;

	int16_t dig_H4;
	int16_t dig_H5;
	int8_t dig_H6;


}BME280_CalibrationData;


void BME280_init(void);
void BME280_reset(void);
uint8_t BME280_readChipID(void);
BME280_Data BME280_read(void);

void BME280_sleep(void);
void BME280_wakeup(void);


BME280_S32_t BME280_compensate_T_int32(BME280_S32_t adc_T);
BME280_U32_t BME280_compensate_P_int64(BME280_S32_t adc_P);
BME280_U32_t BME280_compensate_H_int32(BME280_S32_t adc_H);



#endif /* SENSORS_BME280_H_ */
