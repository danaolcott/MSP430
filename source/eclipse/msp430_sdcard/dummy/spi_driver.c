///////////////////////////////////////////////////////
//SPI Driver file for SPI3
//Configure at Mode 0, idle clock low, data on the 
//leading edge
//
#include "main.h"
#include "spi_driver.h"

//////////////////////////////////////////////////
//SPI_init
//Configure SPI3 on PD0 to PD3
//Configure CS Pin PD1 as normal GPIO
//
void SPI_init()
{
    //enable the clocks
    SysCtlPeripheralEnable(SYSCTL_PERIPH_SSI3);
    SysCtlDelay(3);
    SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOD);
    SysCtlDelay(3);

    //pin configure
    GPIOPinConfigure(GPIO_PD0_SSI3CLK);
//    GPIOPinConfigure(GPIO_PD1_SSI3FSS);
    GPIOPinConfigure(GPIO_PD2_SSI3RX);
    GPIOPinConfigure(GPIO_PD3_SSI3TX);
        
    GPIOPinTypeSSI(GPIO_PORTD_BASE, GPIO_PIN_0);
//    GPIOPinTypeSSI(GPIO_PORTD_BASE, GPIO_PIN_1);
    GPIOPinTypeSSI(GPIO_PORTD_BASE, GPIO_PIN_2);
    GPIOPinTypeSSI(GPIO_PORTD_BASE, GPIO_PIN_3);

    //Chip selectd pin - PD1 as normal GPIO
    GPIOPinTypeGPIOOutput(GPIO_PORTD_BASE, GPIO_PIN_1);
    //init high
    GPIOPinWrite(GPIO_PORTD_BASE, GPIO_PIN_1, GPIO_PIN_1);

    //clear the End of Transfer flag???
    SSIIntClear(SSI3_BASE,SSI_TXEOT);
    SysCtlDelay(3);

    //Configure SPI 
    //12mhz clock, 8bits, idle clock low, data read on leading edge
    //master clock, mode0, master, 12mhz clk, 8bit
    //last param is the number of bits, range from... 1-8?
    SSIConfigSetExpClk(SSI3_BASE, 80000000, SSI_FRF_MOTO_MODE_0,SSI_MODE_MASTER, 8000000, 8);
    SysCtlDelay(3);

    //enable SPI3
    SSIEnable(SSI3_BASE);   
}   

void SPI_WriteData(uint8_t data)
{
    uint32_t dummy;
    GPIOPinWrite(GPIO_PORTD_BASE, GPIO_PIN_1, ~GPIO_PIN_1);
    SSIDataPut(SSI3_BASE, data);
    while(SSIBusy(SSI3_BASE));
    SSIDataGet(SSI3_BASE, &dummy);
    GPIOPinWrite(GPIO_PORTD_BASE, GPIO_PIN_1, GPIO_PIN_1);
}

void SPI_WriteReg(uint8_t reg, uint8_t data)
{
    uint32_t dummy;
    GPIOPinWrite(GPIO_PORTD_BASE, GPIO_PIN_1, ~GPIO_PIN_1);  

    SSIDataPut(SSI3_BASE, reg);
    while(SSIBusy(SSI3_BASE));
    SSIDataGet(SSI3_BASE, &dummy);  //clear fifo

    SSIDataPut(SSI3_BASE, data);
    while(SSIBusy(SSI3_BASE));
    SSIDataGet(SSI3_BASE, &dummy);  //clear fifo

    GPIOPinWrite(GPIO_PORTD_BASE, GPIO_PIN_1, GPIO_PIN_1);    
}

void SPI_WriteArray(uint8_t* data, uint32_t length)
{
    uint32_t dummy;
    //chip select
    GPIOPinWrite(GPIO_PORTD_BASE, GPIO_PIN_1, ~GPIO_PIN_1);

    uint32_t i;
    for(i = 0 ; i < length ; i++)
    {        
        SSIDataPut(SSI3_BASE, data[i]);
        while(SSIBusy(SSI3_BASE));
        SSIDataGet(SSI3_BASE, &dummy);  //clear fifo
    }

    //chip select
    GPIOPinWrite(GPIO_PORTD_BASE, GPIO_PIN_1, GPIO_PIN_1);
}

/////////////////////////////////////////
//
uint8_t SPI_ReadData(uint8_t reg)
{
    uint32_t data = 0x00;

    //chip select
    GPIOPinWrite(GPIO_PORTD_BASE, GPIO_PIN_1, ~GPIO_PIN_1);  

    //send reg
    SSIDataPut(SSI3_BASE, reg);
    while(SSIBusy(SSI3_BASE));    
    SSIDataGet(SSI3_BASE, &data);      //clear incoming fifo

    //send a dummy byte to generate clock
    SSIDataPut(SSI3_BASE, 0xFF);
    while(SSIBusy(SSI3_BASE));    
    SSIDataGet(SSI3_BASE, &data);     //read the result

    //deselect
    GPIOPinWrite(GPIO_PORTD_BASE, GPIO_PIN_1, GPIO_PIN_1);  

    //return the 8 bit value
    return ((uint8_t)((data & 0xFF)));
}


//////////////////////////////////////////
//Set the Speed of the SPI clock
//this is for running the sd card initially
//at 400khz and flipping to 12mhz for the 
//data and lcd
//Assumes that the spi has already been
//configured.
void SPI_SetSpeedHz(uint32_t speed)
{
    //disable the spi3??
    SSIDisable(SSI3_BASE);
    SysCtlDelay(3);

    //clear the End of Transfer flag???
    SSIIntClear(SSI3_BASE,SSI_TXEOT);
    SysCtlDelay(3);

    //configure SPI at the speed given   
    //8bits, idle clock low, data read on leading edge
    //master clock, mode0, master, 12mhz clk, 8bit
    //last param is the number of bits, range from... 1-8?
    SSIConfigSetExpClk(SSI3_BASE, 80000000, SSI_FRF_MOTO_MODE_0,SSI_MODE_MASTER, speed, 8);
    SysCtlDelay(3);

    //enable SPI3
    SSIEnable(SSI3_BASE);  
}

