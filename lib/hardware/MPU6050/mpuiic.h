#ifndef __MPUIIC_H
#define __MPUIIC_H
#include "sys.h"

//IO direction setting
#define MPU_SDA_IN()  {GPIOB->CRH&=0XFFFF0FFF;GPIOB->CRH|=8<<12;}
#define MPU_SDA_OUT() {GPIOB->CRH&=0XFFFF0FFF;GPIOB->CRH|=3<<12;}

//IO operation function	 
#define MPU_IIC_SCL    PBout(10) 		//SCL
#define MPU_IIC_SDA    PBout(11) 		//SDA	 
#define MPU_READ_SDA   PBin(11) 		//Input SDA

//IIC all operation functions
void MPU_IIC_Delay(void); 				//MPU IIC delay function
void MPU_IIC_Init(void); 				//Initialize IIC IO port
void MPU_IIC_Start(void);			 	//Send IIC start signal
void MPU_IIC_Stop(void); 				//Send IIC stop signal
void MPU_IIC_Send_Byte(u8 txd); 		//IIC sends a byte
u8 MPU_IIC_Read_Byte(unsigned char ack);//IIC reads a byte
u8 MPU_IIC_Wait_Ack(void); 				//IIC waits for ACK signal
void MPU_IIC_Ack(void); 				//IIC sends ACK signal
void MPU_IIC_NAck(void); 				//IIC does not send ACK signal

void IMPU_IC_Write_One_Byte(u8 daddr,u8 addr,u8 data);
u8 MPU_IIC_Read_One_Byte(u8 daddr,u8 addr);	  
#endif
















