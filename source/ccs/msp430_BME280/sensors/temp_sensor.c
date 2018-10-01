/*
 * temp_sensor.c
 *
 *  Created on: Sep 30, 2018
 *      Author: danao
 *
 *  Temp sensor
 *
 *  Sensor Transfer Function:
	Vout = Tc x Ta + V0c    where:
    Vout    = output voltage on pin2
    Tc      = Temp Coeff                    10mV / C
    Ta      = Ambient Temp
    V0c     = Output voltage at 0 deg C     500mv

	Ta = (Vout - V0c) / Tc
 *
 */


#include "temp_sensor.h"
#include "adc.h"



/////////////////////////////////////////
void temp_sensor_init(void)
{

}

///////////////////////////////////////
//Returns the temp in Farenheight as floating
//point value.  Temp sensor connected to
//channel 5, P1.5 on MSP430
//
float temp_sensor_readF(void)
{
	uint16_t readings[5] = {0x00};
	int i = 0x00;
	for (i = 0 ; i < 5 ; i++)
	{
		readings[i] = adc_read(5) & 0x3FF;
	}

	//pointer and length - sort, remove high, low and get average
	uint16_t currentADC = temp_sensor_getAverage(readings, 5);

	//convert 10bit adc to temp using the
	//following equation - See Datasheet
	//stable 3.3 supply
//    float currentMV = ((float)currentADC / 1024.0) * 3300.0;

    //configured to use internal ref voltage = 1.5v
    float currentMV = ((float)currentADC / 1024.0) * 1500.0;

    float currentTEMP = (currentMV - 500.0) / 10.0;     //centigrade
    currentTEMP = ((currentTEMP * 9.0 / 5.0) + 32.0);

    return currentTEMP;
}


/////////////////////////////////////////////////////////////////
//Compute the average of a list of readings.  Sorts, removes the
//high and low, and gets the average of middle 3.
uint16_t temp_sensor_getAverage(uint16_t* data, uint16_t size)
{
    unsigned int i, j, val;

    for (i = 0 ; i < size ; i++)
    {
        for (j = i + 1; j < size ; j++)
        {
            if(data[i] > data[j])
            {
                val = data[i];
                data[i] = data[j];
                data[j] = val;
            }
        }
    }

    //with array of data sorted, take the average
    //of the middle
    uint16_t sum = 0;

    for (i = 1 ; i < size - 1 ; i++)
    	sum += data[i];

    return (sum / (size - 2));
}




