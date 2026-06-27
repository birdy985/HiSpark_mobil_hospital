#ifndef __BC20_H
#define __BC20_H

#include "sys.h"
#include "usart.h"
#include "delay.h"
#include "led.h"

#define PowerKey PBout(13)	// PB13
#define ResetKey PBout(14)	// PB13
void Clear_Buffer(void);	//清空缓存
void BC20_Init(void);			//初始化BC26
char BC20_Receive_Data(void);	//接收从云端发送过来的数据

void Asso_QMTCFG(char* ProductKey,char* DeviceName,char* DeviceSecret);	//
void Asso_QMTCONN(char* ConnectID);	
void Asso_QMTSUB(char* SubTopic);	
void Asso_QMTPUB(char* PubTopic,char* value);

void MQTT_Init(char* ProductKey,char* DeviceName,char* DeviceSecret,char* ConnectID);	//MQTT 连接初始化
void MQTT_Sub(char* SubTopic);	//订阅主题
void MQTT_Pub(char* PubTopic,char* value);	//发布主题
void PowerKey_Init(void);
void ResetKey_Init(void);
typedef struct
{
	u8 CSQ;			//网络信号值
	u8 NetStatus;	//网络状态值
} BC20;




#endif



