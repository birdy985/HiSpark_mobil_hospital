#include "sys.h"
#include "usart.h"	  
////////////////////////////////////////////////////////////////////////////////// 	 
//如果使用ucos,则包括下面的头文件即可.
#if SYSTEM_SUPPORT_OS
#include "includes.h"					//ucos 使用	  
#endif

//V1.3修改说明 
//支持适应不同频率下的串口波特率设置.
//加入了对printf的支持
//增加了串口接收命令功能.
//修正了printf第一个字符丢失的bug
//V1.4修改说明
//1,修改串口初始化IO的bug
//2,修改了USART_RX_STA,使得串口最大接收字节数为2的14次方
//3,增加了USART_REC_LEN,用于定义串口最大允许接收的字节数(不大于2的14次方)
//4,修改了EN_USART3_RX的使能方式
//V1.5修改说明
//1,增加了对UCOSII的支持
////////////////////////////////////////////////////////////////////////////////// 	  
 

//////////////////////////////////////////////////////////////////
//加入以下代码,支持printf函数,而不需要选择use MicroLIB	  
#if 1
#pragma import(__use_no_semihosting)             
//标准库需要的支持函数                 
struct __FILE 
{ 
	int handle; 

}; 

FILE __stdout;       
//定义_sys_exit()以避免使用半主机模式    
_sys_exit(int x) 
{ 
	x = x; 
} 
//重定义fputc函数 
int fputc(int ch, FILE *f)
{      
	USART_SendData(USART2, (uint8_t) ch);
	while(USART_GetFlagStatus(USART2,USART_FLAG_TC)==RESET);
    //USART_SendData(USART2,(uint8_t)ch);    
	return ch;
}
#endif 

/*使用microLib的方法*/
 /* 
int fputc(int ch, FILE *f)
{
	USART_SendData(USART1, (uint8_t) ch);

	while (USART_GetFlagStatus(USART1, USART_FLAG_TC) == RESET) {}	
   
    return ch;
}
int GetKey (void)  { 

    while (!(USART1->SR & USART_FLAG_RXNE));

    return ((int)(USART1->DR & 0x1FF));
}
*/
 
u16 USART1_RX_LENGTH;
u8 USART1_RX_BUFF[200];

#if EN_USART1_RX   //如果使能了接收
//串口1中断服务程序
//注意,读取USARTx->SR能避免莫名其妙的错误   	
u8 USART_RX_BUF[USART_REC_LEN];     //接收缓冲,最大USART_REC_LEN个字节.
//接收状态
//bit15，	接收完成标志
//bit14，	接收到0x0d
//bit13~0，	接收到的有效字节数目
u16 USART_RX_STA=0;       //接收状态标记	  

void uart_init(u32 bound){
    //GPIO端口设置
   GPIO_InitTypeDef GPIO_InitStructure;				   //IO口配置结构体
	USART_InitTypeDef USART_InitStructure;				   //串口配置结构体
	NVIC_InitTypeDef NVIC_InitStructure;				   //中断配置结构体
	 
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_USART1|RCC_APB2Periph_GPIOA, ENABLE);	//使能USART1，GPIOA时钟
 	USART_DeInit(USART1);          //复位串口1  在开启外设前 最好都要复位一下外设
	 //USART1_TX   PA.9
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_9; //PA.9
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;	//复用推挽输出	 PA.9选择复用状态才能进入串口模式
    GPIO_Init(GPIOA, &GPIO_InitStructure); //初始化PA9
   
    //USART1_RX	  PA.10
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_10;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN_FLOATING;//浮空输入
    GPIO_Init(GPIOA, &GPIO_InitStructure);  //初始化PA10




   //USART 初始化设置

	USART_InitStructure.USART_BaudRate = bound;                //波特率设置 一般设置为9600;
	USART_InitStructure.USART_WordLength = USART_WordLength_8b;//字长为8位数据格式
	USART_InitStructure.USART_StopBits = USART_StopBits_1;     //一个停止位
	USART_InitStructure.USART_Parity = USART_Parity_No;        //无奇偶校验位
	USART_InitStructure.USART_HardwareFlowControl = USART_HardwareFlowControl_None;//无硬件数据流控制
	USART_InitStructure.USART_Mode = USART_Mode_Rx | USART_Mode_Tx;	   //收发模式   这里可以配置仅发 或仅收
		
    USART_Init(USART1, &USART_InitStructure); //初始化串口
		
		

#if EN_USART1_RX		  //如果使能了接收  
   //Usart1 NVIC 配置
    NVIC_InitStructure.NVIC_IRQChannel = USART1_IRQn;
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority=1 ;//抢占优先级3
	NVIC_InitStructure.NVIC_IRQChannelSubPriority = 1;		//子优先级3
	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;			//IRQ通道使能
	NVIC_Init(&NVIC_InitStructure);	//根据指定的参数初始化VIC寄存器
   
	 USART_ITConfig(USART1, USART_IT_RXNE, ENABLE);//开启中断	 接收到数据进入中断
	 

#endif

    USART_Cmd(USART1, ENABLE);                    //使能串口 

}




void Uart1_SendStr(char* SendBuff)
{
	while(*SendBuff)
	{
		while((USART1->SR & 0x40) == 0);
		USART1->DR = (u8) *SendBuff;
		SendBuff ++;
	}
}

void USART1_IRQHandler(void)
{
	if(USART_GetITStatus(USART1,USART_IT_RXNE) != RESET)
	{
		USART1_RX_BUFF[USART1_RX_LENGTH ++] = USART1->DR;
	}
}



void USART2_Configuration(void)
{
	
	USART_InitTypeDef USART_InitStructure;
	GPIO_InitTypeDef GPIO_InitStructure;
	NVIC_InitTypeDef NVIC_InitStructure;
	
	
	RCC_APB1PeriphClockCmd(RCC_APB1Periph_USART2, ENABLE);
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA,ENABLE);
	/* Configure USART2 Rx (PA.3) as input floating */
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_3;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN_FLOATING;		  //
	GPIO_Init(GPIOA, &GPIO_InitStructure);
	
	/* Configure USART2 Tx (PA.2) as alternate function push-pull */
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_2;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;			  //
	GPIO_Init(GPIOA, &GPIO_InitStructure);
	
	USART_InitStructure.USART_BaudRate = 9600;						//
	USART_InitStructure.USART_WordLength = USART_WordLength_8b;		//
	USART_InitStructure.USART_StopBits = USART_StopBits_1;			//
	USART_InitStructure.USART_Parity = USART_Parity_No;				//
	USART_InitStructure.USART_HardwareFlowControl = USART_HardwareFlowControl_None;   //
	USART_InitStructure.USART_Mode = USART_Mode_Rx | USART_Mode_Tx;					//
	/* Configure USART2 */
	USART_Init(USART2, &USART_InitStructure);							//
	/* Enable USART2 Receive and Transmit interrupts */
	USART_ITConfig(USART2, USART_IT_RXNE, DISABLE);                    //
	//USART_ITConfig(USART2, USART_IT_IDLE, ENABLE);//
	/* Enable the USART2 */
	USART_Cmd(USART2, ENABLE);	                  //
	
	
	NVIC_PriorityGroupConfig(NVIC_PriorityGroup_2);	
	NVIC_InitStructure.NVIC_IRQChannel = USART2_IRQn;	   	//
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 1;		   //
	NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0;			   //
	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;		//
	NVIC_Init(&NVIC_InitStructure);
}



#endif	

