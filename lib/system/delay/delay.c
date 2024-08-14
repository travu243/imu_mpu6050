#include "delay.h"
////////////////////////////////////////////////////////////////////////////////// 	 
//If you need to use OS, just include the following header file.
#if SYSTEM_SUPPORT_OS
#include "includes.h"					//ucos ʹ��	  
#endif
//////////////////////////////////////////////////////////////////////////////////	 
//V1.2 modification instructions
//Fixed the error of calling a dead loop in the interrupt
//To prevent inaccurate delay, use do while structure!
//V1.3 modification description
//Added support for UCOSII delay.
//If ucosII is used, delay_init will automatically set the value of SYSTICK to correspond to TICKS_PER_SEC of ucos.
//delay_ms and delay_us have also been modified for ucos.
//delay_us can be used under ucos, and it is very accurate. More importantly, it does not occupy additional timers.
//delay_ms can be used as OSTimeDly under ucos. When ucos is not started, it uses delay_us to achieve accurate delay
//It can be used to initialize peripherals. After ucos is started, delay_ms chooses OSTimeDly implementation or delay_us implementation according to the length of delay.
//V1.4 modification description 20110929
//Modified the bug that the interrupt in delay_ms cannot respond when ucos is used but ucos is not started.
//V1.5 modification description 20120902
//Add ucos lock in delay_us to prevent inaccurate delay caused by ucos interrupting the execution of delay_us.
//V1.6 modification instructions 20150109
//Add OSLockNesting judgment in delay_ms.
//V1.7 modification notes 20150319
//Modify OS support mode to support any OS (not limited to UCOSII and UCOSIII, any OS can be supported in theory)
//Add: delay_osrunning/delay_ostickspersec/delay_osintnesting three macro definitions
//Add: delay_osschedlock/delay_osschedunlock/delay_ostimedly three functions
//V1.8 modification notes 20150519
//Fix 2 bugs when supporting UCOSIII:
//delay_tickspersec changed to: delay_ostickspersec
//delay_intnesting changed to: delay_osintnesting
//////////////////////////////////////////////////////////////////////////////////  

static u8  fac_us=0;							//us delay multiplier		   
static u16 fac_ms=0;							//ms delay multiplier, under ucos, represents the number of ms per beat
	
	
#if SYSTEM_SUPPORT_OS							//If SYSTEM_SUPPORT_OS is defined, it means that OS is supported (not limited to UCOS).
//When delay_us/delay_ms needs to support OS, three OS-related macro definitions and functions are needed to support it
//First, there are three macro definitions:
// delay_osrunning: used to indicate whether the OS is currently running to determine whether related functions can be used
//delay_ostickspersec: used to indicate the clock beat set by the OS, delay_init will initialize the hash according to this parameter
// delay_osintnesting: used to indicate the OS interrupt nesting level, because interrupts cannot be scheduled, delay_ms uses this parameter to determine how to run
//Then there are three functions:
// delay_osschedlock: used to lock OS task scheduling and prohibit scheduling
//delay_osschedunlock: used to unlock OS task scheduling and reopen scheduling
// delay_ostimedly: used for OS delay, which can cause task scheduling.
//This routine only supports UCOSII and UCOSIII. For other OS, please refer to the transplantation
//Support UCOSII
#ifdef 	OS_CRITICAL_METHOD						//OS_CRITICAL_METHOD������,˵��Ҫ֧��UCOSII				
#define delay_osrunning		OSRunning			//OS�Ƿ����б��,0,������;1,������
#define delay_ostickspersec	OS_TICKS_PER_SEC	//OSʱ�ӽ���,��ÿ����ȴ���
#define delay_osintnesting 	OSIntNesting		//�ж�Ƕ�׼���,���ж�Ƕ�״���
#endif

//֧��UCOSIII
#ifdef 	CPU_CFG_CRITICAL_METHOD					//CPU_CFG_CRITICAL_METHOD������,˵��Ҫ֧��UCOSIII	
#define delay_osrunning		OSRunning			//OS�Ƿ����б��,0,������;1,������
#define delay_ostickspersec	OSCfg_TickRate_Hz	//OSʱ�ӽ���,��ÿ����ȴ���
#define delay_osintnesting 	OSIntNestingCtr		//�ж�Ƕ�׼���,���ж�Ƕ�״���
#endif


//us����ʱʱ,�ر��������(��ֹ���us���ӳ�)
void delay_osschedlock(void)
{
#ifdef CPU_CFG_CRITICAL_METHOD   				//ʹ��UCOSIII
	OS_ERR err; 
	OSSchedLock(&err);							//UCOSIII�ķ�ʽ,��ֹ���ȣ���ֹ���us��ʱ
#else											//����UCOSII
	OSSchedLock();								//UCOSII�ķ�ʽ,��ֹ���ȣ���ֹ���us��ʱ
#endif
}

//us����ʱʱ,�ָ��������
void delay_osschedunlock(void)
{	
#ifdef CPU_CFG_CRITICAL_METHOD   				//ʹ��UCOSIII
	OS_ERR err; 
	OSSchedUnlock(&err);						//UCOSIII�ķ�ʽ,�ָ�����
#else											//����UCOSII
	OSSchedUnlock();							//UCOSII�ķ�ʽ,�ָ�����
#endif
}

//����OS�Դ�����ʱ������ʱ
//ticks:��ʱ�Ľ�����
void delay_ostimedly(u32 ticks)
{
#ifdef CPU_CFG_CRITICAL_METHOD
	OS_ERR err; 
	OSTimeDly(ticks,OS_OPT_TIME_PERIODIC,&err);	//UCOSIII��ʱ��������ģʽ
#else
	OSTimeDly(ticks);							//UCOSII��ʱ
#endif 
}
 
//systick�жϷ�����,ʹ��ucosʱ�õ�
void SysTick_Handler(void)
{	
	if(delay_osrunning==1)						//OS��ʼ����,��ִ�������ĵ��ȴ���
	{
		OSIntEnter();							//�����ж�
		OSTimeTick();       					//����ucos��ʱ�ӷ������               
		OSIntExit();       	 					//���������л����ж�
	}
}
#endif

			   
//��ʼ���ӳٺ���
//��ʹ��OS��ʱ��,�˺������ʼ��OS��ʱ�ӽ���
//SYSTICK��ʱ�ӹ̶�ΪHCLKʱ�ӵ�1/8
//SYSCLK:ϵͳʱ��
void delay_init()
{
#if SYSTEM_SUPPORT_OS  							//�����Ҫ֧��OS.
	u32 reload;
#endif
	SysTick_CLKSourceConfig(SysTick_CLKSource_HCLK_Div8);	//ѡ���ⲿʱ��  HCLK/8
	fac_us=SystemCoreClock/8000000;				//Ϊϵͳʱ�ӵ�1/8  
#if SYSTEM_SUPPORT_OS  							//�����Ҫ֧��OS.
	reload=SystemCoreClock/8000000;				//ÿ���ӵļ������� ��λΪK	   
	reload*=1000000/delay_ostickspersec;		//����delay_ostickspersec�趨���ʱ��
												//reloadΪ24λ�Ĵ���,���ֵ:16777216,��72M��,Լ��1.86s����	
	fac_ms=1000/delay_ostickspersec;			//����OS������ʱ�����ٵ�λ	   

	SysTick->CTRL|=SysTick_CTRL_TICKINT_Msk;   	//����SYSTICK�ж�
	SysTick->LOAD=reload; 						//ÿ1/delay_ostickspersec���ж�һ��	
	SysTick->CTRL|=SysTick_CTRL_ENABLE_Msk;   	//����SYSTICK    

#else
	fac_ms=(u16)fac_us*1000;					//��OS��,����ÿ��ms��Ҫ��systickʱ����   
#endif
}								    

#if SYSTEM_SUPPORT_OS  							//�����Ҫ֧��OS.
//��ʱnus
//nusΪҪ��ʱ��us��.		    								   
void delay_us(u32 nus)
{		
	u32 ticks;
	u32 told,tnow,tcnt=0;
	u32 reload=SysTick->LOAD;					//LOAD��ֵ	    	 
	ticks=nus*fac_us; 							//��Ҫ�Ľ�����	  		 
	tcnt=0;
	delay_osschedlock();						//��ֹOS���ȣ���ֹ���us��ʱ
	told=SysTick->VAL;        					//�ս���ʱ�ļ�����ֵ
	while(1)
	{
		tnow=SysTick->VAL;	
		if(tnow!=told)
		{	    
			if(tnow<told)tcnt+=told-tnow;		//����ע��һ��SYSTICK��һ���ݼ��ļ������Ϳ�����.
			else tcnt+=reload-tnow+told;	    
			told=tnow;
			if(tcnt>=ticks)break;				//ʱ�䳬��/����Ҫ�ӳٵ�ʱ��,���˳�.
		}  
	};
	delay_osschedunlock();						//�ָ�OS����									    
}
//��ʱnms
//nms:Ҫ��ʱ��ms��
void delay_ms(u16 nms)
{	
	if(delay_osrunning&&delay_osintnesting==0)	//���OS�Ѿ�������,���Ҳ������ж�����(�ж����治���������)	    
	{		 
		if(nms>=fac_ms)							//��ʱ��ʱ�����OS������ʱ������ 
		{ 
   			delay_ostimedly(nms/fac_ms);		//OS��ʱ
		}
		nms%=fac_ms;							//OS�Ѿ��޷��ṩ��ôС����ʱ��,������ͨ��ʽ��ʱ    
	}
	delay_us((u32)(nms*1000));					//��ͨ��ʽ��ʱ  
}
#else
//Delay nus
//nus is the number of us to be delayed.
void delay_us(u32 nus)
{
u32 temp;
SysTick->LOAD=nus*fac_us; 					//Time loading
SysTick->VAL=0x00; 							//Clear counter
SysTick->CTRL|=SysTick_CTRL_ENABLE_Msk ; 	//Start countdown
do
{
temp=SysTick->CTRL;
}while((temp&0x01)&&!(temp&(1<<16))); 		//Wait for time to arrive
SysTick->CTRL&=~SysTick_CTRL_ENABLE_Msk; 	//Turn off counter
SysTick->VAL =0X00; 						//Clear counter
}
//Delay nms
//Note the range of nms
//SysTick->LOAD is a 24-bit register, so the maximum delay is:
//nms<=0xffffff*8*1000/SYSCLK
//SYSCLK unit is Hz, nms unit is ms
//For 72M conditions, nms<=1864
void delay_ms(u16 nms)
{	 		  	  
	u32 temp;		   
	SysTick->LOAD=(u32)nms*fac_ms;				//Time loading (SysTick->LOAD is 24bit)
	SysTick->VAL =0x00;							//Clear counter
	SysTick->CTRL|=SysTick_CTRL_ENABLE_Msk ;	//Start countdown 
	do
	{
		temp=SysTick->CTRL;
	}while((temp&0x01)&&!(temp&(1<<16)));		//Waiting time to arrive
	SysTick->CTRL&=~SysTick_CTRL_ENABLE_Msk;	//Close Counter
	SysTick->VAL =0X00;       					//Clear counter	  	    
} 
#endif 








































