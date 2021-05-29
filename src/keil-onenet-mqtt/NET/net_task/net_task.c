/**
	************************************************************
	************************************************************
	************************************************************
	*	文件名： 	net_task.c
	*
	*	作者： 		张继瑞
	*
	*	日期： 		2018-03-29
	*
	*	版本： 		V1.0
	*
	*	说明： 		网络相关任务
	*
	*	修改记录：	
	************************************************************
	************************************************************
	************************************************************
**/

//框架
#include "framework.h"

//网络设备
#include "net_device.h"

//网络任务
#include "net_task.h"

//协议
#include "onenet.h"
#include "fault.h"

//驱动
#include "delay.h"

//C库
#include <stdio.h>
#include <string.h>


#define NET_COUNT	5			//错误计数

#define NET_TIME	80			//设定时间--单位秒

unsigned short timer_count = 0;	//时间计数--单位秒


const char *topics[] = {"kyLin_topic", "topic_test"};


extern DATA_STREAM data_stream[];
extern unsigned char data_stream_cnt;


/*
************************************************************
*	函数名称：	NET_Event_CallBack
*
*	函数功能：	网络事件回调
*
*	入口参数：	net_event：事件类型
*
*	返回参数：	无
*
*	说明：		
************************************************************
*/
__weak void NET_Event_CallBack(NET_EVENT net_event)
{

	switch((unsigned char)net_event)
	{
		case NET_EVENT_Timer_Check_Err:		//网络定时检查超时错误
		break;
		
		case NET_EVENT_Timer_Send_Err:		//网络发送失败错误
		break;
		
		case NET_EVENT_Send_HeartBeat:		//即将发送心跳包
		break;
		
		case NET_EVENT_Send_Data:			//即将发送数据点
		break;
		
		case NET_EVENT_Send_Subscribe:		//即将发送订阅数据
		break;
		
		case NET_EVENT_Send_UnSubscribe:	//即将发送取消订阅数据
		break;
		
		case NET_EVENT_Send_Publish:		//即将发送推送数据
		break;
		
		case NET_EVENT_Send:				//开始发送数据
		break;
		
		case NET_EVENT_Recv:				//Modbus用-收到数据查询指令
		break;
		
		case NET_EVENT_Check_Status:		//进入网络模组状态检查
		break;
		
		case NET_EVENT_Device_Ok:			//网络模组检测Ok
		break;
		case NET_EVENT_Device_Err:			//网络模组检测错误
		break;
		
		case NET_EVENT_Initialize:			//正在初始化网络模组
		break;
		
		case NET_EVENT_Init_Ok:				//网络模组初始化成功
		break;
		
		case NET_EVENT_Auto_Create_Ok:		//自动创建设备成功
		break;
		
		case NET_EVENT_Auto_Create_Err:		//自动创建设备失败
		break;
		
		case NET_EVENT_Connect:				//正在连接、登录OneNET
		break;
		
		case NET_EVENT_Connect_Ok:			//连接、登录成功
		break;
		
		case NET_EVENT_Connect_Err:			//连接、登录错误
		break;
		
		case NET_EVENT_Fault_Process:		//错误处理
		break;
		
		default:							//无
		break;
	}

}

//==========================================================
//	函数名称：	NET_Task_GetMcuSerial
//
//	函数功能：	获取MCU ID
//
//	入口参数：	serial：指针地址
//
//	返回参数：	0-成功		1-失败
//
//	说明：		
//==========================================================
static _Bool NET_Task_GetMcuSerial(char **serial)
{
	
	_Bool result = 1;
	
	*serial = (char *)NET_MallocBuffer(16);
	
	if(*serial)
	{
		unsigned int mcu_id = 0;
		
		mcu_id = *(volatile unsigned int *)( 0x1FFFF7F0 );
		
		snprintf(*serial, 16, "mcu_id_%x", mcu_id);
		
		result = 0;
	}
	
	return result;

}

//==========================================================
//	函数名称：	NET_Task_ErrCheck
//
//	函数功能：	获取网络模块状态决定错误等级
//
//	入口参数：	无
//
//	返回参数：	无
//
//	说明：		
//==========================================================
void NET_Task_ErrCheck(void)
{
	
	unsigned char err_type = 0;

	err_type = NET_DEVICE_Check();												//网络设备状态检查
	if(err_type == NET_DEVICE_CONNECTED || err_type == NET_DEVICE_CLOSED || err_type == NET_DEVICE_GOT_IP)
	{
		net_fault_info.net_fault_level = net_fault_info.net_fault_level_r = NET_FAULT_LEVEL_1;
	}
	else if(err_type == NET_DEVICE_NO_DEVICE || err_type == NET_DEVICE_NO_CARD || err_type == NET_DEVICE_INITIAL)
	{
		net_fault_info.net_fault_level = net_fault_info.net_fault_level_r = NET_FAULT_LEVEL_3;
	}
	else																		//NET_DEVICE_CONNECTING、NET_DEVICE_BUSY
	{
		net_fault_info.net_fault_level = net_fault_info.net_fault_level_r = NET_FAULT_LEVEL_0;
	}

}

/*
************************************************************
*	函数名称：	NET_Timer
*
*	函数功能：	定时检查网络状态标志位
*
*	入口参数：	无
*
*	返回参数：	无
*
*	说明：		定时器任务。定时检查网络状态，若持续超过设定时间无网络连接，则进行平台重连
************************************************************
*/
void NET_Timer(void)
{
	
	if(onenet_info.net_work == 0)										//如果在规定时间内网络还未接入成功
	{
		if(++timer_count >= NET_TIME) 									//如果网络断开超时
		{
			NET_Event_CallBack(NET_EVENT_Timer_Check_Err);
			
			timer_count = 0;
			
			onenet_info.err_check = 1;
		}
	}
	else
	{
		timer_count = 0;												//清除计数
		
		if(net_device_info.send_count >= NET_COUNT)						//如果发送错误计数达到NET_COUNT次
		{
			NET_Event_CallBack(NET_EVENT_Timer_Send_Err);
		
			net_device_info.send_count = 0;
			
			onenet_info.err_check = 1;
		}
	}

}

/*
************************************************************
*	函数名称：	RECV_Task
*
*	函数功能：	处理平台下发的命令
*
*	入口参数：	无
*
*	返回参数：	无
*
*	说明：		平台下发的命令并解析、处理
************************************************************
*/
void RECV_Task(void)
{

	if(onenet_info.cmd_ptr)
	{
		OneNET_RevPro(onenet_info.cmd_ptr);
		
		onenet_info.cmd_ptr = NULL;
	}

}

/*
************************************************************
*	函数名称：	NET_FLAG_Task
*
*	函数功能：	网络相关标志
*
*	入口参数：	无
*
*	返回参数：	无
*
*	说明：		
************************************************************
*/
void NET_FLAG_Task(void)
{
	
	static unsigned short heart_count = 0, data_count = 0;
		
	//心跳-------------------------------------------------------------------
	if(++heart_count >= 400)
	{
		heart_count = 0;
		
		NET_Event_CallBack(NET_EVENT_Send_HeartBeat);
		
		onenet_info.send_data = SEND_TYPE_HEART;							//发送心跳请求
	}
	
	//数据-------------------------------------------------------------------
	if(++data_count >= 300)
	{
		data_count = 0;
		
		NET_Event_CallBack(NET_EVENT_Send_Data);
		
		onenet_info.send_data = SEND_TYPE_DATA;								//发送数据
	}
	
	//错误检测和处理----------------------------------------------------------
	if(onenet_info.err_check == 1)
	{
		NET_Task_ErrCheck();
		
		onenet_info.err_check = 0;
	}
	
	//错误处理---------------------------------------------------------------
	if(net_fault_info.net_fault_level != NET_FAULT_LEVEL_0)					//如果错误标志被设置
	{
		NET_Event_CallBack(NET_EVENT_Fault_Process);
		
		NET_Fault_Process();												//进入错误处理函数
	}

}

/*
************************************************************
*	函数名称：	DATA_P_Task
*
*	函数功能：	数据发送主任务
*
*	入口参数：	无
*
*	返回参数：	无
*
*	说明：		
************************************************************
*/
void DATA_P_Task(void)
{
	
	switch(onenet_info.send_data)
	{
		case SEND_TYPE_DATA:
		
			onenet_info.send_data = OneNET_SendData(FORMAT_TYPE3, NULL, NULL, data_stream, data_stream_cnt);//上传数据到平台
		
		break;
		
		case SEND_TYPE_SUBSCRIBE:
		
			onenet_info.send_data = OneNET_Subscribe(topics, 2);										//订阅主题
		
		break;
		
		case SEND_TYPE_UNSUBSCRIBE:
		
			onenet_info.send_data = OneNET_UnSubscribe(topics, 2);										//取消订阅的主题
		
		break;
		
		case SEND_TYPE_PUBLISH:
		
			onenet_info.send_data = OneNET_Publish("pc_topic", "Publish Test");							//发布主题
		
		break;
		
		case SEND_TYPE_HEART:
		
			onenet_info.send_data = OneNET_SendData_Heart();
		
		break;
		
		case SEND_TYPE_BINFILE:
		
			onenet_info.send_data = OneNET_Send_BinFile(onenet_info.file_bin_name, onenet_info.file_bin, onenet_info.file_bin_size);
		
		break;
	}
	
	OneNET_Check_Heart();

}

/*
************************************************************
*	函数名称：	DATA_S_Task
*
*	函数功能：	循环发送链表里边待发送的数据块
*
*	入口参数：	无
*
*	返回参数：	无
*
*	说明：		
************************************************************
*/
void DATA_S_Task(void)
{

	if(NET_DEVICE_CheckListHead() && onenet_info.net_work)
	{
		NET_Event_CallBack(NET_EVENT_Send);
		
		if(!NET_DEVICE_SendData(NET_DEVICE_GetListHeadBuf(), NET_DEVICE_GetListHeadLen()))
			NET_DEVICE_DeleteDataSendList();
	}

}

/*
************************************************************
*	函数名称：	NET_Task
*
*	函数功能：	网络连接、平台接入
*
*	入口参数：	无
*
*	返回参数：	无
*
*	说明：		网络连接任务任务。会在心跳检测里边检测网络连接状态，如果有错，会标记状态，然后在这里进行重连
************************************************************
*/
void NET_Task(void)
{
	
	static unsigned char auto_create_count = 4;
	static char *serial = NULL;

	if(!onenet_info.net_work && net_device_info.device_ok)				//当没有网络 且 网络模块检测到时
	{
		if(!NET_DEVICE_Init())											//初始化网络设备，能连入网络
		{
			NET_Event_CallBack(NET_EVENT_Connect);
			
			if(strlen(onenet_info.reg_code) >= 16 && auto_create_count)
			{
				--auto_create_count;
				
				if(serial == NULL)
#if 1
					NET_Task_GetMcuSerial(&serial);
#else
					NET_DEVICE_GetSerial(&serial);
#endif
				
				if(!OneNET_RepetitionCreateFlag(onenet_info.api_key))
				{
					if(!OneNET_CreateDevice(onenet_info.reg_code, serial, serial, onenet_info.dev_id, onenet_info.api_key))
					{
						NET_Event_CallBack(NET_EVENT_Auto_Create_Ok);
						
						auto_create_count = 0;
							
						strcpy(onenet_info.auif, serial);
					}
					else
						NET_Event_CallBack(NET_EVENT_Auto_Create_Err);
				}
				else
					NET_Event_CallBack(NET_EVENT_Auto_Create_Err);
			}
			else
			{
				if(OneNET_GetLocation(onenet_info.dev_id, onenet_info.api_key, gps.lon, gps.lat) == 0)
					gps.flag = 1;
				
				OneNET_ConnectIP(onenet_info.ip, onenet_info.port);
				
				OneNET_DevLink(onenet_info.dev_id, onenet_info.pro_id, onenet_info.auif);//接入平台
				
				if(onenet_info.net_work)
				{
					onenet_info.send_data = SEND_TYPE_SUBSCRIBE;			//接入成功则订阅主题
					
					NET_Event_CallBack(NET_EVENT_Connect_Ok);
				}
				else
				{
					NET_Event_CallBack(NET_EVENT_Connect_Err);
				}
			}
		}

	}
	
	if(!net_device_info.device_ok) 										//当网络设备未做检测
	{
		if(!NET_DEVICE_Exist())											//网络设备检测
		{
			NET_Event_CallBack(NET_EVENT_Device_Ok);
			net_device_info.device_ok = 1;								//检测到网络设备，标记
		}
		else
			NET_Event_CallBack(NET_EVENT_Device_Err);
	}

}

/*
************************************************************
*	函数名称：	NET_Task_Init
*
*	函数功能：	网络相关任务
*
*	入口参数：	无
*
*	返回参数：	无
*
*	说明：		
************************************************************
*/
void NET_Task_Init(void)
{

	//创建应用任务
	
	FW_CreateTask(RECV_Task, 4);
	
	FW_CreateTask(NET_FLAG_Task, 10);
	
	FW_CreateTask(DATA_P_Task, 10);
	
	FW_CreateTask(DATA_S_Task, net_device_info.send_time);
	
	FW_CreateTask(NET_Task, 5);
	
	FW_CreateTask(NET_Timer, 200);

}
