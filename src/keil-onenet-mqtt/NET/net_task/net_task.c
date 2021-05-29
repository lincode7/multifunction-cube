/**
	************************************************************
	************************************************************
	************************************************************
	*	�ļ����� 	net_task.c
	*
	*	���ߣ� 		�ż���
	*
	*	���ڣ� 		2018-03-29
	*
	*	�汾�� 		V1.0
	*
	*	˵���� 		�����������
	*
	*	�޸ļ�¼��	
	************************************************************
	************************************************************
	************************************************************
**/

//���
#include "framework.h"

//�����豸
#include "net_device.h"

//��������
#include "net_task.h"

//Э��
#include "onenet.h"
#include "fault.h"

//����
#include "delay.h"

//C��
#include <stdio.h>
#include <string.h>


#define NET_COUNT	5			//�������

#define NET_TIME	80			//�趨ʱ��--��λ��

unsigned short timer_count = 0;	//ʱ�����--��λ��


const char *topics[] = {"kyLin_topic", "topic_test"};


extern DATA_STREAM data_stream[];
extern unsigned char data_stream_cnt;


/*
************************************************************
*	�������ƣ�	NET_Event_CallBack
*
*	�������ܣ�	�����¼��ص�
*
*	��ڲ�����	net_event���¼�����
*
*	���ز�����	��
*
*	˵����		
************************************************************
*/
__weak void NET_Event_CallBack(NET_EVENT net_event)
{

	switch((unsigned char)net_event)
	{
		case NET_EVENT_Timer_Check_Err:		//���綨ʱ��鳬ʱ����
		break;
		
		case NET_EVENT_Timer_Send_Err:		//���緢��ʧ�ܴ���
		break;
		
		case NET_EVENT_Send_HeartBeat:		//��������������
		break;
		
		case NET_EVENT_Send_Data:			//�����������ݵ�
		break;
		
		case NET_EVENT_Send_Subscribe:		//�������Ͷ�������
		break;
		
		case NET_EVENT_Send_UnSubscribe:	//��������ȡ����������
		break;
		
		case NET_EVENT_Send_Publish:		//����������������
		break;
		
		case NET_EVENT_Send:				//��ʼ��������
		break;
		
		case NET_EVENT_Recv:				//Modbus��-�յ����ݲ�ѯָ��
		break;
		
		case NET_EVENT_Check_Status:		//��������ģ��״̬���
		break;
		
		case NET_EVENT_Device_Ok:			//����ģ����Ok
		break;
		case NET_EVENT_Device_Err:			//����ģ�������
		break;
		
		case NET_EVENT_Initialize:			//���ڳ�ʼ������ģ��
		break;
		
		case NET_EVENT_Init_Ok:				//����ģ���ʼ���ɹ�
		break;
		
		case NET_EVENT_Auto_Create_Ok:		//�Զ������豸�ɹ�
		break;
		
		case NET_EVENT_Auto_Create_Err:		//�Զ������豸ʧ��
		break;
		
		case NET_EVENT_Connect:				//�������ӡ���¼OneNET
		break;
		
		case NET_EVENT_Connect_Ok:			//���ӡ���¼�ɹ�
		break;
		
		case NET_EVENT_Connect_Err:			//���ӡ���¼����
		break;
		
		case NET_EVENT_Fault_Process:		//������
		break;
		
		default:							//��
		break;
	}

}

//==========================================================
//	�������ƣ�	NET_Task_GetMcuSerial
//
//	�������ܣ�	��ȡMCU ID
//
//	��ڲ�����	serial��ָ���ַ
//
//	���ز�����	0-�ɹ�		1-ʧ��
//
//	˵����		
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
//	�������ƣ�	NET_Task_ErrCheck
//
//	�������ܣ�	��ȡ����ģ��״̬��������ȼ�
//
//	��ڲ�����	��
//
//	���ز�����	��
//
//	˵����		
//==========================================================
void NET_Task_ErrCheck(void)
{
	
	unsigned char err_type = 0;

	err_type = NET_DEVICE_Check();												//�����豸״̬���
	if(err_type == NET_DEVICE_CONNECTED || err_type == NET_DEVICE_CLOSED || err_type == NET_DEVICE_GOT_IP)
	{
		net_fault_info.net_fault_level = net_fault_info.net_fault_level_r = NET_FAULT_LEVEL_1;
	}
	else if(err_type == NET_DEVICE_NO_DEVICE || err_type == NET_DEVICE_NO_CARD || err_type == NET_DEVICE_INITIAL)
	{
		net_fault_info.net_fault_level = net_fault_info.net_fault_level_r = NET_FAULT_LEVEL_3;
	}
	else																		//NET_DEVICE_CONNECTING��NET_DEVICE_BUSY
	{
		net_fault_info.net_fault_level = net_fault_info.net_fault_level_r = NET_FAULT_LEVEL_0;
	}

}

/*
************************************************************
*	�������ƣ�	NET_Timer
*
*	�������ܣ�	��ʱ�������״̬��־λ
*
*	��ڲ�����	��
*
*	���ز�����	��
*
*	˵����		��ʱ�����񡣶�ʱ�������״̬�������������趨ʱ�����������ӣ������ƽ̨����
************************************************************
*/
void NET_Timer(void)
{
	
	if(onenet_info.net_work == 0)										//����ڹ涨ʱ�������绹δ����ɹ�
	{
		if(++timer_count >= NET_TIME) 									//�������Ͽ���ʱ
		{
			NET_Event_CallBack(NET_EVENT_Timer_Check_Err);
			
			timer_count = 0;
			
			onenet_info.err_check = 1;
		}
	}
	else
	{
		timer_count = 0;												//�������
		
		if(net_device_info.send_count >= NET_COUNT)						//������ʹ�������ﵽNET_COUNT��
		{
			NET_Event_CallBack(NET_EVENT_Timer_Send_Err);
		
			net_device_info.send_count = 0;
			
			onenet_info.err_check = 1;
		}
	}

}

/*
************************************************************
*	�������ƣ�	RECV_Task
*
*	�������ܣ�	����ƽ̨�·�������
*
*	��ڲ�����	��
*
*	���ز�����	��
*
*	˵����		ƽ̨�·����������������
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
*	�������ƣ�	NET_FLAG_Task
*
*	�������ܣ�	������ر�־
*
*	��ڲ�����	��
*
*	���ز�����	��
*
*	˵����		
************************************************************
*/
void NET_FLAG_Task(void)
{
	
	static unsigned short heart_count = 0, data_count = 0;
		
	//����-------------------------------------------------------------------
	if(++heart_count >= 400)
	{
		heart_count = 0;
		
		NET_Event_CallBack(NET_EVENT_Send_HeartBeat);
		
		onenet_info.send_data = SEND_TYPE_HEART;							//������������
	}
	
	//����-------------------------------------------------------------------
	if(++data_count >= 300)
	{
		data_count = 0;
		
		NET_Event_CallBack(NET_EVENT_Send_Data);
		
		onenet_info.send_data = SEND_TYPE_DATA;								//��������
	}
	
	//������ʹ���----------------------------------------------------------
	if(onenet_info.err_check == 1)
	{
		NET_Task_ErrCheck();
		
		onenet_info.err_check = 0;
	}
	
	//������---------------------------------------------------------------
	if(net_fault_info.net_fault_level != NET_FAULT_LEVEL_0)					//��������־������
	{
		NET_Event_CallBack(NET_EVENT_Fault_Process);
		
		NET_Fault_Process();												//�����������
	}

}

/*
************************************************************
*	�������ƣ�	DATA_P_Task
*
*	�������ܣ�	���ݷ���������
*
*	��ڲ�����	��
*
*	���ز�����	��
*
*	˵����		
************************************************************
*/
void DATA_P_Task(void)
{
	
	switch(onenet_info.send_data)
	{
		case SEND_TYPE_DATA:
		
			onenet_info.send_data = OneNET_SendData(FORMAT_TYPE3, NULL, NULL, data_stream, data_stream_cnt);//�ϴ����ݵ�ƽ̨
		
		break;
		
		case SEND_TYPE_SUBSCRIBE:
		
			onenet_info.send_data = OneNET_Subscribe(topics, 2);										//��������
		
		break;
		
		case SEND_TYPE_UNSUBSCRIBE:
		
			onenet_info.send_data = OneNET_UnSubscribe(topics, 2);										//ȡ�����ĵ�����
		
		break;
		
		case SEND_TYPE_PUBLISH:
		
			onenet_info.send_data = OneNET_Publish("pc_topic", "Publish Test");							//��������
		
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
*	�������ƣ�	DATA_S_Task
*
*	�������ܣ�	ѭ������������ߴ����͵����ݿ�
*
*	��ڲ�����	��
*
*	���ز�����	��
*
*	˵����		
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
*	�������ƣ�	NET_Task
*
*	�������ܣ�	�������ӡ�ƽ̨����
*
*	��ڲ�����	��
*
*	���ز�����	��
*
*	˵����		���������������񡣻������������߼����������״̬������д�����״̬��Ȼ���������������
************************************************************
*/
void NET_Task(void)
{
	
	static unsigned char auto_create_count = 4;
	static char *serial = NULL;

	if(!onenet_info.net_work && net_device_info.device_ok)				//��û������ �� ����ģ���⵽ʱ
	{
		if(!NET_DEVICE_Init())											//��ʼ�������豸������������
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
				
				OneNET_DevLink(onenet_info.dev_id, onenet_info.pro_id, onenet_info.auif);//����ƽ̨
				
				if(onenet_info.net_work)
				{
					onenet_info.send_data = SEND_TYPE_SUBSCRIBE;			//����ɹ���������
					
					NET_Event_CallBack(NET_EVENT_Connect_Ok);
				}
				else
				{
					NET_Event_CallBack(NET_EVENT_Connect_Err);
				}
			}
		}

	}
	
	if(!net_device_info.device_ok) 										//�������豸δ�����
	{
		if(!NET_DEVICE_Exist())											//�����豸���
		{
			NET_Event_CallBack(NET_EVENT_Device_Ok);
			net_device_info.device_ok = 1;								//��⵽�����豸�����
		}
		else
			NET_Event_CallBack(NET_EVENT_Device_Err);
	}

}

/*
************************************************************
*	�������ƣ�	NET_Task_Init
*
*	�������ܣ�	�����������
*
*	��ڲ�����	��
*
*	���ز�����	��
*
*	˵����		
************************************************************
*/
void NET_Task_Init(void)
{

	//����Ӧ������
	
	FW_CreateTask(RECV_Task, 4);
	
	FW_CreateTask(NET_FLAG_Task, 10);
	
	FW_CreateTask(DATA_P_Task, 10);
	
	FW_CreateTask(DATA_S_Task, net_device_info.send_time);
	
	FW_CreateTask(NET_Task, 5);
	
	FW_CreateTask(NET_Timer, 200);

}
