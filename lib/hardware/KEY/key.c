#include "stm32f10x.h"
#include "key.h"
#include "sys.h" 
#include "delay.h"

//Button initialization function
void KEY_Init(void) //IO initialization
{
	GPIO_InitTypeDef GPIO_InitStructure;
	
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA|RCC_APB2Periph_GPIOE,ENABLE);//Enable PORTA, PORTE clock
	
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_2|GPIO_Pin_3|GPIO_Pin_4;//KEY0-KEY2
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IPU; //Set to pull-up input
	GPIO_Init(GPIOE, &GPIO_InitStructure);//Initialize GPIOE2,3,4
	
	//Initialize WK_UP-->GPIOA.0 pull-down input
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_0;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IPD; //PA0 is set to input, pull-down by default
	GPIO_Init(GPIOA, &GPIO_InitStructure);//Initialize GPIOA.0

}
//Key processing function
//Return key value
//mode: 0, continuous pressing is not supported; 1, continuous pressing is supported;
//0, no key is pressed
//1, KEY0 is pressed
//2, KEY1 is pressed
//3, KEY2 is pressed
//4, KEY3 is pressed WK_UP
//Note that this function has a response priority, KEY0>KEY1>KEY2>KEY3!!
u8 KEY_Scan(u8 mode)
{	 
	static u8 key_up=1; //Key press and release flag
	if(mode)key_up=1; //Support continuous press
	if(key_up&&(KEY0==0||KEY1==0||KEY2==0||WK_UP==1))
	{
		delay_ms(10);//Debounce
		key_up=0;
		if(KEY0==0)return KEY0_PRES;
		else if(KEY1==0)return KEY1_PRES;
		else if(KEY2==0)return KEY2_PRES;
		else if(WK_UP==1)return WKUP_PRES;
	}else if(KEY0==1&&KEY1==1&&KEY2==1&&WK_UP==0)key_up=1; 	    
 	return 0;//No button pressed
}
