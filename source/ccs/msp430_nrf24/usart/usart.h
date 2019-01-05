#ifndef __USART__H
#define __USART__H




//prototypes for uart
void usart_init(void);				//setup for SPI
void usart_writeByte(uint8_t value);
void usart_writeString(uint8_t* buffer);
void usart_writeStringLength(uint8_t* buffer, uint8_t size);
void usart_processCommand(uint8_t* buffer, uint8_t len);




#endif

