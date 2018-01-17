/////////////////////////////////////////
/*
I2C Driver File Function Definitions
1/13/18
Dana Olcott

Low level register settings from Texas
Instruments files with some minor changes.
The following are instended to work with 
the si5351 i2c communcation interface.

*/
/////////////////////////////////////////
#include <msp430.h>
#include <stdint.h>

#include "i2c.h"
#include "TI_USCI_I2C_master.h"


/////////////////////////////////////////
//Check to see if the slave is connected
//
void i2c_init(void)
{
	//enable interrupts - required to read
	//the nack, ack..etc
	_EINT();

    //init for a transmit
    TI_USCI_I2C_transmitinit(I2C_ADDRESS, I2C_BUS_PRESCALE);

    //wait
    while (TI_USCI_I2C_notready());

	if (!TI_USCI_I2C_slave_present(I2C_ADDRESS))
    {
        while(1);
    }
}



////////////////////////////////////////////////
//multibyte write
void i2c_write(uint8_t* data, uint8_t len)
{
 	//enable interrupts - required to read ack/nack, etc
	_EINT();

	//setup i2c slave for writing - send the address
	//(7bits, not upshifted) and the prescale
	TI_USCI_I2C_transmitinit(I2C_ADDRESS,I2C_BUS_PRESCALE);
	while ( TI_USCI_I2C_notready() );		//wait

	//send the data
	TI_USCI_I2C_transmit(len, data);
	while ( TI_USCI_I2C_notready() );
}




////////////////////////////////////////////////
//multibyte read
//generic read that reads at the current
//reg address, generates a start, nack on
//last byte, and a stop
void i2c_read(uint8_t* data, uint8_t len)
{
	_EINT();

    //configure as master receiver
	TI_USCI_I2C_receiveinit(I2C_ADDRESS,I2C_BUS_PRESCALE);
	while ( TI_USCI_I2C_notready() );

	//read len bytes into data
	TI_USCI_I2C_receive(len, data);
	while ( TI_USCI_I2C_notready() );

}




