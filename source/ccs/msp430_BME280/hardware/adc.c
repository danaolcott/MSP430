/*
 * adc.c
 *
 *  Created on: Sep 30, 2018
 *      Author: danao
 *
 *
 */

#include <msp430g2553.h>
#include <stdio.h>
#include <stddef.h>
#include <stdint.h>

#include "adc.h"


static void adc_dummy_delay(uint32_t value)
{
	volatile uint32_t temp = value;
	while (temp > 0)
		temp--;
}

//////////////////////////////////////////////
//Configure ADC Channel 5 as ADC, P1.5, Pin 7
//
void adc_init(void)
{
	//enable analog output channels - A5, P1.5, Pin 7
	ADC10AE0 |= BIT5;
}


////////////////////////////////////////////////
//reads and return the ADC 10-bit value for
//the channel provided.  Assumes the channel / pin
//is properly configured as ADC
uint16_t adc_read(uint8_t channel)
{
	uint16_t value = 0x00;

	//enable analog output channels - ADC10AE0 register
	//see adc_init

	//ADC core is configured using the following registers:
	//ADC10CTL0 and ADC10CTL1

	//REFON = 1 - use internal ref
	//REF2_5V = 0 - use internal ref voltage = 1.5v.

	//64 adc10 clocks, use internal reference, enable internal
	//reference clock use 1.5 v as ref., enable adc
	ADC10CTL0 = ADC10SHT_3 + SREF_1 + REFON + ADC10ON;


//	ADC10CTL0 = ADC10SHT_3 + SREF_1 + REFON + ADC10ON;
	//set the channel bits - 12 - 15 in ADC10CTL1 reg
	//assume 0 to 10 for valid channels
	if (channel > 10)
	{
		return 0;
	}

	//ADC10CTL1 register - sets the input channel, the
	//clock divider, clock source (default is the ADC10osc)
	//try the memory clock - ADC10SSEL_2
	ADC10CTL1 = channel << 12;		//input channel
	ADC10CTL1 |= ADC10DIV_3;		//clock divider 2
//	ADC10CTL1 |= ADC10SSEL_2;		//MCLK

	adc_dummy_delay(20000);

	ADC10CTL0 |= ENC + ADC10SC;		//ENC bit set to start conversion
	adc_dummy_delay(20000);
	while (ADC10CTL1 & ADC10BUSY);	//wait for the conversion

	value = ADC10MEM;

	return value;

}

