//单片机头文件
#include "stm32f10x.h"

//网络设备
#include "net_device.h"

//协议文件
#include "onenet.h"
#include "fault.h"
#include "mqttkit.h"

//协议分析
#include "protocol_parser.h"

//命令回调
#include "cmd_callback.h"

//硬件驱动
#include "usart.h"
#include "delay.h"

//C库
#include <string.h>
#include <stdio.h>
#include <stdlib.h>


//当正式环境注册码达到16个字符则启用自动创建功能，否则不启用
//如果要采用自动创建设备的方式，apikey必须为master-key，且正式环境注册码有效
ONETNET_INFO onenet_info = {"709577224", "Ro6M1s8mnUK=XUOAiW5WY99MS=Y=",
							"412306", "22414",
							"",
							"183.230.40.39", "6002",
							NULL, NULL, 0, 7, NULL,
							0, 0, 0, 1, 0, 0, 0, 0, 0, 0};


//==========================================================
//	函数名称：	OneNET_RepetitionCreateFlag
//
//	函数功能：	允许重复注册设备
//
//	入口参数：	apikey：必须是masterkey
//
//	返回参数：	0-成功		1-失败
//
//	说明：		允许重复注册，否则第一次创建成功之后，再次创建会失败
//==========================================================
_Bool OneNET_RepetitionCreateFlag(const char *apikey)
{

	_Bool result = 1;
	char send_buf[136];
	unsigned char time_out = 200;
	
	if(!net_device_info.net_work)
		return result;
	
	NET_DEVICE_Close();
	
	if(NET_DEVICE_Connect("TCP", "183.230.40.33", "80") == 0)
	{
		snprintf(send_buf, sizeof(send_buf), "PUT /register_attr HTTP/1.1\r\napi-key:%s\r\nHost:api.heclouds.com\r\n"
						"Content-Length:19\r\n\r\n"
						"{\"allow_dup\": true}", apikey);
		
		if(!NET_DEVICE_SendData((unsigned char *)send_buf, strlen(send_buf)))
		{
			net_device_info.cmd_ipd = NULL;
			
			while(--time_out)
			{
				if(net_device_info.cmd_ipd != NULL)
					break;
				
				DelayXms(10);
			}
			
			if(time_out)
			{
				if(strstr(net_device_info.cmd_ipd, "succ"))
				{
					UsartPrintf(USART_DEBUG, "Tips:	OneNET_RepetitionCreateFlag Ok\r\n");
					result = 0;
				}
				else if(strstr(net_device_info.cmd_ipd, "auth failed"))
				{
					UsartPrintf(USART_DEBUG, "WARN:	当前使用的不是masterkey 或 apikey错误\r\n");
				}
				else
					UsartPrintf(USART_DEBUG, "Tips:	OneNET_RepetitionCreateFlag Err\r\n");
			}
			else
				UsartPrintf(USART_DEBUG, "Tips:	OneNET_RepetitionCreateFlag Time Out\r\n");
		}
		
		NET_DEVICE_Close();
	}
	
	return result;

}

//==========================================================
//	函数名称：	OneNET_CreateDevice
//
//	函数功能：	在产品中创建一个设备
//
//	入口参数：	reg_code：正式环境注册码
//				dev_name：设备名
//				auth_info：鉴权信息
//				devid：保存返回的devid
//				apikey：保存返回的apikey
//
//	返回参数：	0-成功		1-失败
//
//	说明：		
//==========================================================
_Bool OneNET_CreateDevice(const char *reg_code, const char *dev_name, const char *auth_info, char *devid, char *apikey)
{

	_Bool result = 1;
	unsigned short send_len = 20 + strlen(dev_name) + strlen(auth_info);
	unsigned char time_out = 200;
	char *send_ptr = NULL, *data_ptr = NULL;
	
	if(!net_device_info.net_work)
		return result;
	
	send_ptr = NET_MallocBuffer(send_len + 140);
	if(send_ptr == NULL)
		return result;
	
	NET_DEVICE_Close();
	
	if(NET_DEVICE_Connect("TCP", "183.230.40.33", "80") == 0)
	{
		snprintf(send_ptr, 140 + send_len, "POST /register_de?register_code=%s HTTP/1.1\r\n"
						"Host: api.heclouds.com\r\n"
						"Content-Length:%d\r\n\r\n"
						"{\"sn\":\"%s\",\"title\":\"%s\"}",
		
						reg_code, send_len, auth_info, dev_name);
		
		if(!NET_DEVICE_SendData((unsigned char *)send_ptr, strlen(send_ptr)))
		{
			//{"device_id":"12345678","key":"gZrujZgtH3ivgNk33Qy3HgdTErg="}
			
			net_device_info.cmd_ipd = NULL;
			
			while(--time_out)
			{
				if(net_device_info.cmd_ipd != NULL)
					break;
				
				DelayXms(10);
			}
			
			if(time_out)
			{
				data_ptr = strstr(net_device_info.cmd_ipd, "device_id");
				
				if(strstr(net_device_info.cmd_ipd, "auth failed"))
				{
					UsartPrintf(USART_DEBUG, "WARN:	正式环境注册码错误\r\n");
				}
			}
			
			if(data_ptr)
			{
				if(sscanf(data_ptr, "device_id\":\"%[^\"]\",\"key\":\"%[^\"]\"", devid, apikey) == 2)
				{
					UsartPrintf(USART_DEBUG, "create device: %s, %s\r\n", devid, apikey);
					result = 0;
				}
			}
		}
		
		NET_DEVICE_Close();
	}
	
	NET_FreeBuffer(send_ptr);
	
	return result;

}

//==========================================================
//	函数名称：	OneNET_GetLinkIP
//
//	函数功能：	获取使用协议的登录IP和PORT
//
//	入口参数：	protocol：协议号
//				ip：保存返回IP的缓存区
//				port：保存返回port的缓存区
//
//	返回参数：	0-成功		1-失败
//
//	说明：		1-edp	2-nwx	3-jtext		4-Hiscmd
//				5-jt808			6-modbus	7-mqtt
//				8-gr20			9-reg		10-HTTP(自定义)
//				获取IP本身不支持HTTP协议，这里自定义一个标志
//==========================================================
_Bool OneNET_GetLinkIP(unsigned char protocol, char *ip, char *port)
{
	
	_Bool result = 1;
	char *data_ptr = NULL;
	char send_buf[128];
	unsigned char time_out = 200;
	
	if(!net_device_info.net_work)
		return result;
	
	if(onenet_info.get_ip)
		return !result;
	
	if(protocol == 10)													//如果是HTTP协议
	{
		strcpy(ip, "183.230.40.33");
		strcpy(port, "80");
		
		onenet_info.get_ip = 1;
		
		return !result;
	}
	
	NET_DEVICE_Close();
	
	if(NET_DEVICE_Connect("TCP", "183.230.40.33", "80") == 0)
	{
		memset(send_buf, 0, sizeof(send_buf));
		snprintf(send_buf, sizeof(send_buf), "GET http://api.heclouds.com/s?t=%d HTTP/1.1\r\n"
												"api-key:=sUT=jsLGXkQcUz3Z9EaiNQ80U0=\r\n"
												"Host:api.heclouds.com\r\n\r\n",
												protocol);
		
		net_device_info.cmd_ipd = NULL;
		
		NET_DEVICE_SendData((unsigned char *)send_buf, strlen(send_buf));
		
		while(--time_out)
		{
			if(net_device_info.cmd_ipd != NULL)
				break;
			
			DelayXms(10);
		}
		
		if(time_out)
		{
			data_ptr = strstr(net_device_info.cmd_ipd, "no-cache");				//找到最后的关键词
		}
		
		if(data_ptr != NULL)
		{
			if(strstr(data_ptr, "unsupportted") != NULL)						//不支持的协议类型
			{
				UsartPrintf(USART_DEBUG, "不支持该协议类型\r\n");
				
				onenet_info.get_ip = 1;
			}
			else if(strstr(data_ptr, "can't find a available") != NULL)			//不支持的协议类型
			{
				UsartPrintf(USART_DEBUG, "can't find a available IP\r\n");
				
				onenet_info.get_ip = 1;
			}
			else
			{
				if(sscanf(data_ptr, "no-cache\r\n%[^:]:%s", ip, port) == 2)
				{
					onenet_info.get_ip = 1;
					result = 0;
					
					UsartPrintf(USART_DEBUG, "Get ip: %s, port: %s\r\n", ip, port);
				}
			}
		}
		
		NET_DEVICE_Close();
	}
	
	return result;

}

//==========================================================
//	函数名称：	OneNET_GetLocation
//
//	函数功能：	获取LBS定位数据
//
//	入口参数：	device_id：设备ID
//				api_key：apikey
//				lon：缓存lon坐标
//				lat：缓存lat坐标
//
//	返回参数：	0-成功	其他错误码
//
//	说明：		
//==========================================================
unsigned char OneNET_GetLocation(const char *device_id, const char *api_key, char *lon, char *lat)
{
	
	unsigned char result = 255;

#if(LBS_EN == 1)
	char *data_ptr = NULL;
	char send_buf[128];
	unsigned char time_out = 200;
	
	if(!net_device_info.net_work)
		return result;
	
	if(strlen(device_id) < 6 || strlen(api_key) != 28)
		return result;
	
	if(lon == NULL || lat == NULL)
		return result;
	
	NET_DEVICE_Close();
	
	onenet_info.connect_ip = 0;
	onenet_info.net_work = 0;
	
	if(NET_DEVICE_Connect("TCP", "183.230.40.33", "80") == 0)
	{
		memset(send_buf, 0, sizeof(send_buf));
		snprintf(send_buf, sizeof(send_buf), "GET /devices/%s/lbs/latestLocation HTTP/1.1\r\n"
												"api-key:%s\r\n"
												"Host:api.heclouds.com\r\n\r\n",
												device_id, api_key);
		
		net_device_info.cmd_ipd = NULL;
		
		NET_DEVICE_SendData((unsigned char *)send_buf, strlen(send_buf));
		
		while(--time_out)
		{
			if(net_device_info.cmd_ipd != NULL)
				break;
			
			DelayXms(15);
		}
		
		//{"errno":0,"data":{"at":"2018-12-07 15:28:05.348","accuracy":696,"lon":106.xxx,"lat":29.xxx},"error":"succ"}
		
		if(time_out)
		{
			data_ptr = strstr(net_device_info.cmd_ipd, "\"lon\":");
		}
		
		if(data_ptr != NULL)
		{
			unsigned char i = 0;
			
			data_ptr = strchr(data_ptr, ':');
			
			data_ptr++;
			while(*data_ptr != ',')
				lon[i++] = *data_ptr++;
			
			i = 0;
			data_ptr += 7;
			while(*data_ptr != '}')
				lat[i++] = *data_ptr++;
			
			UsartPrintf(USART_DEBUG, "lon:%s, lat: %s\r\n", lon, lat);
			
			onenet_info.lbs = 1;
			
			result = 0;
		}
		else
		{
			if(++onenet_info.lbs_count >= 4)
				UsartPrintf(USART_DEBUG, "WARN:	请检查产品是否开通位置能力!!!\r\n");
		}
		
		NET_DEVICE_Close();
	}
#endif
	
	return result;

}

//==========================================================
//	函数名称：	OneNET_ConnectIP
//
//	函数功能：	连接IP
//
//	入口参数：	ip：IP地址缓存指针
//				port：端口缓存指针
//
//	返回参数：	返回初始化结果
//
//	说明：		0-成功		1-失败
//==========================================================
_Bool OneNET_ConnectIP(char *ip, char *port)
{

	_Bool result = 1;
	
	if(!net_device_info.net_work)									//网络模组具备网络连接能力
		return result;
	
	if(onenet_info.connect_ip)										//已经连接了IP
		return !result;
	
	if(!NET_DEVICE_Connect("TCP", ip, port))
	{
		result = 0;
		net_fault_info.net_fault_count = 0;
		onenet_info.connect_ip = 1;
	}
	else
	{
		DelayXms(500);
		
		if(++onenet_info.err_count >= 5)
		{
			onenet_info.err_count = 0;
			UsartPrintf(USART_DEBUG, "请检查IP地址和PORT是否正确\r\n");
		}
	}
	
	return result;

}

//==========================================================
//	函数名称：	OneNET_DevLink
//
//	函数功能：	与onenet创建连接
//
//	入口参数：	devid：创建设备的devid
//				proid：产品ID
//				auth_key：创建设备的masterKey或apiKey或设备鉴权信息
//
//	返回参数：	无
//
//	说明：		与onenet平台建立连接，成功或会标记oneNetInfo.netWork网络状态标志
//==========================================================
void OneNET_DevLink(const char* devid, const char *proid, const char* auth_info)
{
	
	MQTT_PACKET_STRUCTURE mqtt_packet = {NULL, 0, 0, 0};					//协议包
	
	unsigned char time_out = 200;
	
	UsartPrintf(USART_DEBUG, "OneNET_DevLink\r\n"
							"PROID: %s,	AUIF: %s,	DEVID:%s\r\n"
                        , proid, auth_info, devid);
	
	if(MQTT_PacketConnect(proid, auth_info, devid, 256, 0, MQTT_QOS_LEVEL0, NULL, NULL, 0, &mqtt_packet) == 0)
	{
		Protocol_Parser_Print(mqtt_packet._data);
		
		NET_DEVICE_SendData(mqtt_packet._data, mqtt_packet._len);			//上传平台
		//NET_DEVICE_AddDataSendList(mqtt_packet._data, mqtt_packet._len, 1);
		
		MQTT_DeleteBuffer(&mqtt_packet);									//删包
		
		while(--time_out)
		{
			if(onenet_info.cmd_ptr != NULL)
			{
				OneNET_RevPro(onenet_info.cmd_ptr);
				
				onenet_info.cmd_ptr = NULL;
				
				break;
			}
			
			DelayXms(10);
		}
	}
	else
		UsartPrintf(USART_DEBUG, "WARN:	MQTT_PacketConnect Failed\r\n");
	
	if(onenet_info.net_work)											//如果接入成功
	{
		onenet_info.err_count = 0;
	}
	else
	{
		if(++onenet_info.err_count >= 5)								//如果超过设定次数后，还未接入平台
		{
			onenet_info.net_work = 0;
			onenet_info.err_count = 0;
			
			onenet_info.err_check = 1;
		}
	}
	
}

//==========================================================
//	函数名称：	OneNET_DisConnect
//
//	函数功能：	与平台断开连接
//
//	入口参数：	无
//
//	返回参数：	0-成功		1-失败
//
//	说明：		
//==========================================================
_Bool OneNET_DisConnect(void)
{

	MQTT_PACKET_STRUCTURE mqtt_packet = {NULL, 0, 0, 0};						//协议包

	if(!onenet_info.net_work)
		return 1;
	
	if(MQTT_PacketDisConnect(&mqtt_packet) == 0)
	{
		Protocol_Parser_Print(mqtt_packet._data);
		
		//NET_DEVICE_SendData(mqtt_packet._data, mqtt_packet._len);				//向平台发送订阅请求
		NET_DEVICE_AddDataSendList(mqtt_packet._data, mqtt_packet._len, 1);		//加入链表
		
		MQTT_DeleteBuffer(&mqtt_packet);										//删包
	}
	
	return 0;

}

//==========================================================
//	函数名称：	OneNET_SendData
//
//	函数功能：	上传数据到平台
//
//	入口参数：	type：发送数据的格式
//				devid：设备ID
//				apikey：设备apikey
//				streamArray：数据流
//				streamArrayNum：数据流个数
//
//	返回参数：	SEND_TYPE_OK-发送成功	SEND_TYPE_DATA-需要重送
//
//	说明：		
//==========================================================
unsigned char OneNET_SendData(FORMAT_TYPE type, char *devid, char *apikey, DATA_STREAM *streamArray, unsigned short streamArrayCnt)
{
	
	MQTT_PACKET_STRUCTURE mqtt_packet = {NULL, 0, 0, 0};										//协议包
	
	_Bool status = SEND_TYPE_OK;
	short body_len = 0;
	
	if(!onenet_info.net_work)
		return SEND_TYPE_DATA;
	
	UsartPrintf(USART_DEBUG, "Tips:	OneNET_SendData-MQTT_TYPE%d\r\n", type);
	
	body_len = DSTREAM_GetDataStream_Body_Measure(type, streamArray, streamArrayCnt, 0);		//获取当前需要发送的数据流的总长度
	if(body_len > 0)
	{
		if(MQTT_PacketSaveData(devid, body_len, NULL, (uint8)type, &mqtt_packet) == 0)
		{
			body_len = DSTREAM_GetDataStream_Body(type, streamArray, streamArrayCnt, mqtt_packet._data, mqtt_packet._size, mqtt_packet._len);
			
			if(body_len > 0)
			{
				mqtt_packet._len += body_len;
				UsartPrintf(USART_DEBUG, "Send %d Bytes\r\n", mqtt_packet._len);
				
				Protocol_Parser_Print(mqtt_packet._data);
				
				//NET_DEVICE_SendData(mqtt_packet._data, mqtt_packet._len);						//上传数据到平台
				NET_DEVICE_AddDataSendList(mqtt_packet._data, mqtt_packet._len, 1);				//加入链表
			}
			else
				UsartPrintf(USART_DEBUG, "WARN:	DSTREAM_GetDataStream_Body Failed\r\n");
				
			MQTT_DeleteBuffer(&mqtt_packet);													//删包
		}
		else
			UsartPrintf(USART_DEBUG, "WARN:	MQTT_NewBuffer Failed\r\n");
	}
	else if(body_len < 0)
		status = SEND_TYPE_OK;
	else
		status = SEND_TYPE_DATA;
	
	net_fault_info.net_fault_level_r = NET_FAULT_LEVEL_0;										//发送之后清除标记
	
	return status;
	
}

//==========================================================
//	函数名称：	OneNET_Send_BinFile
//
//	函数功能：	上传二进制文件到平台
//
//	入口参数：	name：数据流名
//				file：文件
//				file_size：文件长度
//
//	返回参数：	SEND_TYPE_OK-发送成功	SEND_TYPE_BINFILE-需要重送
//
//	说明：		
//==========================================================
#define PKT_SIZE 1024
unsigned char OneNET_Send_BinFile(char *name, const unsigned char *file, unsigned int file_size)
{
	
	MQTT_PACKET_STRUCTURE mqtt_packet = {NULL, 0, 0, 0};					//协议包
	
	unsigned char status = SEND_TYPE_BINFILE;

	char *type_bin_head = NULL;												//图片数据头
	unsigned char *file_t = (unsigned char *)file;
	
	if(name == NULL || file == NULL || file_size == 0)
		return status;
	
	if(!onenet_info.net_work)
		return SEND_TYPE_BINFILE;
	
	type_bin_head = (char *)NET_MallocBuffer(13 + strlen(name));
	if(type_bin_head == NULL)
		return status;
	
	sprintf(type_bin_head, "{\"ds_id\":\"%s\"}", name);
	
	if(MQTT_PacketSaveBinData(name, file_size, &mqtt_packet) == 0)
	{
		UsartPrintf(USART_DEBUG, "Send %d Bytes\r\n", mqtt_packet._len);
		
		Protocol_Parser_Print(mqtt_packet._data);
		
		NET_DEVICE_SendData(mqtt_packet._data, mqtt_packet._len);			//上传数据到平台
		//NET_DEVICE_AddDataSendList(edp_packet._data, edp_packet._len, 1);	//加入链表
		
		MQTT_DeleteBuffer(&mqtt_packet);									//删包
		
		UsartPrintf(USART_DEBUG, "Image Len = %d\r\n", file_size);
		
		while(file_size > 0)
		{
			DelayXms(net_device_info.send_time);
			UsartPrintf(USART_DEBUG, "Image Reamin %d Bytes\r\n", file_size);
			
			if(file_size >= PKT_SIZE)
			{
				if(!NET_DEVICE_SendData(file_t, PKT_SIZE))					//串口发送分片
				{
					file_t += PKT_SIZE;
					file_size -= PKT_SIZE;
				}
			}
			else
			{
				if(!NET_DEVICE_SendData(file_t, (unsigned short)file_size))	//串口发送最后一个分片
					file_size = 0;
			}
		}
		
		UsartPrintf(USART_DEBUG, "Tips:	Image Send Ok\r\n");
		
		status = SEND_TYPE_OK;
	}
	else
		UsartPrintf(USART_DEBUG, "MQTT_PacketSaveData Failed\r\n");
	
	NET_FreeBuffer(type_bin_head);
	
	return status;

}

//==========================================================
//	函数名称：	OneNET_SendData_Heart
//
//	函数功能：	心跳检测
//
//	入口参数：	无
//
//	返回参数：	SEND_TYPE_OK-发送成功	SEND_TYPE_DATA-需要重送
//
//	说明：		
//==========================================================
unsigned char OneNET_SendData_Heart(void)
{
	
	MQTT_PACKET_STRUCTURE mqtt_packet = {NULL, 0, 0, 0};				//协议包
	
	if(!onenet_info.net_work)											//如果网络为连接
		return SEND_TYPE_HEART;
	
	if(MQTT_PacketPing(&mqtt_packet))
		return SEND_TYPE_HEART;
	
	onenet_info.heart_beat = 0;
	
	Protocol_Parser_Print(mqtt_packet._data);
	
	//NET_DEVICE_SendData(mqtt_packet._data, mqtt_packet._len);			//向平台上传心跳请求
	NET_DEVICE_AddDataSendList(mqtt_packet._data, mqtt_packet._len, 0);	//加入链表
	
	MQTT_DeleteBuffer(&mqtt_packet);									//删包
	
	return SEND_TYPE_OK;

}

//==========================================================
//	函数名称：	OneNET_Check_Heart
//
//	函数功能：	发送心跳后的心跳检测
//
//	入口参数：	无
//
//	返回参数：	0-成功	1-等待
//
//	说明：		基于调用时基，runCount每隔此函数调用一次的时间自增
//				达到设定上限检测心跳标志位是否就绪
//				上限时间可以不用太精确
//==========================================================
_Bool OneNET_Check_Heart(void)
{
	
	static unsigned char runCount = 0;
	
	if(!onenet_info.net_work)
		return 1;

	if(onenet_info.heart_beat == 1)
	{
		runCount = 0;
		onenet_info.err_count = 0;
		
		return 0;
	}
	
	if(++runCount >= 40)
	{
		runCount = 0;
		
		UsartPrintf(USART_DEBUG, "HeartBeat TimeOut: %d\r\n", onenet_info.err_count);
		onenet_info.send_data = SEND_TYPE_HEART;		//发送心跳请求
		
		if(++onenet_info.err_count >= 3)
		{
			onenet_info.err_count = 0;
			
			onenet_info.err_check = 1;
		}
	}
	
	return 1;

}

//==========================================================
//	函数名称：	OneNET_Publish
//
//	函数功能：	发布消息
//
//	入口参数：	topic：发布的主题
//				msg：消息内容
//
//	返回参数：	SEND_TYPE_OK-成功	SEND_TYPE_PUBLISH-需要重送
//
//	说明：		
//==========================================================
unsigned char OneNET_Publish(const char *topic, const char *msg)
{

	MQTT_PACKET_STRUCTURE mqtt_packet = {NULL, 0, 0, 0};						//协议包

	if(!onenet_info.net_work)
		return SEND_TYPE_PUBLISH;
	
	UsartPrintf(USART_DEBUG, "Publish Topic: %s, Msg: %s\r\n", topic, msg);
	
	if(MQTT_PacketPublish(MQTT_PUBLISH_ID, topic, msg, strlen(msg), MQTT_QOS_LEVEL2, 0, 1, &mqtt_packet) == 0)
	{
		Protocol_Parser_Print(mqtt_packet._data);
		
		//NET_DEVICE_SendData(mqtt_packet._data, mqtt_packet._len);				//向平台发送订阅请求
		NET_DEVICE_AddDataSendList(mqtt_packet._data, mqtt_packet._len, 1);		//加入链表
		
		MQTT_DeleteBuffer(&mqtt_packet);										//删包
	}
	
	return SEND_TYPE_OK;

}

//==========================================================
//	函数名称：	OneNET_Subscribe
//
//	函数功能：	订阅
//
//	入口参数：	topics：订阅的topic
//				topic_cnt：topic个数
//
//	返回参数：	SEND_TYPE_OK-成功	SEND_TYPE_SUBSCRIBE-需要重发
//
//	说明：		
//==========================================================
unsigned char OneNET_Subscribe(const char *topics[], unsigned char topic_cnt)
{
	
	unsigned char i = 0;
	
	MQTT_PACKET_STRUCTURE mqtt_packet = {NULL, 0, 0, 0};						//协议包

	if(!onenet_info.net_work)
		return SEND_TYPE_SUBSCRIBE;
	
	for(; i < topic_cnt; i++)
		UsartPrintf(USART_DEBUG, "Subscribe Topic: %s\r\n", topics[i]);
	
	if(MQTT_PacketSubscribe(MQTT_SUBSCRIBE_ID, MQTT_QOS_LEVEL2, topics, topic_cnt, &mqtt_packet) == 0)
	{
		Protocol_Parser_Print(mqtt_packet._data);
		
		//NET_DEVICE_SendData(mqtt_packet._data, mqtt_packet._len);				//向平台发送订阅请求
		NET_DEVICE_AddDataSendList(mqtt_packet._data, mqtt_packet._len, 1);		//加入链表
		
		MQTT_DeleteBuffer(&mqtt_packet);										//删包
	}
	
	return SEND_TYPE_OK;

}

//==========================================================
//	函数名称：	OneNET_UnSubscribe
//
//	函数功能：	取消订阅
//
//	入口参数：	topics：订阅的topic
//				topic_cnt：topic个数
//
//	返回参数：	SEND_TYPE_OK-发送成功	SEND_TYPE_UNSUBSCRIBE-需要重发
//
//	说明：		
//==========================================================
unsigned char OneNET_UnSubscribe(const char *topics[], unsigned char topic_cnt)
{
	
	unsigned char i = 0;
	
	MQTT_PACKET_STRUCTURE mqtt_packet = {NULL, 0, 0, 0};							//协议包

	if(!onenet_info.net_work)
		return SEND_TYPE_UNSUBSCRIBE;
	
	for(; i < topic_cnt; i++)
		UsartPrintf(USART_DEBUG, "UnSubscribe Topic: %s\r\n", topics[i]);
	
	if(MQTT_PacketUnSubscribe(MQTT_UNSUBSCRIBE_ID, topics, topic_cnt, &mqtt_packet) == 0)
	{
		Protocol_Parser_Print(mqtt_packet._data);
		
		//NET_DEVICE_SendData(mqtt_packet._data, mqtt_packet._len);				//向平台发送取消订阅请求
		NET_DEVICE_AddDataSendList(mqtt_packet._data, mqtt_packet._len, 1);		//加入链表
		
		MQTT_DeleteBuffer(&mqtt_packet);										//删包
	}
	
	return SEND_TYPE_OK;

}

//==========================================================
//	函数名称：	OneNET_CmdHandle
//
//	函数功能：	读取平台rb中的数据
//
//	入口参数：	无
//
//	返回参数：	无
//
//	说明：		
//==========================================================
void OneNET_CmdHandle(void)
{
	
	unsigned char *dataPtr = NULL, *ipdPtr = NULL;		//数据指针

	dataPtr = NET_DEVICE_Read();						//等待数据

	if(dataPtr != NULL)									//数据有效
	{
		ipdPtr = NET_DEVICE_GetIPD(dataPtr);			//检查是否是平台数据
		if(ipdPtr != NULL)
		{
			net_device_info.send_count = 0;
			
			if(onenet_info.connect_ip)
				onenet_info.cmd_ptr = ipdPtr;			//集中处理
			
			net_device_info.cmd_ipd = (char *)ipdPtr;
		}
		else
		{
			if(strstr((char *)dataPtr, "SEND OK") != NULL)
			{
				net_device_info.send_count = 0;
			}
			else if(strstr((char *)dataPtr, "CLOSE") != NULL && onenet_info.net_work)
			{
				UsartPrintf(USART_DEBUG, "WARN:	连接断开，准备重连\r\n");
				
				onenet_info.err_check = 1;
			}
			else
				NET_DEVICE_CmdHandle((char *)dataPtr);
		} 
	}

}

//==========================================================
//	函数名称：	OneNET_RevPro
//
//	函数功能：	平台返回数据检测
//
//	入口参数：	dataPtr：平台返回的数据
//
//	返回参数：	无
//
//	说明：		
//==========================================================
void OneNET_RevPro(unsigned char *cmd)
{
	
	MQTT_PACKET_STRUCTURE mqtt_packet = {NULL, 0, 0, 0};							//协议包
	
	char *req_payload = NULL;
	char *cmdid_topic = NULL;
	
	unsigned short topic_len = 0;
	unsigned short req_len = 0;
	
	unsigned char qos = 0;
	static unsigned short pkt_id = 0;
	
	Protocol_Parser_Print(cmd);
	
	switch(MQTT_UnPacketRecv(cmd))
	{
		case MQTT_PKT_CONNACK:
		
			switch(MQTT_UnPacketConnectAck(cmd))
			{
				case 0:
					UsartPrintf(USART_DEBUG, "Tips:	连接成功\r\n");
					onenet_info.net_work = 1;
				break;
				
				case 1:UsartPrintf(USART_DEBUG, "WARN:	连接失败：协议错误\r\n");break;
				case 2:UsartPrintf(USART_DEBUG, "WARN:	连接失败：非法的clientid\r\n");break;
				case 3:UsartPrintf(USART_DEBUG, "WARN:	连接失败：服务器失败\r\n");break;
				case 4:UsartPrintf(USART_DEBUG, "WARN:	连接失败：用户名或密码错误\r\n");break;
				case 5:UsartPrintf(USART_DEBUG, "WARN:	连接失败：非法链接(比如token非法)\r\n");break;
				
				default:UsartPrintf(USART_DEBUG, "ERR:	连接失败：未知错误\r\n");break;
			}
		
		break;
		
		case MQTT_PKT_PINGRESP:
		
			UsartPrintf(USART_DEBUG, "Tips:	HeartBeat OK\r\n");
			onenet_info.heart_beat = 1;
		
		break;
		
		case MQTT_PKT_CMD:																//命令下发
			
			if(MQTT_UnPacketCmd(cmd, &cmdid_topic, &req_payload, &req_len) == 0)		//解出topic和消息体
			{
				UsartPrintf(USART_DEBUG, "cmdid: %s, req: %s, req_len: %d\r\n", cmdid_topic, req_payload, req_len);
				
				//执行命令回调------------------------------------------------------------
				CALLBACK_Execute(req_payload);
				
				if(MQTT_PacketCmdResp(cmdid_topic, req_payload, &mqtt_packet) == 0)		//命令回复组包
				{
					UsartPrintf(USART_DEBUG, "Tips:	Send CmdResp\r\n");
					
					Protocol_Parser_Print(mqtt_packet._data);
					
					//NET_DEVICE_SendData(mqtt_packet._data, mqtt_packet._len);			//回复命令
					NET_DEVICE_AddDataSendList(mqtt_packet._data, mqtt_packet._len, 1);	//加入链表
					MQTT_DeleteBuffer(&mqtt_packet);									//删包
				}
				
				MQTT_FreeBuffer(cmdid_topic);
				MQTT_FreeBuffer(req_payload);
				onenet_info.send_data = SEND_TYPE_DATA;
			}
		
		break;
			
		case MQTT_PKT_PUBLISH:															//接收的Publish消息
		
			if(MQTT_UnPacketPublish(cmd, &cmdid_topic, &topic_len, &req_payload, &req_len, &qos, &pkt_id) == 0)
			{
				UsartPrintf(USART_DEBUG, "topic: %s, topic_len: %d, payload: %s, payload_len: %d\r\n",
																	cmdid_topic, topic_len, req_payload, req_len);
				
				//执行命令回调------------------------------------------------------------
				CALLBACK_Execute(req_payload);
				
				switch(qos)
				{
					case 1:																//收到publish的qos为1，设备需要回复Ack
					
						if(MQTT_PacketPublishAck(pkt_id, &mqtt_packet) == 0)
						{
							UsartPrintf(USART_DEBUG, "Tips:	Send PublishAck\r\n");
							
							Protocol_Parser_Print(mqtt_packet._data);
							
							//NET_DEVICE_SendData(mqtt_packet._data, mqtt_packet._len);
							NET_DEVICE_AddDataSendList(mqtt_packet._data, mqtt_packet._len, 1);//加入链表
							MQTT_DeleteBuffer(&mqtt_packet);
						}
					
					break;
					
					case 2:																//收到publish的qos为2，设备先回复Rec
																						//平台回复Rel，设备再回复Comp
						if(MQTT_PacketPublishRec(pkt_id, &mqtt_packet) == 0)
						{
							UsartPrintf(USART_DEBUG, "Tips:	Send PublishRec\r\n");
							
							Protocol_Parser_Print(mqtt_packet._data);
							
							//NET_DEVICE_SendData(mqtt_packet._data, mqtt_packet._len);
							NET_DEVICE_AddDataSendList(mqtt_packet._data, mqtt_packet._len, 1);//加入链表
							MQTT_DeleteBuffer(&mqtt_packet);
						}
					
					break;
					
					default:
						break;
				}
				
				MQTT_FreeBuffer(cmdid_topic);
				MQTT_FreeBuffer(req_payload);
				onenet_info.send_data = SEND_TYPE_DATA;
			}
		
		break;
			
		case MQTT_PKT_PUBACK:															//发送Publish消息，平台回复的Ack
		
			if(MQTT_UnPacketPublishAck(cmd) == 0)
			{
				UsartPrintf(USART_DEBUG, "Tips:	MQTT Publish Send OK\r\n");
				
#if(LBS_EN == 1)
				if(lbs_info.lbs_ok == 1 && !onenet_info.lbs && onenet_info.lbs_count < 4)	//如果获取到了基站信息 且 未获取到位置坐标 且 获取次数小于一定值
					onenet_info.net_work = 0;												//则重新获取一下位置信息
#endif
			}
			
		break;
			
		case MQTT_PKT_PUBREC:															//发送Publish消息，平台回复的Rec，设备需回复Rel消息
		
			if(MQTT_UnPacketPublishRec(cmd) == 0)
			{
				UsartPrintf(USART_DEBUG, "Tips:	Rev PublishRec\r\n");
				if(MQTT_PacketPublishRel(MQTT_PUBLISH_ID, &mqtt_packet) == 0)
				{
					UsartPrintf(USART_DEBUG, "Tips:	Send PublishRel\r\n");
					
					Protocol_Parser_Print(mqtt_packet._data);
					
					//NET_DEVICE_SendData(mqtt_packet._data, mqtt_packet._len);
					NET_DEVICE_AddDataSendList(mqtt_packet._data, mqtt_packet._len, 1);	//加入链表
					MQTT_DeleteBuffer(&mqtt_packet);
				}
			}
		
		break;
			
		case MQTT_PKT_PUBREL:															//收到Publish消息，设备回复Rec后，平台回复的Rel，设备需再回复Comp
			
			if(MQTT_UnPacketPublishRel(cmd, pkt_id) == 0)
			{
				UsartPrintf(USART_DEBUG, "Tips:	Rev PublishRel\r\n");
				if(MQTT_PacketPublishComp(pkt_id, &mqtt_packet) == 0)
				{
					UsartPrintf(USART_DEBUG, "Tips:	Send PublishComp\r\n");
					
					Protocol_Parser_Print(mqtt_packet._data);
					
					//NET_DEVICE_SendData(mqtt_packet._data, mqtt_packet._len);
					NET_DEVICE_AddDataSendList(mqtt_packet._data, mqtt_packet._len, 1);	//加入链表
					MQTT_DeleteBuffer(&mqtt_packet);
				}
			}
		
		break;
		
		case MQTT_PKT_PUBCOMP:															//发送Publish消息，平台返回Rec，设备回复Rel，平台再返回的Comp
		
			if(MQTT_UnPacketPublishComp(cmd) == 0)
			{
				UsartPrintf(USART_DEBUG, "Tips:	Rev PublishComp\r\n");
			}
		
		break;
			
		case MQTT_PKT_SUBACK:															//发送Subscribe消息的Ack
		
			if(MQTT_UnPacketSubscribe(cmd) == 0)
				UsartPrintf(USART_DEBUG, "Tips:	MQTT Subscribe OK\r\n");
			else
				UsartPrintf(USART_DEBUG, "Tips:	MQTT Subscribe Err\r\n");
		
		break;
			
		case MQTT_PKT_UNSUBACK:															//发送UnSubscribe消息的Ack
		
			if(MQTT_UnPacketUnSubscribe(cmd) == 0)
				UsartPrintf(USART_DEBUG, "Tips:	MQTT UnSubscribe OK\r\n");
			else
				UsartPrintf(USART_DEBUG, "Tips:	MQTT UnSubscribe Err\r\n");
		
		break;
		
		default:
			
		break;
	}

}
