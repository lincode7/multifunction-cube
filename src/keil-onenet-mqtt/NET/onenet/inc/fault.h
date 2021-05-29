#ifndef _FAULT_H_
#define _FAULT_H_





typedef enum
{

	FAULT_NONE = 0,		//无错误
	FAULT_REBOOT,		//死机重启错误
	FAULT_PRO,			//协议错误
	FAULT_NODEVICE,		//硬件掉线错误，比如8266或者6311接触不良、sim卡接触不良等硬件原因引起的错误

} FAULT_TYPE;


typedef enum
{

	NET_FAULT_LEVEL_0 = 0,
	NET_FAULT_LEVEL_1,
	NET_FAULT_LEVEL_2,
	NET_FAULT_LEVEL_3,
	NET_FAULT_LEVEL_4,
	NET_FAULT_LEVEL_5,

} FAULT_LEVEL;


typedef struct
{

	unsigned char net_fault_level;
	
	unsigned char net_fault_level_r;
	
	unsigned char net_fault_count;

} NET_FAULT_INFO;

extern NET_FAULT_INFO net_fault_info;


void NET_Fault_Process(void);


#endif
