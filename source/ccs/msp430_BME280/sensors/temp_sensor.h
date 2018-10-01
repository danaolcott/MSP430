/*
 * temp_sensor.h
 *
 *  Created on: Sep 30, 2018
 *      Author: danao
 *
 *  Temperature Sensor File for the MCP9700A-E/T0
 *  temperature sensor.
 *  A simple easy temp sensor with ADC output
 *
 *  Pinout (TO-92 package):
 *  1 - Vdd
 *  2 - Vout
 *  3 - GND
 *
 *
 */

#ifndef SENSORS_TEMP_SENSOR_H_
#define SENSORS_TEMP_SENSOR_H_

#include <stddef.h>
#include <stdint.h>

void temp_sensor_init(void);
float temp_sensor_readF(void);
uint16_t temp_sensor_getAverage(uint16_t* readings, uint16_t numValues);



#endif /* SENSORS_TEMP_SENSOR_H_ */
