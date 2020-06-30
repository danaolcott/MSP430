/*
I2C Middle Layer for tx/rx with the BMP280

*/

#include <msp430.h>
#include <stdio.h>
#include <stdint.h>
#include <stddef.h>

#include "TI_USCI_I2C_master.h"
#include "i2c.h"


/////////////////////////////////////////
//Initialize i2c for a specific address
//
void i2c_init(uint8_t address7bit)
{
  //enable interrupts - required to read ack/nack, etc
  _EINT();

  //setup i2c slave for writing - send the address
  //(7bits, not upshifted) and the prescale
  TI_USCI_I2C_transmitinit(address7bit,I2C_BUS_PRESCALE);

  while ( TI_USCI_I2C_notready() )
  {
	  P1OUT ^= BIT0;		//toggle the pin fast
  }

  if (!TI_USCI_I2C_slave_present(address7bit))
  {
    while(1)
    {
    	P1OUT ^= BIT0;		//toggle the pin fast
    }
  }

}


////////////////////////////////////////////////
//multibyte write
void i2c_write(uint8_t address7bit, uint8_t* data, uint8_t len)
{
  //enable interrupts - required to read ack/nack, etc
  _EINT();

  //setup i2c slave for writing - send the address
  //(7bits, not upshifted) and the prescale
  TI_USCI_I2C_transmitinit(address7bit, I2C_BUS_PRESCALE);
  while ( TI_USCI_I2C_notready() );   //wait

  //send the data
  TI_USCI_I2C_transmit(len, data);
  while ( TI_USCI_I2C_notready() );
}




////////////////////////////////////////////////
//multibyte read
//generic read that reads at the current
//reg address, generates a start, nack on
//last byte, and a stop
void i2c_read(uint8_t address7bit, uint8_t* data, uint8_t len)
{
  _EINT();

    //configure as master receiver
  TI_USCI_I2C_receiveinit(address7bit, I2C_BUS_PRESCALE);
  while ( TI_USCI_I2C_notready() );

  //read len bytes into data
  TI_USCI_I2C_receive(len, data);
  while ( TI_USCI_I2C_notready() );

}



//////////////////////////////////////////////////////////////
//i2c Write to reg address
//Set up master as tx, send one address byte with
//no stop condition.s  Then send len bytes from
//data array.
//useful when address has to be appended with
//certain bits etc.  Ex light sensor.
//
void i2c_writeAddress(uint8_t address7bit, uint8_t address, uint8_t* data, uint8_t len)
{

	//enable interrupts - required to read ack/nack, etc
	_EINT();

	//setup i2c slave for writing - send the address
	//(7bits, not upshifted) and the prescale
	TI_USCI_I2C_transmitinit(address7bit, I2C_BUS_PRESCALE);
	while ( TI_USCI_I2C_notready() );		//wait

	//send the register address - no stop condition
	TI_USCI_I2C_transmit_nostop(1, &address);
	while ( TI_USCI_I2C_notready() );

	//send the data - generates the stop condition when complete
	TI_USCI_I2C_transmit(len, data);
	while ( TI_USCI_I2C_notready() );

}



/////////////////////////////////////////
//read memory bytes from starting address
//Steps:
//setup i2c device as master trasmitter
//and transmit address byte and generate
//the stop condition.  Then, set up i2c
//device as a receiver and read len bytes
//into data array.
//
//currently, this
void i2c_readAddress(uint8_t address7bit, uint8_t address, uint8_t* data, uint8_t len)
{
	_EINT();

	//setup i2c slave for writing - send the address
	//(7bits, not upshifted) and the prescale
	TI_USCI_I2C_transmitinit(address7bit, I2C_BUS_PRESCALE);
	while ( TI_USCI_I2C_notready() );


	//send the register address - no stop condition
//	TI_USCI_I2C_transmit_nostop(1, &address);
	TI_USCI_I2C_transmit(1, &address);

    //wait a bit
    while ( TI_USCI_I2C_notready() );

    //setup i2c as a receiver
	TI_USCI_I2C_receiveinit(address7bit, I2C_BUS_PRESCALE);

    //wait a bit
	while ( TI_USCI_I2C_notready() );

	//read len bytes into data
	TI_USCI_I2C_receive(len, data);

    //wait a bit
	while ( TI_USCI_I2C_notready() );

}





