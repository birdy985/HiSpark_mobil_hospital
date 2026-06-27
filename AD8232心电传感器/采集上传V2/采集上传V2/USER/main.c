#include "sys.h"
#include "delay.h"
#include "usart.h" 
#include "led.h" 		 	   
#include "adc.h"

 int main(void)
 { 
	 u16 value;
	delay_init();
	LED_Init();
	 delay_ms(500);
	NVIC_PriorityGroupConfig(NVIC_PriorityGroup_2);
	USART2_Configuration();	//初始化串口2，波特率为9600
	 delay_ms(500); //延时10ms，修改延时时间可以改变采样速度
	Adc_Init();

	 delay_ms(500);
	 LED0 = 0;
//	 printf("ok");
	 while(1)
	 { 
			value = Get_Adc(0);//读取ADC
			printf("%d\r\n",value);//发送数据，
			delay_ms(10); //延时10ms，修改延时时间可以改变采样速度
	 }
}
 

