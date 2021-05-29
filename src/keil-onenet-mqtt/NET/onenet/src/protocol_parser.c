/**
	************************************************************
	************************************************************
	************************************************************
	*	文件名： 	protocol_parser.c
	*
	*	作者： 		张继瑞
	*
	*	日期： 		2019-07-08
	*
	*	版本： 		V1.0
	*
	*	说明： 		协议解析器
	*
	*	修改记录：	MQTT
	************************************************************
	************************************************************
	************************************************************
**/

//协议
#include "mqttkit.h"

//驱动
#include "usart.h"

//
#include "protocol_parser.h"

//C库
#include <stdio.h>
#include <string.h>
#include <ctype.h>


#define PRINT		DebugPrintf


#if(SEND_PASER_EN == 1 || RECV_PASER_EN == 1)

static char print_buf[64];


//==========================================================
//	函数名称：	print_hexstring
//
//	函数功能：	将hex转为字符串
//
//	入口参数：	hex：hex数据
//				len：数据长度
//
//	返回参数：	返回转换后的缓存指针
//
//	说明：		格式：[0xXX ...]
//==========================================================
static char *print_hexstring(const unsigned char *hex, unsigned char len)
{
	
	char temp_buf[8];

	unsigned char i = 0;
	
	if(hex == (void *)0 || len == 0)									//参数合法性
		return (void *)0;
	
	for(; i < sizeof(print_buf); i++)									//清缓存
		print_buf[i] = 0;
	
	print_buf[0] = '[';
	
	for(i = 0; i < len; i++)
	{
		snprintf(temp_buf, sizeof(temp_buf), "0x%02X", hex[i]);			//格式化
		strcat(print_buf, temp_buf);									//复制到缓存末尾
		
		if(i < len - 1)
			strcat(print_buf, " ");										//添加空格
	}
	
	strcat(print_buf, "]");
	
	return print_buf;

}

//==========================================================
//	函数名称：	print_ascii
//
//	函数功能：	将hex转为ASCII字符
//
//	入口参数：	hex：hex数据
//				len：数据长度
//
//	返回参数：	返回转换后的缓存指针
//
//	说明：		
//==========================================================
static char *print_ascii(const unsigned char *hex, unsigned char len)
{

	char temp_buf[8];

	unsigned char i = 0;
	
	if(hex == (void *)0 || len == 0)									//参数合法性
		return (void *)0;
	
	for(; i < sizeof(print_buf); i++)									//清缓存
		print_buf[i] = 0;
	
	for(i = 0; i < len; i++)
	{
		if(isprint(hex[i]))												//是否为可打印字符
		{
			snprintf(temp_buf, sizeof(temp_buf), "%c", hex[i]);			//格式化
			strcat(print_buf, temp_buf);								//复制到缓存末尾
		}
	}
	
	return print_buf;

}

static int32 MQTT_ReadLength(const uint8 *stream, int32 size, uint32 *len)
{
	
	int32 i;
	const uint8 *in = stream;
	uint32 multiplier = 1;

	*len = 0;
	for(i = 0; i < size; ++i)
	{
		*len += (in[i] & 0x7f) * multiplier;

		if(!(in[i] & 0x80))
		{
			return i + 1;
		}

		multiplier <<= 7;
		if(multiplier >= 2097152)		//128 * *128 * *128
		{
			return -2;					// error, out of range
		}
	}

	return -1;							// not complete

}

#endif

#if(SEND_PASER_EN == 1)

//==========================================================
//	函数名称：	check_devid
//
//	函数功能：	检查devid是否合法
//
//	入口参数：	devid：设备ID
//				len：数据长度
//
//	返回参数：	0-成功	1-失败
//
//	说明：		
//==========================================================
static _Bool check_devid(const unsigned char *devid, unsigned char len)
{
	
	if(devid == (void *)0 || len == 0)			//参数合法性
		return 1;
	
	while(len--)
	{
		if(*devid < '0' || *devid > '9')		//判断是否为数字字符
			return 1;
		
		devid++;
	}
	
	return 0;

}

#endif

#if(SEND_PASER_EN == 1)

//==========================================================
//	函数名称：	Protocol_Parser_Connect
//
//	函数功能：	连接包协议解析输出打印
//
//	入口参数：	pro_buf：协议包
//
//	返回参数：	0-成功	其他-错误
//
//	说明：		
//==========================================================
static unsigned char Protocol_Parser_Connect(const unsigned char *pro_buf)
{

	unsigned char index = 0, mqtt_flag = 0;					//index：pro_buf索引		mqtt_flag：mqtt连接标志位
	unsigned short temp = 0;								//临时保存一些需要计算的判断数据
	unsigned int len = 0;									//此包数据剩余长度(字节)
	_Bool flag = 0;											//判断正误标志
	
//--------------------------------------------------------------------------------------------------------------------------------------------
	temp = pro_buf[index] >> 4;								//消息类型在固定头部第一个字节高四位
	PRINT("第%d字节---[0x%02X]:[%s]登录请求\r\n", index, pro_buf[index], (temp == MQTT_PKT_CONNECT) ? "OK": "ERR");
	if(temp != MQTT_PKT_CONNECT)
		return 1;
	index++;
	
//--------------------------------------------------------------------------------------------------------------------------------------------
	temp = MQTT_ReadLength(pro_buf + index, 4, &len);
	PRINT("第%d~%d字节---%s:[%s]剩余消息长度---%d字节\r\n", index, index + temp - 1, print_hexstring(pro_buf + index, temp),
															(len > 0) ? "OK": "ERR", len);
	if(len == 0 || temp == 0)
		return 2;
	index += temp;
	
//--------------------------------------------------------------------------------------------------------------------------------------------
	temp = pro_buf[index] << 8 | pro_buf[index + 1];
	PRINT("第%d~%d字节---[0x%02X 0x%02X]:[%s]协议名长度---%d字节\r\n", index, index + 1, pro_buf[index], pro_buf[index + 1],
																		(temp == 4) ? "OK": "ERR", temp);
	if(temp != 4)
		return 3;
	index += 2;
	
//--------------------------------------------------------------------------------------------------------------------------------------------
	if(pro_buf[index] == 'M' && pro_buf[index + 1] == 'Q' && pro_buf[index + 2] == 'T' && pro_buf[index + 3] == 'T')
		PRINT("第%d~%d字节---[0x4D 0x51 0x54 0x54]:[OK]协议名---MQTT\r\n", index, index + 2, pro_buf[index]);
	else
	{
		PRINT("第%d~%d字节---%s:[ERR]协议名---协议描述必须为MQTT\r\n",
									index, index + 2, print_hexstring(pro_buf + index, 4));
		return 4;
	}
	index += 4;
	
//--------------------------------------------------------------------------------------------------------------------------------------------
	PRINT("第%d字节---[0x%02X]:[%s]协议等级\r\n", index, pro_buf[index], (pro_buf[index] < 10) ? "OK": "ERR");
	if(temp >= 10)
		return 5;
	index++;
	
//--------------------------------------------------------------------------------------------------------------------------------------------
	mqtt_flag = pro_buf[index];
	PRINT("第%d字节---[0x%02X]:[OK]连接标志\r\n", index, pro_buf[index]);
	index++;
	
//--------------------------------------------------------------------------------------------------------------------------------------------
	temp = pro_buf[index] << 8 | pro_buf[index + 1];
	PRINT("第%d~%d字节---[0x%02X 0x%02X]:[%s]保持连接时间---%d秒\r\n",
								index, index + 1, pro_buf[index], pro_buf[index + 1], (temp != 0) ? "OK": "ERR", temp);
	if(temp == 0)
		return 6;
	index += 2;
	
//--------------------------------------------------------------------------------------------------------------------------------------------
	temp = pro_buf[index] << 8 | pro_buf[index + 1];
	PRINT("第%d~%d字节---[0x%02X 0x%02X]:[%s]设备ID长度---%d字节\r\n", index, index + 1,
								pro_buf[index], pro_buf[index + 1], ((temp >= 7) && (temp <= 16)) ? "OK": "ERR", temp);
	if((temp < 7) || (temp > 16))
		return 7;
	index += 2;
	
//--------------------------------------------------------------------------------------------------------------------------------------------
	if(check_devid(pro_buf + index, temp) == 0)				//devid合法性
		flag = 1;
	else
		flag = 0;
	PRINT("第%d~%d字节---%s:[%s]", index, index + temp - 1, print_hexstring(pro_buf + index, temp), (flag == 1) ? "OK": "ERR");
	PRINT("设备ID---%s\r\n", print_ascii(pro_buf + index, temp));
	if(flag == 0)
		return 8;
	index += temp;
	
//--------------------------------------------------------------------------------------------------------------------------------------------
	if(mqtt_flag & MQTT_CONNECT_WILL_FLAG)					//连接标志是否设置了will flag
	{
		unsigned short will_msg_len = pro_buf[index] << 8 | pro_buf[index + 1];
		
//--------------------------------------------------------------------------------------------------------------------------------------------
		PRINT("第%d~%d字节---[0x%02X 0x%02X]:[%s]Will Topic长度---%d字节\r\n", index, index + 1, pro_buf[index], pro_buf[index + 1],
																				(will_msg_len > 0) ? "OK" : "ERR", will_msg_len);
		if(will_msg_len == 0)
			return 9;
		index += 2;
		
//--------------------------------------------------------------------------------------------------------------------------------------------
		if(will_msg_len < 32)
			PRINT("第%d~%d字节---%s:[OK]", index, index + temp - 1, print_hexstring(pro_buf + index, will_msg_len));
		else
			PRINT("第%d~%d字节---[OK]", index, index + temp - 1);
		
		PRINT("Will Topic---%s\r\n", print_ascii(pro_buf + index, temp));
		index += will_msg_len;
		
//--------------------------------------------------------------------------------------------------------------------------------------------
		will_msg_len = pro_buf[index] << 8 | pro_buf[index + 1];
		PRINT("第%d~%d字节---[0x%02X 0x%02X]:[%s]Will Msg长度---%d字节\r\n", index, index + 1, pro_buf[index], pro_buf[index + 1],
																				(will_msg_len > 0) ? "OK" : "ERR", will_msg_len);
		if(will_msg_len == 0)
			return 10;
		index += 2;
		
//--------------------------------------------------------------------------------------------------------------------------------------------
		if(will_msg_len < 32)
			PRINT("第%d~%d字节---%s:[OK]", index, index + temp - 1, print_hexstring(pro_buf + index, will_msg_len));
		else
			PRINT("第%d~%d字节---[OK]", index, index + temp - 1);
		
		PRINT("Will Msg---%s\r\n", print_ascii(pro_buf + index, temp));
		index += will_msg_len;
	}
	
	if(mqtt_flag & MQTT_CONNECT_USER_NAME)
	{
		unsigned short user_len = pro_buf[index] << 8 | pro_buf[index + 1];
		
//--------------------------------------------------------------------------------------------------------------------------------------------
		PRINT("第%d~%d字节---[0x%02X 0x%02X]:[%s]User Name长度---%d字节\r\n", index, index + 1, pro_buf[index],
																				pro_buf[index + 1], (user_len > 0) ? "OK" : "ERR", user_len);
		if(user_len == 0)
			return 11;
		index += 2;
		temp = user_len;
		
//--------------------------------------------------------------------------------------------------------------------------------------------
		if(user_len < 32)
			PRINT("第%d~%d字节---%s:[OK]", index, index + temp - 1, print_hexstring(pro_buf + index, user_len));
		else
			PRINT("第%d~%d字节---[OK]", index, index + temp - 1);
		
		PRINT("User Name---%s\r\n", print_ascii(pro_buf + index, temp));
		index += user_len;
	}
	
	if(mqtt_flag & MQTT_CONNECT_PASSORD)
	{
		unsigned short pswd_len = pro_buf[index] << 8 | pro_buf[index + 1];
		
//--------------------------------------------------------------------------------------------------------------------------------------------
		PRINT("第%d~%d字节---[0x%02X 0x%02X]:[%s]PassWord长度---%d字节\r\n", index, index + 1, pro_buf[index],
																				pro_buf[index + 1], (pswd_len > 0) ? "OK" : "ERR", pswd_len);
		if(pswd_len == 0)
			return 12;
		index += 2;
		temp = pswd_len;
		
//--------------------------------------------------------------------------------------------------------------------------------------------
		if(pswd_len < 32)
			PRINT("第%d~%d字节---%s:[OK]", index, index + temp - 1, print_hexstring(pro_buf + index, pswd_len));
		else
			PRINT("第%d~%d字节---[OK]", index, index + temp - 1);
		
		PRINT("PassWord---%s\r\n", print_ascii(pro_buf + index, temp));
	}
	
	return 0;

}

//==========================================================
//	函数名称：	Protocol_Parser_DisConnect
//
//	函数功能：	断开连接数据包协议解析输出打印
//
//	入口参数：	pro_buf：协议包
//
//	返回参数：	0-成功	其他-错误
//
//	说明：		
//==========================================================
static unsigned char Protocol_Parser_DisConnect(const unsigned char *pro_buf)
{
	
	unsigned char index = 0, temp = 0;		//index：pro_buf索引		temp：临时保存一些需要计算的判断数据

//--------------------------------------------------------------------------------------------------------------------------------------------
	temp = pro_buf[index] >> 4;				//消息类型在固定头部第一个字节高四位
	PRINT("第%d字节---[0x%02X]:[%s]断开连接请求\r\n", index, pro_buf[index], (temp == MQTT_PKT_CONNECT) ? "OK": "ERR");
	if(temp != MQTT_PKT_CONNECT)
		return 1;
	index++;
	
//--------------------------------------------------------------------------------------------------------------------------------------------
	PRINT("第%d字节---[0x%02X]:[%s]剩余消息长度---%d字节\r\n", index, pro_buf[index], (pro_buf[index] == 0) ? "OK" : "ERR", pro_buf[index]);
	if(pro_buf[index] != 0)
		return 2;
	
	return 0;

}

//==========================================================
//	函数名称：	Protocol_Parser_Publish
//
//	函数功能：	发布数据包协议解析输出打印
//
//	入口参数：	pro_buf：协议包
//
//	返回参数：	0-成功	其他-错误
//
//	说明：		
//==========================================================
static unsigned char Protocol_Parser_Publish(const unsigned char *pro_buf)
{

	unsigned char index = 0, mqtt_flag = 0, remain_cnt = 0;		//index：pro_buf索引		mqtt_flag：固定头部的第一个字节	remain_cnt：剩余字节计数
	unsigned short topic_len = 0;								//topic长度
	unsigned int temp = 0;										//临时保存一些需要计算的判断数据
	unsigned int len = 0;										//此包数据剩余长度(字节)
	
//--------------------------------------------------------------------------------------------------------------------------------------------
	mqtt_flag = pro_buf[index];
	temp = pro_buf[index] >> 4;									//消息类型在固定头部第一个字节高四位
	PRINT("第%d字节---[0x%02X]:[%s]发布消息\r\n", index, pro_buf[index], (temp == MQTT_PKT_PUBLISH) ? "OK": "ERR");
	if(temp != MQTT_PKT_PUBLISH)
		return 1;
	index++;
	
//--------------------------------------------------------------------------------------------------------------------------------------------
	temp = MQTT_ReadLength(pro_buf + index, 4, &len);
	remain_cnt = temp;
	PRINT("第%d~%d字节---%s:[%s]剩余消息长度---%d字节\r\n", index, index + temp - 1,
															print_hexstring(pro_buf + index, temp), (len > 3) ? "OK": "ERR", len);
	if(len <= 3 || temp == 0)
		return 2;
	index += temp;
	
//--------------------------------------------------------------------------------------------------------------------------------------------
	topic_len = pro_buf[index] << 8 | pro_buf[index + 1];
	PRINT("第%d~%d字节---[0x%02X 0x%02X]:[%s]Topic长度---%d字节\r\n", index, index + 1, pro_buf[index],
																		pro_buf[index + 1], (topic_len > 0) ? "OK" : "ERR", topic_len);
	if(topic_len == 0)
		return 3;
	index += 2;
	
//--------------------------------------------------------------------------------------------------------------------------------------------
	if(topic_len < 32)
		PRINT("第%d~%d字节---%s:[OK]", index, index + topic_len - 1, print_hexstring(pro_buf + index, topic_len));
	else
		PRINT("第%d~%d字节---[OK]", index, index + topic_len - 1);
	
	PRINT("Topic---%s\r\n", print_ascii(pro_buf + index, topic_len));
	index += topic_len;
	
	if(((mqtt_flag >> 1) & MQTT_QOS_LEVEL1) || ((mqtt_flag >> 1) & MQTT_QOS_LEVEL2))
	{
//--------------------------------------------------------------------------------------------------------------------------------------------
		PRINT("第%d~%d字节---[0x%02X 0x%02X]:[OK]PKT ID\r\n", index, index + 1, pro_buf[index], pro_buf[index + 1]);
		index += 2;
	}
	
	if(pro_buf[index] == 2)																	//二进制文件
	{
//--------------------------------------------------------------------------------------------------------------------------------------------
		PRINT("第%d字节---[0x%02X]:[OK]数据类型\r\n", index, pro_buf[index]);
		index++;
		
//--------------------------------------------------------------------------------------------------------------------------------------------
		temp = pro_buf[index] << 8 | pro_buf[index + 1];
		PRINT("第%d~%d字节---[0x%02X 0x%02X]:[OK]Bin Head长度---%d字节\r\n", index, index + 1, pro_buf[index],
																			pro_buf[index + 1], (temp > 12) ? "OK": "ERR", temp);
		if(temp <= 12)
			return 4;
		index += 2;
		
//--------------------------------------------------------------------------------------------------------------------------------------------
		PRINT("第%d~%d字节---%s:[OK]Bin Head", index, index + temp - 1, print_hexstring(pro_buf + index, temp));
		PRINT("---%s\r\n", print_ascii(pro_buf + index, temp));
		index += temp;
		
//--------------------------------------------------------------------------------------------------------------------------------------------
		temp = pro_buf[index] << 24 | pro_buf[index + 1] << 16 | pro_buf[index + 2] << 8 | pro_buf[index + 3];
		PRINT("第%d~%d字节---%s:[OK]Bin File长度---%d字节\r\n", index, index + 3, print_hexstring(pro_buf + index, 4), temp);
		index += 4;
	}
	else if((pro_buf[index] == 1) || (pro_buf[index] > 2 && pro_buf[index] < 6))			//上传数据点
	{
//--------------------------------------------------------------------------------------------------------------------------------------------
		PRINT("第%d字节---[0x%02X]:[OK]数据类型\r\n", index, pro_buf[index]);
		index++;
		
//--------------------------------------------------------------------------------------------------------------------------------------------
		temp = pro_buf[index] << 8 | pro_buf[index + 1];
		PRINT("第%d~%d字节---[0x%02X 0x%02X]:[OK]数据长度---%d字节\r\n", index, index + 1, pro_buf[index], pro_buf[index + 1], temp);
		index += 2;
	}
	else																					//发布消息
	{
		unsigned int payload_len = 0;
																							//看是否有Qos标志，当为0的时候是没有pkt id的
		if(((mqtt_flag >> 1) & MQTT_QOS_LEVEL1) || ((mqtt_flag >> 1) & MQTT_QOS_LEVEL2))	//命令接收
			payload_len = len - topic_len - remain_cnt - 3;
		else																				//发布消息
			payload_len = len - topic_len - 2;
		
//--------------------------------------------------------------------------------------------------------------------------------------------
		PRINT("第%d~%d字节---[OK]发布消息---%d字节\r\n", index, index + payload_len - 1, payload_len);
		if(payload_len < 56)
			PRINT("%s\r\n", print_ascii(pro_buf + index, payload_len));
	}
	
	return 0;

}

//==========================================================
//	函数名称：	Protocol_Parser_Subscribe
//
//	函数功能：	订阅数据包协议解析输出打印
//
//	入口参数：	pro_buf：协议包
//
//	返回参数：	0-成功	其他-错误
//
//	说明：		
//==========================================================
static unsigned char Protocol_Parser_Subscribe(const unsigned char *pro_buf)
{

	unsigned char index = 0;						//index：pro_buf索引
	unsigned int temp = 0;							//临时保存一些需要计算的判断数据
	unsigned int len = 0;							//此包数据剩余长度(字节)
	
//--------------------------------------------------------------------------------------------------------------------------------------------
	temp = pro_buf[index] >> 4;						//消息类型在固定头部第一个字节高四位
	PRINT("第%d字节---[0x%02X]:[%s]订阅消息\r\n", index, pro_buf[index], (temp == MQTT_PKT_SUBSCRIBE) ? "OK": "ERR");
	if(temp != MQTT_PKT_SUBSCRIBE)
		return 1;
	index++;
	
//--------------------------------------------------------------------------------------------------------------------------------------------
	temp = MQTT_ReadLength(pro_buf + index, 4, &len);
	PRINT("第%d~%d字节---%s:[%s]剩余消息长度---%d字节\r\n", index, index + temp - 1,
															print_hexstring(pro_buf + index, temp), (len > 4) ? "OK": "ERR", len);
	if(len <= 4 || temp == 0)
		return 2;
	index += temp;
	
//--------------------------------------------------------------------------------------------------------------------------------------------
	PRINT("第%d~%d字节---[0x%02X 0x%02X]:[OK]PKT ID\r\n", index, index + 1, pro_buf[index], pro_buf[index + 1]);
	index += 2;
	len -= 2;
	
	while(len)										//根据消息剩余长度来判断是否到了最后一个topic
	{
//--------------------------------------------------------------------------------------------------------------------------------------------
		temp = pro_buf[index] << 8 | pro_buf[index + 1];
		PRINT("第%d~%d字节---[0x%02X 0x%02X]:[%s]Topic长度---%d字节\r\n", index, index + 1, pro_buf[index],
																			pro_buf[index + 1], (temp > 0) ? "OK" : "ERR" , temp);
		if(temp == 0)
			return 3;
		index += 2;
		len -= 2;
		
//--------------------------------------------------------------------------------------------------------------------------------------------
		PRINT("第%d~%d字节---%s:[OK]Topic", index, index + temp - 1, print_hexstring(pro_buf + index, temp));
		PRINT("---%s\r\n", print_ascii(pro_buf + index, temp));
		index += temp;
		len -= temp;
		
//--------------------------------------------------------------------------------------------------------------------------------------------
		PRINT("第%d字节---[0x%02X]:[%s]QoS\r\n", index, pro_buf[index], (pro_buf[index] <= 2) ? "OK" : "ERR");
		if(pro_buf[index] > 2)
			return 4;
		index++;
		len--;
	}
	
	return 0;

}

//==========================================================
//	函数名称：	Protocol_Parser_UnSubscribe
//
//	函数功能：	取消订阅数据包协议解析输出打印
//
//	入口参数：	pro_buf：协议包
//
//	返回参数：	0-成功	其他-错误
//
//	说明：		
//==========================================================
static unsigned char Protocol_Parser_UnSubscribe(const unsigned char *pro_buf)
{

	unsigned char index = 0;						//index：pro_buf索引
	unsigned int temp = 0;							//临时保存一些需要计算的判断数据
	unsigned int len = 0;							//此包数据剩余长度(字节)
	
//--------------------------------------------------------------------------------------------------------------------------------------------
	temp = pro_buf[index] >> 4;						//消息类型在固定头部第一个字节高四位
	PRINT("第%d字节---[0x%02X]:[%s]取消订阅消息\r\n", index, pro_buf[index], (temp == MQTT_PKT_UNSUBSCRIBE) ? "OK": "ERR");
	if(temp != MQTT_PKT_UNSUBSCRIBE)
		return 1;
	index++;
	
//--------------------------------------------------------------------------------------------------------------------------------------------
	temp = MQTT_ReadLength(pro_buf + index, 4, &len);
	PRINT("第%d~%d字节---%s:[%s]剩余消息长度---%d字节\r\n", index, index + temp - 1,
															print_hexstring(pro_buf + index, temp), (len > 3) ? "OK": "ERR", len);
	if(len <= 3 || temp == 0)
		return 2;
	index += temp;
	
//--------------------------------------------------------------------------------------------------------------------------------------------
	PRINT("第%d~%d字节---[0x%02X 0x%02X]:[OK]PKT ID\r\n", index, index + 1, pro_buf[index], pro_buf[index + 1]);
	index += 2;
	len -= 2;
	
	while(len)										//根据消息剩余长度来判断是否到了最后一个topic
	{
//--------------------------------------------------------------------------------------------------------------------------------------------
		temp = pro_buf[index] << 8 | pro_buf[index + 1];
		PRINT("第%d~%d字节---[0x%02X 0x%02X]:[%s]Topic长度---%d字节\r\n", index, index + 1, pro_buf[index],
																			pro_buf[index + 1], (temp > 0) ? "OK" : "ERR" , temp);
		if(temp == 0)
			return 3;
		index += 2;
		len -= 2;
		
//--------------------------------------------------------------------------------------------------------------------------------------------
		PRINT("第%d~%d字节---%s:[OK]Topic", index, index + temp - 1, print_hexstring(pro_buf + index, temp));
		PRINT("---%s\r\n", print_ascii(pro_buf + index, temp));
		index += temp;
		len -= temp;
	}
	
	return 0;

}

//==========================================================
//	函数名称：	Protocol_Parser_Ping
//
//	函数功能：	心跳数据包协议解析输出打印
//
//	入口参数：	pro_buf：协议包
//
//	返回参数：	0-成功	其他-错误
//
//	说明：		
//==========================================================
static unsigned char Protocol_Parser_Ping(const unsigned char *pro_buf)
{

	unsigned char index = 0, temp = 0;				//index：pro_buf索引		temp：临时保存一些需要计算的判断数据
	
//--------------------------------------------------------------------------------------------------------------------------------------------
	temp = pro_buf[index] >> 4;						//消息类型在固定头部第一个字节高四位
	PRINT("第%d字节---[0x%02X]:[%s]心跳数据\r\n", index, pro_buf[index], (temp == MQTT_PKT_PINGREQ) ? "OK": "ERR");
	if(temp != MQTT_PKT_PINGREQ)
		return 1;
	index++;
	
//--------------------------------------------------------------------------------------------------------------------------------------------
	PRINT("第%d字节---[0x%02X]:[%s]剩余消息长度---%d字节\r\n", index, pro_buf[index], (pro_buf[index] == 0) ? "OK": "ERR", pro_buf[index]);
	if(pro_buf[index] != 0)
		return 2;
	
	return 0;

}

#endif

#if(RECV_PASER_EN == 1)

//==========================================================
//	函数名称：	Protocol_Parser_ConnectResp
//
//	函数功能：	连接回复数据包协议解析输出打印
//
//	入口参数：	pro_buf：协议包
//
//	返回参数：	0-成功	其他-错误
//
//	说明：		
//==========================================================
static unsigned char Protocol_Parser_ConnectResp(const unsigned char *pro_buf)
{

	unsigned char index = 0;						//index：pro_buf索引
	unsigned int temp = 0;							//临时保存一些需要计算的判断数据
	unsigned int len = 0;							//此包数据剩余长度(字节)
	
//--------------------------------------------------------------------------------------------------------------------------------------------
	temp = pro_buf[index] >> 4;						//消息类型在固定头部第一个字节高四位
	PRINT("第%d字节---[0x%02X]:[%s]连接回复\r\n", index, pro_buf[index], (temp == MQTT_PKT_CONNACK) ? "OK": "ERR");
	if(temp != MQTT_PKT_CONNACK)
		return 1;
	index++;
	
//--------------------------------------------------------------------------------------------------------------------------------------------
	temp = MQTT_ReadLength(pro_buf + index, 4, &len);
	PRINT("第%d~%d字节---%s:[%s]剩余消息长度---%d字节\r\n", index, index + temp - 1,
															print_hexstring(pro_buf + index, temp), (len == 2) ? "OK": "ERR", len);
	if(len != 2 || temp == 0)
		return 2;
	index += temp;
	
//--------------------------------------------------------------------------------------------------------------------------------------------
	PRINT("第%d字节---[0x%02X]:[%s]Session Present标志\r\n", index, pro_buf[index], (pro_buf[index] < 2) ? "OK": "ERR");
	if(pro_buf[index] >= 2)
		return 3;
	index++;
	
//--------------------------------------------------------------------------------------------------------------------------------------------
	PRINT("第%d字节---[0x%02X]:[%s]回复结果\r\n", index, pro_buf[index], (pro_buf[index] < 6) ? "OK": "ERR");
	if(pro_buf[index] >= 6)
		return 4;
#if 0
	{
		char *str = (void *)0;
		
		PRINT("第%d字节---[0x%02X]:[OK]连接结果---%d,", index, pro_buf[index], pro_buf[index]);
		
		switch(pro_buf[index])
		{
			case 0:		str = "连接成功";					break;
			case 1:		str = "协议版本错误";				break;
			case 2:		str = "非法的clientid";				break;
			case 3:		str = "服务不可用";					break;
			case 4:		str = "用户名或密码错误";			break;
			case 5:		str = "非法链接(比如token非法)";		break;
		}
		
		PRINT("%s\r\n", str);
	}
#endif
	
	return 0;

}

//==========================================================
//	函数名称：	Protocol_Parser_PingResp
//
//	函数功能：	心跳回复数据包协议解析输出打印
//
//	入口参数：	pro_buf：协议包
//
//	返回参数：	0-成功	其他-错误
//
//	说明：		
//==========================================================
static unsigned char Protocol_Parser_PingResp(const unsigned char *pro_buf)
{

	unsigned char index = 0, temp = 0;				//index：pro_buf索引		temp：临时保存一些需要计算的判断数据
	
//--------------------------------------------------------------------------------------------------------------------------------------------
	temp = pro_buf[index] >> 4;						//消息类型在固定头部第一个字节高四位
	PRINT("第%d字节---[0x%02X]:[%s]心跳回复\r\n", index, pro_buf[index], (temp == MQTT_PKT_PINGRESP) ? "OK": "ERR");
	if(temp != MQTT_PKT_PINGRESP)
		return 1;
	index++;
	
//--------------------------------------------------------------------------------------------------------------------------------------------
	PRINT("第%d字节---[0x%02X]:[%s]剩余消息长度---%d字节\r\n", index, pro_buf[index], (pro_buf[index] == 0) ? "OK": "ERR", pro_buf[index]);
	if(pro_buf[index] != 0)
		return 1;
	
	return 0;

}

//==========================================================
//	函数名称：	Protocol_Parser_PublishResp
//
//	函数功能：	发布消息回复数据包协议解析输出打印
//
//	入口参数：	pro_buf：协议包
//
//	返回参数：	0-成功	其他-错误
//
//	说明：		
//==========================================================
static unsigned char Protocol_Parser_PublishResp(const unsigned char *pro_buf)
{

	unsigned char index = 0;						//index：pro_buf索引
	unsigned int temp = 0;							//临时保存一些需要计算的判断数据
	unsigned int len = 0;							//此包数据剩余长度(字节)
	
//--------------------------------------------------------------------------------------------------------------------------------------------
	temp = pro_buf[index] >> 4;						//消息类型在固定头部第一个字节高四位
	PRINT("第%d字节---[0x%02X]:[%s]发布消息回复\r\n", index, pro_buf[index], (temp == MQTT_PKT_PUBACK) ? "OK": "ERR");
	if(temp != MQTT_PKT_PUBACK)
		return 1;
	index++;
	
//--------------------------------------------------------------------------------------------------------------------------------------------
	temp = MQTT_ReadLength(pro_buf + index, 4, &len);
	PRINT("第%d~%d字节---%s:[%s]剩余消息长度---%d字节\r\n", index, index + temp - 1,
															print_hexstring(pro_buf + index, temp), (len > 1) ? "OK": "ERR", len);
	if(len <= 1 || temp == 0)
		return 2;
	index += temp;
	
//--------------------------------------------------------------------------------------------------------------------------------------------
	PRINT("第%d~%d字节---[0x%02X 0x%02X]:[OK]PKT ID\r\n", index, index + 1, pro_buf[index], pro_buf[index + 1]);
	
	return 0;

}

//==========================================================
//	函数名称：	Protocol_Parser_SubscribeResp
//
//	函数功能：	订阅回复数据包协议解析输出打印
//
//	入口参数：	pro_buf：协议包
//
//	返回参数：	0-成功	其他-错误
//
//	说明：		
//==========================================================
static unsigned char Protocol_Parser_SubscribeResp(const unsigned char *pro_buf)
{

	unsigned char index = 0;						//index：pro_buf索引
	unsigned int temp = 0;							//临时保存一些需要计算的判断数据
	unsigned int len = 0;							//此包数据剩余长度(字节)
	
//--------------------------------------------------------------------------------------------------------------------------------------------
	temp = pro_buf[index] >> 4;						//消息类型在固定头部第一个字节高四位
	PRINT("第%d字节---[0x%02X]:[%s]订阅回复\r\n", index, pro_buf[index], (temp == MQTT_PKT_SUBACK) ? "OK": "ERR");
	if(temp != MQTT_PKT_SUBACK)
		return 1;
	index++;
	
//--------------------------------------------------------------------------------------------------------------------------------------------
	temp = MQTT_ReadLength(pro_buf + index, 4, &len);
	PRINT("第%d~%d字节---%s:[%s]剩余消息长度---%d字节\r\n", index, index + temp - 1,
															print_hexstring(pro_buf + index, temp), (len > 2) ? "OK": "ERR", len);
	if(len <= 2 || temp == 0)
		return 2;
	index += temp;
	
//--------------------------------------------------------------------------------------------------------------------------------------------
	PRINT("第%d~%d字节---[0x%02X 0x%02X]:[OK]PKT ID\r\n", index, index + 1, pro_buf[index], pro_buf[index + 1]);
	index += 2;
	
//--------------------------------------------------------------------------------------------------------------------------------------------
#if 0
	PRINT("第%d字节---[0x%02X]:[OK]订阅回复结果\r\n", index, pro_buf[index]);
#else
	{
		char *str = (void *)0;
		
		PRINT("第%d字节---[0x%02X]:[OK]订阅结果---%d,", index, pro_buf[index], pro_buf[index]);
		
		switch(pro_buf[index])
		{
			case 0:		str = "订阅成功(Qos = 0)";			break;
			case 1:		str = "订阅成功(Qos = 1)";			break;
			case 2:		str = "订阅成功(Qos = 2)";			break;
			case 0x80:	str = "订阅失败";					break;
		}
		
		PRINT("%s\r\n", str);
	}
#endif
	
	
	return 0;

}

//==========================================================
//	函数名称：	Protocol_Parser_UnSubscribeResp
//
//	函数功能：	取消订阅回复数据包协议解析输出打印
//
//	入口参数：	pro_buf：协议包
//
//	返回参数：	0-成功	其他-错误
//
//	说明：		
//==========================================================
static unsigned char Protocol_Parser_UnSubscribeResp(const unsigned char *pro_buf)
{

	unsigned char index = 0;						//index：pro_buf索引
	unsigned int temp = 0;							//临时保存一些需要计算的判断数据
	unsigned int len = 0;							//此包数据剩余长度(字节)
	
//--------------------------------------------------------------------------------------------------------------------------------------------
	temp = pro_buf[index] >> 4;						//消息类型在固定头部第一个字节高四位
	PRINT("第%d字节---[0x%02X]:[%s]取消订阅回复\r\n", index, pro_buf[index], (temp == MQTT_PKT_UNSUBACK) ? "OK": "ERR");
	if(temp != MQTT_PKT_UNSUBACK)
		return 1;
	index++;
	
//--------------------------------------------------------------------------------------------------------------------------------------------
	temp = MQTT_ReadLength(pro_buf + index, 4, &len);
	PRINT("第%d~%d字节---%s:[%s]剩余消息长度---%d字节\r\n", index, index + temp - 1,
															print_hexstring(pro_buf + index, temp), (len > 1) ? "OK": "ERR", len);
	if(len <= 1 || temp == 0)
		return 2;
	index += temp;
	
//--------------------------------------------------------------------------------------------------------------------------------------------
	PRINT("第%d~%d字节---[0x%02X 0x%02X]:[OK]PKT ID\r\n", index, index + 1, pro_buf[index], pro_buf[index + 1]);
	
	return 0;

}

//==========================================================
//	函数名称：	Protocol_Parser_PublishRec
//
//	函数功能：	Publish Rec数据包协议解析输出打印
//
//	入口参数：	pro_buf：协议包
//
//	返回参数：	0-成功	其他-错误
//
//	说明：		
//==========================================================
static unsigned char Protocol_Parser_PublishRec(const unsigned char *pro_buf)
{

	unsigned char index = 0;						//index：pro_buf索引
	unsigned int temp = 0;							//临时保存一些需要计算的判断数据
	unsigned int len = 0;							//此包数据剩余长度(字节)
	
//--------------------------------------------------------------------------------------------------------------------------------------------
	temp = pro_buf[index] >> 4;						//消息类型在固定头部第一个字节高四位
	PRINT("第%d字节---[0x%02X]:[%s]Publish Rec\r\n", index, pro_buf[index], (temp == MQTT_PKT_PUBREC) ? "OK": "ERR");
	if(temp != MQTT_PKT_PUBREC)
		return 1;
	index++;
	
//--------------------------------------------------------------------------------------------------------------------------------------------
	temp = MQTT_ReadLength(pro_buf + index, 4, &len);
	PRINT("第%d~%d字节---%s:[%s]剩余消息长度---%d字节\r\n", index, index + temp - 1,
															print_hexstring(pro_buf + index, temp), (len > 1) ? "OK": "ERR", len);
	if(len <= 1 || temp == 0)
		return 2;
	index += temp;
	
//--------------------------------------------------------------------------------------------------------------------------------------------
	PRINT("第%d~%d字节---[0x%02X 0x%02X]:[OK]PKT ID\r\n", index, index + 1, pro_buf[index], pro_buf[index + 1]);
	
	return 0;

}

//==========================================================
//	函数名称：	Protocol_Parser_PublishRel
//
//	函数功能：	Publish Rel数据包协议解析输出打印
//
//	入口参数：	pro_buf：协议包
//
//	返回参数：	0-成功	其他-错误
//
//	说明：		
//==========================================================
static unsigned char Protocol_Parser_PublishRel(const unsigned char *pro_buf)
{

	unsigned char index = 0;						//index：pro_buf索引
	unsigned int temp = 0;							//临时保存一些需要计算的判断数据
	unsigned int len = 0;							//此包数据剩余长度(字节)
	
//--------------------------------------------------------------------------------------------------------------------------------------------
	temp = pro_buf[index] >> 4;						//消息类型在固定头部第一个字节高四位
	PRINT("第%d字节---[0x%02X]:[%s]Publish Rel\r\n", index, pro_buf[index], (temp == MQTT_PKT_PUBREL) ? "OK": "ERR");
	if(temp != MQTT_PKT_PUBREL)
		return 1;
	index++;
	
//--------------------------------------------------------------------------------------------------------------------------------------------
	temp = MQTT_ReadLength(pro_buf + index, 4, &len);
	PRINT("第%d~%d字节---%s:[%s]剩余消息长度---%d字节\r\n", index, index + temp - 1,
															print_hexstring(pro_buf + index, temp), (len > 1) ? "OK": "ERR", len);
	if(len <= 1 || temp == 0)
		return 2;
	index += temp;
	
//--------------------------------------------------------------------------------------------------------------------------------------------
	PRINT("第%d~%d字节---[0x%02X 0x%02X]:[OK]PKT ID\r\n", index, index + 1, pro_buf[index], pro_buf[index + 1]);
	
	return 0;

}

//==========================================================
//	函数名称：	Protocol_Parser_PublishComp
//
//	函数功能：	Publish Comp数据包协议解析输出打印
//
//	入口参数：	pro_buf：协议包
//
//	返回参数：	0-成功	其他-错误
//
//	说明：		
//==========================================================
static unsigned char Protocol_Parser_PublishComp(const unsigned char *pro_buf)
{

	unsigned char index = 0;						//index：pro_buf索引
	unsigned int temp = 0;							//临时保存一些需要计算的判断数据
	unsigned int len = 0;							//此包数据剩余长度(字节)
	
//--------------------------------------------------------------------------------------------------------------------------------------------
	temp = pro_buf[index] >> 4;						//消息类型在固定头部第一个字节高四位
	PRINT("第%d字节---[0x%02X]:[%s]Publish Comp\r\n", index, pro_buf[index], (temp == MQTT_PKT_PUBCOMP) ? "OK": "ERR");
	if(temp != MQTT_PKT_PUBCOMP)
		return 1;
	index++;
	
//--------------------------------------------------------------------------------------------------------------------------------------------
	temp = MQTT_ReadLength(pro_buf + index, 4, &len);
	PRINT("第%d~%d字节---%s:[%s]剩余消息长度---%d字节\r\n", index, index + temp - 1,
															print_hexstring(pro_buf + index, temp), (len > 1) ? "OK": "ERR", len);
	if(len <= 1 || temp == 0)
		return 2;
	index += temp;
	
//--------------------------------------------------------------------------------------------------------------------------------------------
	PRINT("第%d~%d字节---[0x%02X 0x%02X]:[OK]PKT ID\r\n", index, index + 1, pro_buf[index], pro_buf[index + 1]);
	
	return 0;

}

#endif

//==========================================================
//	函数名称：	Protocol_Parser_Print
//
//	函数功能：	协议解析输出打印
//
//	入口参数：	pro_buf：协议包
//
//	返回参数：	
//
//	说明：		
//==========================================================
unsigned char Protocol_Parser_Print(const unsigned char *pro_buf)
{
	
#if(SEND_PASER_EN == 1 || RECV_PASER_EN == 1)
	unsigned char result = 0;
#endif

	switch(pro_buf[0] >> 4)
	{
#if(SEND_PASER_EN == 1 || RECV_PASER_EN == 1)
		case MQTT_PKT_PUBLISH:
		
			PRINT("\r\n----------------------------------------------------------------------\r\n");
			PRINT("- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -\r\n");
			result = Protocol_Parser_Publish(pro_buf);
			if(result)
				PRINT("\r\n发布数据错误:%d\r\n", result);
			PRINT("- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -\r\n");
			PRINT("----------------------------------------------------------------------\r\n\r\n");
		
		break;
#endif
		
#if(SEND_PASER_EN == 1)
		case MQTT_PKT_CONNECT:
		
			PRINT("\r\n----------------------------------------------------------------------\r\n");
			PRINT("- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -\r\n");
			result = Protocol_Parser_Connect(pro_buf);
			if(result)
				PRINT("\r\n登录报文错误:%d\r\n", result);
			PRINT("- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -\r\n");
			PRINT("----------------------------------------------------------------------\r\n\r\n");
		
		break;
			
		case MQTT_PKT_DISCONNECT:
		
			PRINT("\r\n----------------------------------------------------------------------\r\n");
			PRINT("- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -\r\n");
			result = Protocol_Parser_DisConnect(pro_buf);
			if(result)
				PRINT("\r\n断开连接报文错误:%d\r\n", result);
			PRINT("- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -\r\n");
			PRINT("----------------------------------------------------------------------\r\n\r\n");
		
		break;
		
		case MQTT_PKT_SUBSCRIBE:
		
			PRINT("\r\n----------------------------------------------------------------------\r\n");
			PRINT("- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -\r\n");
			result = Protocol_Parser_Subscribe(pro_buf);
			if(result)
				PRINT("\r\n订阅消息错误:%d\r\n", result);
			PRINT("- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -\r\n");
			PRINT("----------------------------------------------------------------------\r\n\r\n");
		
		break;
		
		case MQTT_PKT_UNSUBSCRIBE:
		
			PRINT("\r\n----------------------------------------------------------------------\r\n");
			PRINT("- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -\r\n");
			result = Protocol_Parser_UnSubscribe(pro_buf);
			if(result)
				PRINT("\r\n取消订阅消息错误:%d\r\n", result);
			PRINT("- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -\r\n");
			PRINT("----------------------------------------------------------------------\r\n\r\n");
		
		break;
		
		case MQTT_PKT_PINGREQ:
		
			PRINT("\r\n----------------------------------------------------------------------\r\n");
			PRINT("- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -\r\n");
			result = Protocol_Parser_Ping(pro_buf);
			if(result)
				PRINT("\r\n心跳消息错误:%d\r\n", result);
			PRINT("- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -\r\n");
			PRINT("----------------------------------------------------------------------\r\n\r\n");
		
		break;
#endif

#if(RECV_PASER_EN == 1)
		case MQTT_PKT_CONNACK:
		
			PRINT("\r\n----------------------------------------------------------------------\r\n");
			PRINT("- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -\r\n");
			result = Protocol_Parser_ConnectResp(pro_buf);
			if(result)
				PRINT("\r\n连接回复错误:%d\r\n", result);
			PRINT("- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -\r\n");
			PRINT("----------------------------------------------------------------------\r\n\r\n");
		
		break;
		
		case MQTT_PKT_PINGRESP:
		
			PRINT("\r\n----------------------------------------------------------------------\r\n");
			PRINT("- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -\r\n");
			result = Protocol_Parser_PingResp(pro_buf);
			if(result)
				PRINT("\r\n心跳回复错误:%d\r\n", result);
			PRINT("- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -\r\n");
			PRINT("----------------------------------------------------------------------\r\n\r\n");
		
		break;
		
		case MQTT_PKT_PUBACK:
		
			PRINT("\r\n----------------------------------------------------------------------\r\n");
			PRINT("- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -\r\n");
			result = Protocol_Parser_PublishResp(pro_buf);
			if(result)
				PRINT("\r\n发布消息回复错误:%d\r\n", result);
			PRINT("- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -\r\n");
			PRINT("----------------------------------------------------------------------\r\n\r\n");
		
		break;
		
		case MQTT_PKT_SUBACK:
		
			PRINT("\r\n----------------------------------------------------------------------\r\n");
			PRINT("- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -\r\n");
			result = Protocol_Parser_SubscribeResp(pro_buf);
			if(result)
				PRINT("\r\n订阅回复错误:%d\r\n", result);
			PRINT("- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -\r\n");
			PRINT("----------------------------------------------------------------------\r\n\r\n");
		
		break;
		
		case MQTT_PKT_UNSUBACK:
		
			PRINT("\r\n----------------------------------------------------------------------\r\n");
			PRINT("- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -\r\n");
			result = Protocol_Parser_UnSubscribeResp(pro_buf);
			if(result)
				PRINT("\r\n取消订阅回复错误:%d\r\n", result);
			PRINT("- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -\r\n");
			PRINT("----------------------------------------------------------------------\r\n\r\n");
		
		break;
		
		case MQTT_PKT_PUBREC:
		
			PRINT("\r\n----------------------------------------------------------------------\r\n");
			PRINT("- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -\r\n");
			result = Protocol_Parser_PublishRec(pro_buf);
			if(result)
				PRINT("\r\nPublish Rec错误:%d\r\n", result);
			PRINT("- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -\r\n");
			PRINT("----------------------------------------------------------------------\r\n\r\n");
		
		break;
		
		case MQTT_PKT_PUBREL:
		
			PRINT("\r\n----------------------------------------------------------------------\r\n");
			PRINT("- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -\r\n");
			result = Protocol_Parser_PublishRel(pro_buf);
			if(result)
				PRINT("\r\nPublish Rel错误:%d\r\n", result);
			PRINT("- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -\r\n");
			PRINT("----------------------------------------------------------------------\r\n\r\n");
		
		break;
		
		case MQTT_PKT_PUBCOMP:
		
			PRINT("\r\n----------------------------------------------------------------------\r\n");
			PRINT("- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -\r\n");
			result = Protocol_Parser_PublishComp(pro_buf);
			if(result)
				PRINT("\r\nPublish Comp错误:%d\r\n", result);
			PRINT("- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -\r\n");
			PRINT("----------------------------------------------------------------------\r\n\r\n");
		
		break;
#endif
		
		default:
		break;
	}
	
	return 0;
}
