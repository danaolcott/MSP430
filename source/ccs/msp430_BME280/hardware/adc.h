/*
 * adc.h
 *
 *  Created on: Sep 30, 2018
 *      Author: danao
 */

#ifndef HARDWARE_ADC_H_
#define HARDWARE_ADC_H_

#include <stddef.h>
#include <stdint.h>

void adc_init(void);
uint16_t adc_read(uint8_t channel);


#endif /* HARDWARE_ADC_H_ */
