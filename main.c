#include "led.h"
#include "delay.h"
#include "sys.h"
#include "usart.h"
#include "mpu6050.h"  
#include "inv_mpu.h"
#include "inv_mpu_dmp_motion_driver.h" 


void send_data(int8_t ch){
while((USART1->SR & USART_SR_TXE)==0){}; 
		USART1->DR = ch;      
}

int main(void)
{

	float pitch,roll,yaw;
	int16_t data;

	uint8_t high_byte;
	uint8_t low_byte;

	NVIC_PriorityGroupConfig(NVIC_PriorityGroup_2);
	uart_init(115200); // gpio A9 TX, gpio A10 RX 
	delay_init();
	LED_Init();
	MPU_Init();				// gpio B10, B11

	while(mpu_dmp_init())
	{
	delay_ms(20);
	}

	while(1){
		delay_ms(5);
		if(mpu_dmp_get_data(&pitch,&roll,&yaw)==0)
		{
			LED=~LED;
		}
		delay_ms(1);
//		printf("%f,%f,%f\r\n",pitch,roll,yaw);
		
		data=yaw*10;
		high_byte = (data >> 8) & 0xFF;
		low_byte = data & 0xFF;
		
		send_data(high_byte);
		send_data(low_byte);
		
	}
}
 

