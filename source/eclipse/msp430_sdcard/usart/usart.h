#ifndef __USART__H
#define __USART__H




//prototypes for uart
void usart_init(void);				//setup for SPI
void usart_writeByte(uint8_t value);
void usart_writeString(char * buffer);
void usart_writeStringLength(char *buffer, uint8_t size);
void usart_processCommand(char *buffer, uint8_t len);




#endif

