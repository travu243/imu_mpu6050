#include "delay.h"
////////////////////////////////////////////////////////////////////////////////// 	 
//If you need to use OS, just include the following header file.
#if SYSTEM_SUPPORT_OS
#include "includes.h"					//ucos 使用	  
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
#ifdef 	OS_CRITICAL_METHOD						//OS_CRITICAL_METHOD定义了,说明要支持UCOSII				
#define delay_osrunning		OSRunning			//OS是否运行标记,0,不运行;1,在运行
#define delay_ostickspersec	OS_TICKS_PER_SEC	//OS时钟节拍,即每秒调度次数
#define delay_osintnesting 	OSIntNesting		//中断嵌套级别,即中断嵌套次数
#endif

//支持UCOSIII
#ifdef 	CPU_CFG_CRITICAL_METHOD					//CPU_CFG_CRITICAL_METHOD定义了,说明要支持UCOSIII	
#define delay_osrunning		OSRunning			//OS是否运行标记,0,不运行;1,在运行
#define delay_ostickspersec	OSCfg_TickRate_Hz	//OS时钟节拍,即每秒调度次数
#define delay_osintnesting 	OSIntNestingCtr		//中断嵌套级别,即中断嵌套次数
#endif


//us级延时时,关闭任务调度(防止打断us级延迟)
void delay_osschedlock(void)
{
#ifdef CPU_CFG_CRITICAL_METHOD   				//使用UCOSIII
	OS_ERR err; 
	OSSchedLock(&err);							//UCOSIII的方式,禁止调度，防止打断us延时
#else											//否则UCOSII
	OSSchedLock();								//UCOSII的方式,禁止调度，防止打断us延时
#endif
}

//us级延时时,恢复任务调度
void delay_osschedunlock(void)
{	
#ifdef CPU_CFG_CRITICAL_METHOD   				//使用UCOSIII
	OS_ERR err; 
	OSSchedUnlock(&err);						//UCOSIII的方式,恢复调度
#else											//否则UCOSII
	OSSchedUnlock();							//UCOSII的方式,恢复调度
#endif
}

//调用OS自带的延时函数延时
//ticks:延时的节拍数
void delay_ostimedly(u32 ticks)
{
#ifdef CPU_CFG_CRITICAL_METHOD
	OS_ERR err; 
	OSTimeDly(ticks,OS_OPT_TIME_PERIODIC,&err);	//UCOSIII延时采用周期模式
#else
	OSTimeDly(ticks);							//UCOSII延时
#endif 
}
 
//systick中断服务函数,使用ucos时用到
void SysTick_Handler(void)
{	
	if(delay_osrunning==1)						//OS开始跑了,才执行正常的调度处理
	{
		OSIntEnter();							//进入中断
		OSTimeTick();       					//调用ucos的时钟服务程序               
		OSIntExit();       	 					//触发任务切换软中断
	}
}
#endif

			   
//初始化延迟函数
//当使用OS的时候,此函数会初始化OS的时钟节拍
//SYSTICK的时钟固定为HCLK时钟的1/8
//SYSCLK:系统时钟
void delay_init()
{
#if SYSTEM_SUPPORT_OS  							//如果需要支持OS.
	u32 reload;
#endif
	SysTick_CLKSourceConfig(SysTick_CLKSource_HCLK_Div8);	//选择外部时钟  HCLK/8
	fac_us=SystemCoreClock/8000000;				//为系统时钟的1/8  
#if SYSTEM_SUPPORT_OS  							//如果需要支持OS.
	reload=SystemCoreClock/8000000;				//每秒钟的计数次数 单位为K	   
	reload*=1000000/delay_ostickspersec;		//根据delay_ostickspersec设定溢出时间
												//reload为24位寄存器,最大值:16777216,在72M下,约合1.86s左右	
	fac_ms=1000/delay_ostickspersec;			//代表OS可以延时的最少单位	   

	SysTick->CTRL|=SysTick_CTRL_TICKINT_Msk;   	//开启SYSTICK中断
	SysTick->LOAD=reload; 						//每1/delay_ostickspersec秒中断一次	
	SysTick->CTRL|=SysTick_CTRL_ENABLE_Msk;   	//开启SYSTICK    

#else
	fac_ms=(u16)fac_us*1000;					//非OS下,代表每个ms需要的systick时钟数   
#endif
}								    

#if SYSTEM_SUPPORT_OS  							//如果需要支持OS.
//延时nus
//nus为要延时的us数.		    								   
void delay_us(u32 nus)
{		
	u32 ticks;
	u32 told,tnow,tcnt=0;
	u32 reload=SysTick->LOAD;					//LOAD的值	    	 
	ticks=nus*fac_us; 							//需要的节拍数	  		 
	tcnt=0;
	delay_osschedlock();						//阻止OS调度，防止打断us延时
	told=SysTick->VAL;        					//刚进入时的计数器值
	while(1)
	{
		tnow=SysTick->VAL;	
		if(tnow!=told)
		{	    
			if(tnow<told)tcnt+=told-tnow;		//这里注意一下SYSTICK是一个递减的计数器就可以了.
			else tcnt+=reload-tnow+told;	    
			told=tnow;
			if(tcnt>=ticks)break;				//时间超过/等于要延迟的时间,则退出.
		}  
	};
	delay_osschedunlock();						//恢复OS调度									    
}
//延时nms
//nms:要延时的ms数
void delay_ms(u16 nms)
{	
	if(delay_osrunning&&delay_osintnesting==0)	//如果OS已经在跑了,并且不是在中断里面(中断里面不能任务调度)	    
	{		 
		if(nms>=fac_ms)							//延时的时间大于OS的最少时间周期 
		{ 
   			delay_ostimedly(nms/fac_ms);		//OS延时
		}
		nms%=fac_ms;							//OS已经无法提供这么小的延时了,采用普通方式延时    
	}
	delay_us((u32)(nms*1000));					//普通方式延时  
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








































