/*
 * 1/12/18
 * MSP430 Example - i2c
 *
 *
 */
//includes
#include <msp430.h>
#include <stdio.h>
#include <stdint.h>
#include <stddef.h>
#include <string.h>

#include "TI_USCI_I2C_master.h"


/////////////////////////////////////////////////////
/*
 * Notes:
 * Use the real i2c address - not the upshifted address
 * Uses P1.6 (SCL) and P1.7 (SDA).  Remove the jumper on the launchpad board.
 *
 */
/////////////////////////////////////////////////////

//#define I2C_ADDRESS				(((uint8_t)0x29) << 1)

//#define I2C_ADDRESS				(uint8_t)0x29	//TSL2561 - light sensor - grounded
#define I2C_ADDRESS				(uint8_t)0x39	//TSL2561 - light sensor - floating
//#define I2C_ADDRESS				(uint8_t)0x60		//si5351 - vfo

//#define I2C_ADDRESS				(uint8_t)0x1E
//#define I2C_ADDRESS				(((uint8_t)0x1E) << 1)

#define I2C_BUS_PRESCALE		((uint8_t)(0x28))	//400khz
//#define I2C_BUS_PRESCALE		((uint8_t)(0xA0))	//100khz



unsigned char timercounter;
unsigned char array[40] = { 0x1, 0x2, 0x3, 0x4, 0x5, 0x6, 0x7, 0x8, 0x9, 0xa, 0xb };
unsigned char store[40] = { 13, 13, 13, 13, 13, 13, 13, 0, 0, 0};



//prototypes
void dummy_delay(unsigned int temp);

void delay_ms(volatile int ticks);
void TimeDelay_Decrement(void);
void GPIO_init(void);
void TimerA_init(void);
void Interrupt_init(void);
void LED_RED_TOGGLE(void);


void LightSensorWrite(uint8_t* data, uint8_t len);
void LightSensorRead(uint8_t* data, uint8_t len);
void LightSensorWriteAddress(uint8_t address, uint8_t* data, uint8_t len);
void LightSensorReadAddress(uint8_t address, uint8_t* data, uint8_t len);


//test functions - vfo read and write
void vfo_writeReg(uint8_t reg, uint8_t data);
void vfo_readReg(uint8_t reg, uint8_t* data);

void i2c_init(void);
void i2c_write(uint8_t* data, uint8_t len);
void i2c_read(uint8_t* data, uint8_t len);

void i2c_writeAddress(uint8_t address, uint8_t* data, uint8_t len);
void i2c_readAddress(uint8_t address, uint8_t* data, uint8_t len);



uint8_t rxData[16];


//global variables
static volatile int TimeDelay;

//main program
void main(void)
{
	//disable the watchdog timer
	WDTCTL = WDTPW + WDTHOLD;
	
	TimerA_init();
	GPIO_init();
	Interrupt_init();

	i2c_init();

	//wait a bit for all devices to power up
	delay_ms(100);



	//main loop
	while(1)
	{
		LED_RED_TOGGLE();

		uint8_t rx[16] = {0x00};

		LightSensorReadAddress(0x0A, rx, 1);

		delay_ms(50);

		i2c_init();


	}

}



void dummy_delay(unsigned int temp)
{
	volatile unsigned int count = temp;
	while(count > 0)
		count--;
}


////////////////////////////////////////
void delay_ms(volatile int ticks)
{
	TimeDelay = ticks;

	//TimeDelay is static and gets decremented
	//in the TimerA ISR
	while (TimeDelay !=0);
}


/////////////////////////////////////////
//Function called from the TimerA ISR
void TimeDelay_Decrement(void)
{
	if (TimeDelay != 0x00)
	{
		TimeDelay--;
	}

}

//////////////////////////////////////
//Configure red led as output
//Note: green led conflicts with i2c
//
void GPIO_init(void)
{
	//setup bit 0 and 6 as output

	P1DIR |= BIT0;
	P1OUT &=~ BIT0;		//turn off

	//set up the user button bit 3
	//as input with pull up enabled
	P1DIR &=~ BIT3;		//input
	P1REN |= BIT3;		//enable pullup/down
	P1OUT |= BIT3;		//resistor set to pull up



}

/////////////////////////////////////////
//Sets up the timer And the frequency
//of the clock, since the rate of overflow
//is connected to this

void TimerA_init(void)
{

	//set the clock frequency to 16 mhz
	//use this when uart at 230400
	BCSCTL1 = CALBC1_16MHZ;
	DCOCTL = CALDCO_16MHZ;

//	BCSCTL1 = CALBC1_8MHZ;
//	DCOCTL = CALDCO_8MHZ;

	//use this with uart at 9600
//	BCSCTL1 = CALBC1_1MHZ;
//	DCOCTL = CALDCO_1MHZ;

	//Set up Timer A register TACTL
	TACTL = 0x0000;		//start from all all clear
	TACTL |= BIT9;		//use SMCLK as the source
	TACTL |= BIT7;		//use prescaler 8
	TACTL |= BIT6;
	TACTL |= BIT4;		//count up to TACCR0(set below)
	TACTL |= BIT1;		//enable the interrupt
	TACTL &=~ BIT0;		//clear all pending interrupts

	//set the countup value.  for 1ms delay,
	//the countup value depends on the clock freq.

	//1 mhz or 16
	TACCR0 = 2000;		//use 2000 for 16 mhz
//	TACCR0 = 125;		//use 125 for  1 mhz
	//	TACCR0 = 1000;		//use 1000 for 8 mhz

	//TACCTL0 - compare capture control register.
	//this has to be set up along with the timer interrupt
	//bit 4 enables the interrupts for this register
	//also called CCIE

	TACCTL0 = CCIE;		//compare, capture interrupt enable

}

///////////////////////////////////////////
void Interrupt_init(void)
{
	//enable all the interrupts
	__bis_SR_register(GIE);

	P1IE |= BIT3;		//enable button interrupt
	P1IFG &=~ BIT3;		//clear the flag

}

///////////////////////////////////////////
void LED_RED_TOGGLE(void)
{
	P1OUT ^= BIT0;
}



//////////////////////////////////////
//TimerA ISR
#pragma vector = TIMER0_A0_VECTOR
__interrupt void Timer_A(void)
{
	//clear the timer interrupt
	TACTL &=~ BIT0;

	TimeDelay_Decrement();

}


/////////////////////////////////////
//P1 ISR for the user button
#pragma vector = PORT1_VECTOR
__interrupt void Port_1(void)
{
	//clear the interrupt flag - button
	P1IFG &=~ BIT3;

	//do something....
	P1OUT ^= BIT0;
}






////////////////////////////////////////////////
//multibyte write
//generic write.  For the light sensor, the
//first element data[0] should contain the
//proper single/multi byte write command
void LightSensorWrite(uint8_t* data, uint8_t len)
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
//generic read. for the light sensor, data[0]
//should contain the correct single/multibyte read
//command.
void LightSensorRead(uint8_t* data, uint8_t len)
{
	_EINT();

	TI_USCI_I2C_receiveinit(I2C_ADDRESS,I2C_BUS_PRESCALE);   	// init receiving with USCI
	while ( TI_USCI_I2C_notready() );         		// wait for bus to be free

	//read len bytes into data
	TI_USCI_I2C_receive(len, data);
	while ( TI_USCI_I2C_notready() );         		// wait for bus to be free

}





//////////////////////////////////////////////////////////////
//write len bytes to starting address
//copies the starting addres
//send the address with no stop
//send the data as usual
//
void LightSensorWriteAddress(uint8_t address, uint8_t* data, uint8_t len)
{
	//setup for a single byte write, check if
	uint8_t writeCmdByte = 0x80;		//generic read single byte
	uint8_t counter = 0x00;
	uint8_t starting = 0x00;

	if (len > 1)				//check for a block write
		writeCmdByte = 0x90;

	//set the starting address
	starting = writeCmdByte | address;

	//enable interrupts - required to read ack/nack, etc
	_EINT();

	//setup i2c slave for writing - send the address
	//(7bits, not upshifted) and the prescale
	TI_USCI_I2C_transmitinit(I2C_ADDRESS,I2C_BUS_PRESCALE);
	while ( TI_USCI_I2C_notready() );		//wait

	//send the register address - no stop condition
	TI_USCI_I2C_transmit_nostop(1, &starting);

	//send the data - generates the stop condition when complete
	TI_USCI_I2C_transmit(len, data);
	while ( TI_USCI_I2C_notready() );

}







/////////////////////////////////////
//Read array from tsl2561 light sensor
//address is the starting address from which
//it starts reading.
//process:
//setup for a write and send the starting address
//setup for a read and read len bytes
void LightSensorReadAddress(uint8_t address, uint8_t* data, uint8_t len)
{
	//setup for a single byte write at address
	uint8_t readCmdByte = 0x80;		//generic single byte
	uint8_t readAddress = 0x00;
	uint8_t rx = 0x00;
	uint8_t counter = 0x00;

	//block
	if (len > 1)
		readCmdByte = 0x90;

	//setup for a read - address
	readAddress = readCmdByte | address;

	_EINT();

	//setup i2c slave for writing - send the address
	//(7bits, not upshifted) and the prescale
	TI_USCI_I2C_transmitinit(I2C_ADDRESS,I2C_BUS_PRESCALE);
	while ( TI_USCI_I2C_notready() );		//wait

	//send the register address - no stop condition
//	TI_USCI_I2C_transmit_nostop(1, &readAddress);
	TI_USCI_I2C_transmit(1, &readAddress);

	//switch the master to reading and read len
	//bytes into data*

	TI_USCI_I2C_receiveinit(I2C_ADDRESS,I2C_BUS_PRESCALE);   	// init receiving with USCI
	while ( TI_USCI_I2C_notready() );         		// wait for bus to be free

	//read len bytes into data
	TI_USCI_I2C_receive(len, data);


}




void vfo_writeReg(uint8_t reg, uint8_t data)
{
	uint8_t tx[2] = {reg, data};
	i2c_write(tx, 2);
}


void vfo_readReg(uint8_t reg, uint8_t* data)
{
	uint8_t result = 0x00;
	uint8_t tx = reg;

	i2c_write(&tx, 1);
	i2c_read(&result, 1);

	*data = result;
}



/////////////////////////////////////////
void i2c_init(void)
{
 	//enable interrupts - required to read ack/nack, etc
	_EINT();

	//setup i2c slave for writing - send the address
	//(7bits, not upshifted) and the prescale
	TI_USCI_I2C_transmitinit(I2C_ADDRESS,I2C_BUS_PRESCALE);

	while ( TI_USCI_I2C_notready() );		//wait

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





//////////////////////////////////////////////////////////////
//i2c Write to reg address
//Set up master as tx, send one address byte with
//no stop condition.s  Then send len bytes from
//data array.
//useful when address has to be appended with
//certain bits etc.  Ex light sensor.
//
void i2c_writeAddress(uint8_t address, uint8_t* data, uint8_t len)
{

	//enable interrupts - required to read ack/nack, etc
	_EINT();

	//setup i2c slave for writing - send the address
	//(7bits, not upshifted) and the prescale
	TI_USCI_I2C_transmitinit(I2C_ADDRESS,I2C_BUS_PRESCALE);
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
void i2c_readAddress(uint8_t address, uint8_t* data, uint8_t len)
{
	_EINT();

	//setup i2c slave for writing - send the address
	//(7bits, not upshifted) and the prescale
	TI_USCI_I2C_transmitinit(I2C_ADDRESS,I2C_BUS_PRESCALE);
	while ( TI_USCI_I2C_notready() );


	//send the register address - no stop condition
//	TI_USCI_I2C_transmit_nostop(1, &address);
	TI_USCI_I2C_transmit(1, &address);

    //wait a bit
    while ( TI_USCI_I2C_notready() );

    //setup i2c as a receiver
	TI_USCI_I2C_receiveinit(I2C_ADDRESS,I2C_BUS_PRESCALE);

    //wait a bit
	while ( TI_USCI_I2C_notready() );

	//read len bytes into data
	TI_USCI_I2C_receive(len, data);

    //wait a bit
	while ( TI_USCI_I2C_notready() );

}






