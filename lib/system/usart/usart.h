#ifndef __USART_H
#define __USART_H
#include "stdio.h"	
#include "sys.h" 

#define USART_REC_LEN  			200  	//Define the maximum number of bytes to receive: 200
#define EN_USART1_RX 			1		//Enable (1)/disable (0) serial port 1 reception
	  	
extern u8  USART_RX_BUF[USART_REC_LEN]; //Receive buffer, maximum USART_REC_LEN bytes. The last byte is a line break character
extern u16 USART_RX_STA;         		//Receive status flag
//If you want the serial port to interrupt reception, please do not comment the following macro definition
void uart_init(u32 bound);
#endif


