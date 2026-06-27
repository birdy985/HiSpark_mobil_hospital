#include "bc20.h"
#include "string.h"

char *Receive_Data;		//
extern u8 USART1_RX_BUFF[200];		//	
extern u16 USART1_RX_LENGTH;			//
BC20 BC20_Status;

void PowerKey_Init(void)
{
	GPIO_InitTypeDef  GPIO_InitStructure;
 	
 RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB, ENABLE);	 //使能PA,PD端口时钟
	
 GPIO_InitStructure.GPIO_Pin = GPIO_Pin_13;				 //LED0-->PA.8 端口配置
 GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP; 		 //推挽输出
 GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;		 //IO口速度为50MHz
 GPIO_Init(GPIOB, &GPIO_InitStructure);					 //根据设定参数初始化GPIOA.8
// GPIO_SetBits(GPIOB,GPIO_Pin_13);						 //PA.8 输出高

// GPIO_InitStructure.GPIO_Pin = GPIO_Pin_2;	    		 //LED1-->PD.2 端口配置, 推挽输出
// GPIO_Init(GPIOD, &GPIO_InitStructure);	  				 //推挽输出 ，IO口速度为50MHz
 GPIO_ResetBits(GPIOB,GPIO_Pin_13); 						 //PB.13 输出高 
}

void ResetKey_Init(void)
{
	GPIO_InitTypeDef  GPIO_InitStructure;
 	
 RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB, ENABLE);	 //使能PA,PD端口时钟
	
 GPIO_InitStructure.GPIO_Pin = GPIO_Pin_14;				 //LED0-->PA.8 端口配置
 GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP; 		 //推挽输出
 GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;		 //IO口速度为50MHz
 GPIO_Init(GPIOB, &GPIO_InitStructure);					 //根据设定参数初始化GPIOA.8
// GPIO_SetBits(GPIOB,GPIO_Pin_13);						 //PA.8 输出高

// GPIO_InitStructure.GPIO_Pin = GPIO_Pin_2;	    		 //LED1-->PD.2 端口配置, 推挽输出
// GPIO_Init(GPIOD, &GPIO_InitStructure);	  				 //推挽输出 ，IO口速度为50MHz
 GPIO_ResetBits(GPIOB,GPIO_Pin_14); 						 //PB.13 输出高 
}

void Clear_Buffer(void)
{
	memset(USART1_RX_BUFF,0,sizeof(USART1_RX_BUFF));
	USART1_RX_LENGTH = 0;
}


void BC20_Init(void)
{
	delay_ms(1000);
	GPIO_SetBits(GPIOB,GPIO_Pin_13);
	delay_ms(1000);
	delay_ms(1000);
	GPIO_ResetBits(GPIOB,GPIO_Pin_13);
	
	//发送测试命令
	Uart1_SendStr("AT");
	delay_ms(300);
	
	Receive_Data = strstr((const char*)USART1_RX_BUFF,(const char*)"OK");
	
	while(Receive_Data == NULL)
	{
		Clear_Buffer();
		Uart1_SendStr("AT\r\n");
		delay_ms(300);
		Receive_Data = strstr((const char*)USART1_RX_BUFF,(const char*)"OK");
		
	}
	Clear_Buffer();
	//关闭自动休眠
	Uart1_SendStr("AT+QSCLK=0\r\n");
	delay_ms(300);
	Receive_Data = strstr((const char*)USART1_RX_BUFF,(const char*)"OK");
	
	while(Receive_Data == NULL)
	{
		Clear_Buffer();
		Uart1_SendStr("AT+QSCLK=0\r\n");
		delay_ms(300);
		Receive_Data = strstr((const char*)USART1_RX_BUFF,(const char*)"OK");
	}
	Clear_Buffer();
	
	Uart1_SendStr("AT+CFUN=1\r\n");
	delay_ms(300);
	Receive_Data = strstr((const char*)USART1_RX_BUFF,(const char*)"OK");
	while(Receive_Data == NULL)
	{
		Clear_Buffer();
		Uart1_SendStr("AT+CFUN=1\r\n");
		delay_ms(300);
		Receive_Data = strstr((const char*)USART1_RX_BUFF,(const char*)"OK");
	}
	Clear_Buffer();
	
	//关闭省电模式
	Uart1_SendStr("AT+CPSMS=0\r\n");
	delay_ms(300);
	Receive_Data = strstr((const char*)USART1_RX_BUFF,(const char*)"OK");
	while(Receive_Data == NULL)
	{
		Clear_Buffer();
		Uart1_SendStr("AT+CPSMS=0\r\n");
		delay_ms(300);
		Receive_Data = strstr((const char*)USART1_RX_BUFF,(const char*)"OK");
	}
	Clear_Buffer();
	
	//关闭eDRX模式
	Uart1_SendStr("AT+CEDRXS=0\r\n");
	delay_ms(300);
	Receive_Data = strstr((const char*)USART1_RX_BUFF,(const char*)"OK");
	while(Receive_Data == NULL)
	{
		Clear_Buffer();
		Uart1_SendStr("AT+CEDRXS=0\r\n");
		delay_ms(300);
		Receive_Data = strstr((const char*)USART1_RX_BUFF,(const char*)"OK");
	}
	Clear_Buffer();
	
	//获取卡号
	Uart1_SendStr( "AT+CIMI\r\n" );	
	delay_ms( 300 );
	Receive_Data = strstr((const char*)USART1_RX_BUFF,(const char*)"460");
	
	while(Receive_Data == NULL)
	{
		Clear_Buffer();
		Uart1_SendStr("AT+CIMI\r\n");
		delay_ms(300);
		Receive_Data = strstr((const char*)USART1_RX_BUFF,(const char*)"460");
	}
	Clear_Buffer();
	
	//激活网络
	Uart1_SendStr("AT+CGATT=1\r\n");
	delay_ms(300);
	Receive_Data = strstr((const char*)USART1_RX_BUFF,(const char*)"OK");
	Clear_Buffer();
	//查看网络
	Uart1_SendStr("AT+CGATT?\r\n");
	delay_ms(300);
	Receive_Data = strstr((const char*)USART1_RX_BUFF,(const char*)"+CGATT: 1");
	
	while( Receive_Data == NULL )
	{
		Clear_Buffer();
		Uart1_SendStr( "AT+CGATT?\r\n" );
		delay_ms( 300 );
		Receive_Data = strstr( (const char*)USART1_RX_BUFF, (const char*)"+CGATT: 1" );
	}
	Clear_Buffer();
	
	//查看获取的CSQ的值
	Uart1_SendStr( "AT+CESQ\r\n" );
	delay_ms( 300 );
	Receive_Data = strstr( (const char*)USART1_RX_BUFF, (const char*)"+CESQ" );
	if( Receive_Data )
	{
		BC20_Status.CSQ = (Receive_Data[7]-0x30) * 10 + (Receive_Data[8]-0x30 ); //将字符转换为数字
		if( ( BC20_Status.CSQ == 99 ) || ( (Receive_Data[7]-0x30) == 0 ) )	//说明没有信号或信号过小
		{
			while( 1 )
			{
				BC20_Status.NetStatus = 0;
				delay_ms( 1000 );
				 LED0 = !LED0;
			}
		}
		else
		{
			BC20_Status.NetStatus = 1;
		}
	}
}

/* 在该函数中使用了 $，是为了处理方便。在阿里云上“发布消息”时，输入的数据开头要带上 $ */
char BC20_Receive_Data( void )
{
	char* position = NULL;
	
	Receive_Data = strstr( (const char*)USART1_RX_BUFF, (const char*)"+QMTRECV" );
	if( Receive_Data )
	{
		position = strrchr( Receive_Data, '}' );
	}
	
	return *(position-20);		//返回 $ 后面的第一个字符
}
//配置MQTT
void Asso_QMTCFG( char* ProductKey, char* DeviceName, char* DeviceSecret )
{
	char temp[200];

	memset( temp, 0, sizeof( temp ) );	//清空 temp，避免隐藏错误
	
	strcat( temp, "AT+QMTCFG=\"aliauth\",0,\"" );	//AT+MQTCFG="aliauth",0,"
	strcat( temp, ProductKey );		//AT+MQTCFG="aliauth",0,"ProductKey
	strcat( temp, "\",\"" );		//AT+MQTCFG="aliauth",0,"ProductKey","
	strcat( temp, DeviceName ); 	//AT+MQTCFG="aliauth",0,"ProductKey","DeviceName
	strcat( temp, "\",\"" );		//AT+MQTCFG="aliauth",0,"ProductKey","DeviceName","
	strcat( temp, DeviceSecret );	//AT+MQTCFG="aliauth",0,"ProductKey","DeviceName","DeviceSecret
	strcat( temp, "\"\r\n" );		//AT+MQTCFG="aliauth",0,"ProductKey","DeviceName","DeviceSecret"\r\n

	Uart1_SendStr( temp );
}

//打开MQTT客户端网络
void Asso_QMTCONN( char* ConnectID )
{
	char temp[200];
	
	memset( temp, 0, sizeof( temp ) );	//清空 temp，避免隐藏错误
	
	strcat( temp, "AT+QMTCONN=0,\"" );	//AT+QMTCONN=0,"
	strcat( temp, ConnectID );			//AT+QMTCONN=0,"ConnectID
	strcat( temp, "\"\r\n" );			//AT+QMTCONN=0,"ConnectID"\r\n
	
	Uart1_SendStr( temp );
}

//订阅主题
void Asso_QMTSUB( char* SubTopic )
{
	char temp[200];
	
	memset( temp, 0, sizeof( temp ) );	//清空 temp，避免隐藏错误
	
	strcat( temp, "AT+QMTSUB=0,1,\"" );	//AT+QMTSUB=0,1,"
	strcat( temp, SubTopic );			//AT+QMTSUB=0,1,"SubTopic
	strcat( temp, "\",0\r\n" );			//AT+QMTSUB=0,1,"SubTopic",0\r\n
	
	Uart1_SendStr( temp );
}

//发布消息
void Asso_QMTPUB( char* PubTopic, char* value )
{
	char temp[200];
	
	memset( temp, 0, sizeof( temp ) );	//清空 temp，避免隐藏错误
	
	strcat( temp, "AT+QMTPUB=0,0,0,0,\"" );	//AT+QMTPUB=0,0,0,0,"
	strcat( temp, PubTopic );				//AT+QMTPUB=0,0,0,0,"PubTopic
	strcat( temp, "\",\"{params:{CurrentHumidity:" ); //AT+QMTPUB=0,0,0,0,"PubTopic","{params:{CurrentHumidity:
	strcat( temp, value );	//AT+QMTPUB=0,0,0,0,"PubTopic","{params:{CurrentHumidity:value
	strcat( temp, "}}\"" );	//AT+QMTPUB=0,0,0,0,"PubTopic","{params:{CurrentHumidity:value}}"
	
	Uart1_SendStr( temp );
}


//MQTT配置
void MQTT_Init( char* ProductKey, char* DeviceName, char* DeviceSecret, char* ConnectID )
{
	Asso_QMTCFG( ProductKey, DeviceName, DeviceSecret );
	delay_ms( 300 );

	
	Clear_Buffer();	
	//------------检验信息------------
	
	
	Uart1_SendStr( "AT+QMTOPEN=0,\"iot-as-mqtt.cn-shanghai.aliyuncs.com\",1883\r\n" );	//通过 tcp 方式连接阿里云
	delay_ms( 1000 );
	Receive_Data = strstr( (const char*)USART1_RX_BUFF, (const char*)"+QMTOPEN: 0,0" );	//查看返回状态

	
	/* 必须等到成功连上阿里云 */
	while( Receive_Data == NULL )
	{
		Clear_Buffer();
		Uart1_SendStr( "AT+QMTOPEN=0,\"iot-as-mqtt.cn-shanghai.aliyuncs.com\",1883\r\n" );	//通过 tcp 方式连接阿里云
		delay_ms( 1000 );
		Receive_Data = strstr( (const char*)USART1_RX_BUFF, (const char*)"+QMTOPEN: 0,0" );	//查看返回状态
	}
	
	
	Clear_Buffer();	
	//------------检验信息------------
	//
	
	Asso_QMTCONN( ConnectID );
	delay_ms( 300 );
	Receive_Data = strstr( (const char*)USART1_RX_BUFF, (const char*)"+QMTCONN: 0,0,0" );
	
	
	/* 必须等到成功匹配到 MQTT */
	while( Receive_Data == NULL )
	{
		Clear_Buffer();
		Asso_QMTCONN( ConnectID );
		delay_ms( 300 );
		Receive_Data = strstr( (const char*)USART1_RX_BUFF, (const char*)"+QMTCONN: 0,0,0" );	//查看返回状态	
	}
	
	
	Clear_Buffer();

}

void MQTT_Sub( char* SubTopic )
{
	Clear_Buffer();
	Asso_QMTSUB( SubTopic );
	delay_ms( 300 );
	Receive_Data = strstr( (const char*)USART1_RX_BUFF, (const char*)"+QMTSUB:" );
	
	while( Receive_Data == NULL )
	{
		Clear_Buffer();
		Asso_QMTSUB( SubTopic );
	delay_ms( 300 );
		Receive_Data = strstr( (const char*)USART1_RX_BUFF, (const char*)"+QMTSUB:" );
	}
	
	Clear_Buffer();
}

void MQTT_Pub( char* PubTopic, char* value )
{
	Clear_Buffer();
	Asso_QMTPUB( PubTopic, value );
	delay_ms( 300 );
	Receive_Data = strstr( (const char*)USART1_RX_BUFF, (const char*)"+QMTPUB: 0,0,0" );
	
	while( Receive_Data == NULL )
	{
		Receive_Data = strstr( (const char*)USART1_RX_BUFF, (const char*)"+QMTPUB: 0,0,0" );
	}
	
	Clear_Buffer();

}


