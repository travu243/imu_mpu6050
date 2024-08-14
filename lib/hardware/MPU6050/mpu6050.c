#include "mpu6050.h"
#include "sys.h"
#include "delay.h"
#include "usart.h"   

//Initialize MPU6050
//Return value: 0, success
//Others, error code
u8 MPU_Init(void)
{ 
	u8 res;
  GPIO_InitTypeDef  GPIO_InitStructure;
	
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_AFIO,ENABLE);//Enable AFIO clock
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA,ENABLE);//Enable peripheral IO PORTA clock first
	
  GPIO_InitStructure.GPIO_Pin = GPIO_Pin_15;	 //Port configuration
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP; 		 //Push-pull output
  GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;		 //IO port speed is 50MHz
  GPIO_Init(GPIOA, &GPIO_InitStructure);					 //Initialize GPIOA according to the set parameters

	GPIO_PinRemapConfig(GPIO_Remap_SWJ_JTAGDisable,ENABLE);//Disable JTAG so that PA15 can be used as ordinary IO, otherwise PA15 cannot be used as ordinary IO!!!
	
	MPU_AD0_CTRL=0;			//Control the AD0 pin of MPU6050 to low level, the slave address is: 0X68
	
	MPU_IIC_Init();//Initialize IIC bus
	MPU_Write_Byte(MPU_PWR_MGMT1_REG,0X80);	//Reset MPU6050
  delay_ms(100);
	MPU_Write_Byte(MPU_PWR_MGMT1_REG,0X00);	//Wake up MPU6050
	MPU_Set_Gyro_Fsr(3);					//Gyroscope sensor, ±2000dps
	MPU_Set_Accel_Fsr(0);					//Acceleration sensor, ±2g
	MPU_Set_Rate(50);						//Set sampling rate to 50Hz
	MPU_Write_Byte(MPU_INT_EN_REG,0X00);	//Turn off all interrupts
	MPU_Write_Byte(MPU_USER_CTRL_REG,0X00);//I2C master mode off
	MPU_Write_Byte(MPU_FIFO_EN_REG,0X00);	//Turn off FIFO
	MPU_Write_Byte(MPU_INTBP_CFG_REG,0X80);	//INT pin low level is valid
	res=MPU_Read_Byte(MPU_DEVICE_ID_REG);
	if(res==MPU_ADDR)   //Device ID is correct
	{
		MPU_Write_Byte(MPU_PWR_MGMT1_REG,0X01);	//Set CLKSEL, PLL X axis as reference
		MPU_Write_Byte(MPU_PWR_MGMT2_REG,0X00);	//Both accelerometer and gyroscope work
		MPU_Set_Rate(50);						//Set sampling rate to 50Hz
 	}else return 1;
	return 0;
}
//Set the full scale range of the MPU6050 gyroscope sensor
//fsr:0,±250dps;1,±500dps;2,±1000dps;3,±2000dps
//Return value: 0, setting successful
// Others, setting failed
u8 MPU_Set_Gyro_Fsr(u8 fsr)
{
	return MPU_Write_Byte(MPU_GYRO_CFG_REG,fsr<<3);//Set the gyroscope full-scale range 
}
//Set the full scale range of the MPU6050 accelerometer
//fsr:0,±2g;1,±4g;2,±8g;3,±16g
//Return value: 0, setting successful
//Others, setting failed
u8 MPU_Set_Accel_Fsr(u8 fsr)
{
	return MPU_Write_Byte(MPU_ACCEL_CFG_REG,fsr<<3);//Set the full scale range of the acceleration sensor
}
//Set the digital low-pass filter of MPU6050
//lpf: digital low-pass filter frequency (Hz)
//Return value: 0, setting successful
// Others, setting failed
u8 MPU_Set_LPF(u16 lpf)
{
	u8 data=0;
	if(lpf>=188)data=1;
	else if(lpf>=98)data=2;
	else if(lpf>=42)data=3;
	else if(lpf>=20)data=4;
	else if(lpf>=10)data=5;
	else data=6;
	return MPU_Write_Byte(MPU_CFG_REG,data);//Set digital low-pass filter
}
//Set the sampling rate of MPU6050 (assuming Fs=1KHz)
//rate:4~1000(Hz)
//Return value: 0, setting successful
// Other, setting failed
u8 MPU_Set_Rate(u16 rate)
{
	u8 data;
	if(rate>1000)rate=1000;
	if(rate<4)rate=4;
	data=1000/rate-1;
	data=MPU_Write_Byte(MPU_SAMPLE_RATE_REG,data); //Set digital low-pass filter
	return MPU_Set_LPF(rate/2); //Automatically set LPF to half the sampling rate
}
//Get temperature value
//Return value: temperature value (expanded 100 times)
short MPU_Get_Temperature(void)
{
	u8 buf[2];
	short raw;
	float temp;
	MPU_Read_Len(MPU_ADDR,MPU_TEMP_OUTH_REG,2,buf);
	raw=((u16)buf[0]<<8)|buf[1];
	temp=36.53+((double)raw)/340;
	return temp*100;
}
//Get gyroscope value (raw value)
//gx,gy,gz: gyroscope x,y,z axis raw readings (signed)
//Return value: 0, success
// Others, error code
u8 MPU_Get_Gyroscope(short *gx,short *gy,short *gz)
{
    u8 buf[6],res;  
	res=MPU_Read_Len(MPU_ADDR,MPU_GYRO_XOUTH_REG,6,buf);
	if(res==0)
	{
		*gx=((u16)buf[0]<<8)|buf[1];  
		*gy=((u16)buf[2]<<8)|buf[3];  
		*gz=((u16)buf[4]<<8)|buf[5];
	} 	
    return res;
}



u8 MPU_Get_Accelerometer(short *ax,short *ay,short *az)
{
    u8 buf[6],res;  
	res=MPU_Read_Len(MPU_ADDR,MPU_ACCEL_XOUTH_REG,6,buf);
	if(res==0)
	{
		*ax=((u16)buf[0]<<8)|buf[1];  
		*ay=((u16)buf[2]<<8)|buf[3];  
		*az=((u16)buf[4]<<8)|buf[5];
	} 	
    return res;;
}




u8 MPU_Write_Len(u8 addr,u8 reg,u8 len,u8 *buf)
{
	u8 i; 
    MPU_IIC_Start(); 
	MPU_IIC_Send_Byte((addr<<1)|0);
	if(MPU_IIC_Wait_Ack())	
	{
		MPU_IIC_Stop();		 
		return 1;		
	}
    MPU_IIC_Send_Byte(reg);	
    MPU_IIC_Wait_Ack();
	for(i=0;i<len;i++)
	{
		MPU_IIC_Send_Byte(buf[i]);	
		if(MPU_IIC_Wait_Ack())	
		{
			MPU_IIC_Stop();	 
			return 1;		 
		}		
	}    
    MPU_IIC_Stop();	 
	return 0;	
} 




u8 MPU_Read_Len(u8 addr,u8 reg,u8 len,u8 *buf)
{ 
 	MPU_IIC_Start(); 
	MPU_IIC_Send_Byte((addr<<1)|0);
	if(MPU_IIC_Wait_Ack())
	{
		MPU_IIC_Stop();		 
		return 1;		
	}
    MPU_IIC_Send_Byte(reg);
    MPU_IIC_Wait_Ack();		
    MPU_IIC_Start();
	MPU_IIC_Send_Byte((addr<<1)|1);
    MPU_IIC_Wait_Ack();	
	while(len)
	{
		if(len==1)*buf=MPU_IIC_Read_Byte(0);
		else *buf=MPU_IIC_Read_Byte(1);	
		len--;
		buf++; 
	}    
    MPU_IIC_Stop();	
	return 0;	
}





u8 MPU_Write_Byte(u8 reg,u8 data) 				 
{ 
    MPU_IIC_Start(); 
	MPU_IIC_Send_Byte((MPU_ADDR<<1)|0);
	if(MPU_IIC_Wait_Ack())
	{
		MPU_IIC_Stop();		 
		return 1;		
	}
    MPU_IIC_Send_Byte(reg);	
    MPU_IIC_Wait_Ack();
	MPU_IIC_Send_Byte(data);
	if(MPU_IIC_Wait_Ack())	
	{
		MPU_IIC_Stop();	 
		return 1;		 
	}		 
    MPU_IIC_Stop();	 
	return 0;
}




u8 MPU_Read_Byte(u8 reg)
{
	u8 res;
    MPU_IIC_Start(); 
	MPU_IIC_Send_Byte((MPU_ADDR<<1)|0);
	MPU_IIC_Wait_Ack();		
    MPU_IIC_Send_Byte(reg);
    MPU_IIC_Wait_Ack();
    MPU_IIC_Start();
	MPU_IIC_Send_Byte((MPU_ADDR<<1)|1);
    MPU_IIC_Wait_Ack();	
	res=MPU_IIC_Read_Byte(0);
    MPU_IIC_Stop();
	return res;		
}


