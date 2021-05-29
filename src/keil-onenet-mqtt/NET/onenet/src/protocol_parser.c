/**
	************************************************************
	************************************************************
	************************************************************
	*	�ļ����� 	protocol_parser.c
	*
	*	���ߣ� 		�ż���
	*
	*	���ڣ� 		2019-07-08
	*
	*	�汾�� 		V1.0
	*
	*	˵���� 		Э�������
	*
	*	�޸ļ�¼��	MQTT
	************************************************************
	************************************************************
	************************************************************
**/

//Э��
#include "mqttkit.h"

//����
#include "usart.h"

//
#include "protocol_parser.h"

//C��
#include <stdio.h>
#include <string.h>
#include <ctype.h>


#define PRINT		DebugPrintf


#if(SEND_PASER_EN == 1 || RECV_PASER_EN == 1)

static char print_buf[64];


//==========================================================
//	�������ƣ�	print_hexstring
//
//	�������ܣ�	��hexתΪ�ַ���
//
//	��ڲ�����	hex��hex����
//				len�����ݳ���
//
//	���ز�����	����ת����Ļ���ָ��
//
//	˵����		��ʽ��[0xXX ...]
//==========================================================
static char *print_hexstring(const unsigned char *hex, unsigned char len)
{
	
	char temp_buf[8];

	unsigned char i = 0;
	
	if(hex == (void *)0 || len == 0)									//�����Ϸ���
		return (void *)0;
	
	for(; i < sizeof(print_buf); i++)									//�建��
		print_buf[i] = 0;
	
	print_buf[0] = '[';
	
	for(i = 0; i < len; i++)
	{
		snprintf(temp_buf, sizeof(temp_buf), "0x%02X", hex[i]);			//��ʽ��
		strcat(print_buf, temp_buf);									//���Ƶ�����ĩβ
		
		if(i < len - 1)
			strcat(print_buf, " ");										//��ӿո�
	}
	
	strcat(print_buf, "]");
	
	return print_buf;

}

//==========================================================
//	�������ƣ�	print_ascii
//
//	�������ܣ�	��hexתΪASCII�ַ�
//
//	��ڲ�����	hex��hex����
//				len�����ݳ���
//
//	���ز�����	����ת����Ļ���ָ��
//
//	˵����		
//==========================================================
static char *print_ascii(const unsigned char *hex, unsigned char len)
{

	char temp_buf[8];

	unsigned char i = 0;
	
	if(hex == (void *)0 || len == 0)									//�����Ϸ���
		return (void *)0;
	
	for(; i < sizeof(print_buf); i++)									//�建��
		print_buf[i] = 0;
	
	for(i = 0; i < len; i++)
	{
		if(isprint(hex[i]))												//�Ƿ�Ϊ�ɴ�ӡ�ַ�
		{
			snprintf(temp_buf, sizeof(temp_buf), "%c", hex[i]);			//��ʽ��
			strcat(print_buf, temp_buf);								//���Ƶ�����ĩβ
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
//	�������ƣ�	check_devid
//
//	�������ܣ�	���devid�Ƿ�Ϸ�
//
//	��ڲ�����	devid���豸ID
//				len�����ݳ���
//
//	���ز�����	0-�ɹ�	1-ʧ��
//
//	˵����		
//==========================================================
static _Bool check_devid(const unsigned char *devid, unsigned char len)
{
	
	if(devid == (void *)0 || len == 0)			//�����Ϸ���
		return 1;
	
	while(len--)
	{
		if(*devid < '0' || *devid > '9')		//�ж��Ƿ�Ϊ�����ַ�
			return 1;
		
		devid++;
	}
	
	return 0;

}

#endif

#if(SEND_PASER_EN == 1)

//==========================================================
//	�������ƣ�	Protocol_Parser_Connect
//
//	�������ܣ�	���Ӱ�Э����������ӡ
//
//	��ڲ�����	pro_buf��Э���
//
//	���ز�����	0-�ɹ�	����-����
//
//	˵����		
//==========================================================
static unsigned char Protocol_Parser_Connect(const unsigned char *pro_buf)
{

	unsigned char index = 0, mqtt_flag = 0;					//index��pro_buf����		mqtt_flag��mqtt���ӱ�־λ
	unsigned short temp = 0;								//��ʱ����һЩ��Ҫ������ж�����
	unsigned int len = 0;									//�˰�����ʣ�೤��(�ֽ�)
	_Bool flag = 0;											//�ж������־
	
//--------------------------------------------------------------------------------------------------------------------------------------------
	temp = pro_buf[index] >> 4;								//��Ϣ�����ڹ̶�ͷ����һ���ֽڸ���λ
	PRINT("��%d�ֽ�---[0x%02X]:[%s]��¼����\r\n", index, pro_buf[index], (temp == MQTT_PKT_CONNECT) ? "OK": "ERR");
	if(temp != MQTT_PKT_CONNECT)
		return 1;
	index++;
	
//--------------------------------------------------------------------------------------------------------------------------------------------
	temp = MQTT_ReadLength(pro_buf + index, 4, &len);
	PRINT("��%d~%d�ֽ�---%s:[%s]ʣ����Ϣ����---%d�ֽ�\r\n", index, index + temp - 1, print_hexstring(pro_buf + index, temp),
															(len > 0) ? "OK": "ERR", len);
	if(len == 0 || temp == 0)
		return 2;
	index += temp;
	
//--------------------------------------------------------------------------------------------------------------------------------------------
	temp = pro_buf[index] << 8 | pro_buf[index + 1];
	PRINT("��%d~%d�ֽ�---[0x%02X 0x%02X]:[%s]Э��������---%d�ֽ�\r\n", index, index + 1, pro_buf[index], pro_buf[index + 1],
																		(temp == 4) ? "OK": "ERR", temp);
	if(temp != 4)
		return 3;
	index += 2;
	
//--------------------------------------------------------------------------------------------------------------------------------------------
	if(pro_buf[index] == 'M' && pro_buf[index + 1] == 'Q' && pro_buf[index + 2] == 'T' && pro_buf[index + 3] == 'T')
		PRINT("��%d~%d�ֽ�---[0x4D 0x51 0x54 0x54]:[OK]Э����---MQTT\r\n", index, index + 2, pro_buf[index]);
	else
	{
		PRINT("��%d~%d�ֽ�---%s:[ERR]Э����---Э����������ΪMQTT\r\n",
									index, index + 2, print_hexstring(pro_buf + index, 4));
		return 4;
	}
	index += 4;
	
//--------------------------------------------------------------------------------------------------------------------------------------------
	PRINT("��%d�ֽ�---[0x%02X]:[%s]Э��ȼ�\r\n", index, pro_buf[index], (pro_buf[index] < 10) ? "OK": "ERR");
	if(temp >= 10)
		return 5;
	index++;
	
//--------------------------------------------------------------------------------------------------------------------------------------------
	mqtt_flag = pro_buf[index];
	PRINT("��%d�ֽ�---[0x%02X]:[OK]���ӱ�־\r\n", index, pro_buf[index]);
	index++;
	
//--------------------------------------------------------------------------------------------------------------------------------------------
	temp = pro_buf[index] << 8 | pro_buf[index + 1];
	PRINT("��%d~%d�ֽ�---[0x%02X 0x%02X]:[%s]��������ʱ��---%d��\r\n",
								index, index + 1, pro_buf[index], pro_buf[index + 1], (temp != 0) ? "OK": "ERR", temp);
	if(temp == 0)
		return 6;
	index += 2;
	
//--------------------------------------------------------------------------------------------------------------------------------------------
	temp = pro_buf[index] << 8 | pro_buf[index + 1];
	PRINT("��%d~%d�ֽ�---[0x%02X 0x%02X]:[%s]�豸ID����---%d�ֽ�\r\n", index, index + 1,
								pro_buf[index], pro_buf[index + 1], ((temp >= 7) && (temp <= 16)) ? "OK": "ERR", temp);
	if((temp < 7) || (temp > 16))
		return 7;
	index += 2;
	
//--------------------------------------------------------------------------------------------------------------------------------------------
	if(check_devid(pro_buf + index, temp) == 0)				//devid�Ϸ���
		flag = 1;
	else
		flag = 0;
	PRINT("��%d~%d�ֽ�---%s:[%s]", index, index + temp - 1, print_hexstring(pro_buf + index, temp), (flag == 1) ? "OK": "ERR");
	PRINT("�豸ID---%s\r\n", print_ascii(pro_buf + index, temp));
	if(flag == 0)
		return 8;
	index += temp;
	
//--------------------------------------------------------------------------------------------------------------------------------------------
	if(mqtt_flag & MQTT_CONNECT_WILL_FLAG)					//���ӱ�־�Ƿ�������will flag
	{
		unsigned short will_msg_len = pro_buf[index] << 8 | pro_buf[index + 1];
		
//--------------------------------------------------------------------------------------------------------------------------------------------
		PRINT("��%d~%d�ֽ�---[0x%02X 0x%02X]:[%s]Will Topic����---%d�ֽ�\r\n", index, index + 1, pro_buf[index], pro_buf[index + 1],
																				(will_msg_len > 0) ? "OK" : "ERR", will_msg_len);
		if(will_msg_len == 0)
			return 9;
		index += 2;
		
//--------------------------------------------------------------------------------------------------------------------------------------------
		if(will_msg_len < 32)
			PRINT("��%d~%d�ֽ�---%s:[OK]", index, index + temp - 1, print_hexstring(pro_buf + index, will_msg_len));
		else
			PRINT("��%d~%d�ֽ�---[OK]", index, index + temp - 1);
		
		PRINT("Will Topic---%s\r\n", print_ascii(pro_buf + index, temp));
		index += will_msg_len;
		
//--------------------------------------------------------------------------------------------------------------------------------------------
		will_msg_len = pro_buf[index] << 8 | pro_buf[index + 1];
		PRINT("��%d~%d�ֽ�---[0x%02X 0x%02X]:[%s]Will Msg����---%d�ֽ�\r\n", index, index + 1, pro_buf[index], pro_buf[index + 1],
																				(will_msg_len > 0) ? "OK" : "ERR", will_msg_len);
		if(will_msg_len == 0)
			return 10;
		index += 2;
		
//--------------------------------------------------------------------------------------------------------------------------------------------
		if(will_msg_len < 32)
			PRINT("��%d~%d�ֽ�---%s:[OK]", index, index + temp - 1, print_hexstring(pro_buf + index, will_msg_len));
		else
			PRINT("��%d~%d�ֽ�---[OK]", index, index + temp - 1);
		
		PRINT("Will Msg---%s\r\n", print_ascii(pro_buf + index, temp));
		index += will_msg_len;
	}
	
	if(mqtt_flag & MQTT_CONNECT_USER_NAME)
	{
		unsigned short user_len = pro_buf[index] << 8 | pro_buf[index + 1];
		
//--------------------------------------------------------------------------------------------------------------------------------------------
		PRINT("��%d~%d�ֽ�---[0x%02X 0x%02X]:[%s]User Name����---%d�ֽ�\r\n", index, index + 1, pro_buf[index],
																				pro_buf[index + 1], (user_len > 0) ? "OK" : "ERR", user_len);
		if(user_len == 0)
			return 11;
		index += 2;
		temp = user_len;
		
//--------------------------------------------------------------------------------------------------------------------------------------------
		if(user_len < 32)
			PRINT("��%d~%d�ֽ�---%s:[OK]", index, index + temp - 1, print_hexstring(pro_buf + index, user_len));
		else
			PRINT("��%d~%d�ֽ�---[OK]", index, index + temp - 1);
		
		PRINT("User Name---%s\r\n", print_ascii(pro_buf + index, temp));
		index += user_len;
	}
	
	if(mqtt_flag & MQTT_CONNECT_PASSORD)
	{
		unsigned short pswd_len = pro_buf[index] << 8 | pro_buf[index + 1];
		
//--------------------------------------------------------------------------------------------------------------------------------------------
		PRINT("��%d~%d�ֽ�---[0x%02X 0x%02X]:[%s]PassWord����---%d�ֽ�\r\n", index, index + 1, pro_buf[index],
																				pro_buf[index + 1], (pswd_len > 0) ? "OK" : "ERR", pswd_len);
		if(pswd_len == 0)
			return 12;
		index += 2;
		temp = pswd_len;
		
//--------------------------------------------------------------------------------------------------------------------------------------------
		if(pswd_len < 32)
			PRINT("��%d~%d�ֽ�---%s:[OK]", index, index + temp - 1, print_hexstring(pro_buf + index, pswd_len));
		else
			PRINT("��%d~%d�ֽ�---[OK]", index, index + temp - 1);
		
		PRINT("PassWord---%s\r\n", print_ascii(pro_buf + index, temp));
	}
	
	return 0;

}

//==========================================================
//	�������ƣ�	Protocol_Parser_DisConnect
//
//	�������ܣ�	�Ͽ��������ݰ�Э����������ӡ
//
//	��ڲ�����	pro_buf��Э���
//
//	���ز�����	0-�ɹ�	����-����
//
//	˵����		
//==========================================================
static unsigned char Protocol_Parser_DisConnect(const unsigned char *pro_buf)
{
	
	unsigned char index = 0, temp = 0;		//index��pro_buf����		temp����ʱ����һЩ��Ҫ������ж�����

//--------------------------------------------------------------------------------------------------------------------------------------------
	temp = pro_buf[index] >> 4;				//��Ϣ�����ڹ̶�ͷ����һ���ֽڸ���λ
	PRINT("��%d�ֽ�---[0x%02X]:[%s]�Ͽ���������\r\n", index, pro_buf[index], (temp == MQTT_PKT_CONNECT) ? "OK": "ERR");
	if(temp != MQTT_PKT_CONNECT)
		return 1;
	index++;
	
//--------------------------------------------------------------------------------------------------------------------------------------------
	PRINT("��%d�ֽ�---[0x%02X]:[%s]ʣ����Ϣ����---%d�ֽ�\r\n", index, pro_buf[index], (pro_buf[index] == 0) ? "OK" : "ERR", pro_buf[index]);
	if(pro_buf[index] != 0)
		return 2;
	
	return 0;

}

//==========================================================
//	�������ƣ�	Protocol_Parser_Publish
//
//	�������ܣ�	�������ݰ�Э����������ӡ
//
//	��ڲ�����	pro_buf��Э���
//
//	���ز�����	0-�ɹ�	����-����
//
//	˵����		
//==========================================================
static unsigned char Protocol_Parser_Publish(const unsigned char *pro_buf)
{

	unsigned char index = 0, mqtt_flag = 0, remain_cnt = 0;		//index��pro_buf����		mqtt_flag���̶�ͷ���ĵ�һ���ֽ�	remain_cnt��ʣ���ֽڼ���
	unsigned short topic_len = 0;								//topic����
	unsigned int temp = 0;										//��ʱ����һЩ��Ҫ������ж�����
	unsigned int len = 0;										//�˰�����ʣ�೤��(�ֽ�)
	
//--------------------------------------------------------------------------------------------------------------------------------------------
	mqtt_flag = pro_buf[index];
	temp = pro_buf[index] >> 4;									//��Ϣ�����ڹ̶�ͷ����һ���ֽڸ���λ
	PRINT("��%d�ֽ�---[0x%02X]:[%s]������Ϣ\r\n", index, pro_buf[index], (temp == MQTT_PKT_PUBLISH) ? "OK": "ERR");
	if(temp != MQTT_PKT_PUBLISH)
		return 1;
	index++;
	
//--------------------------------------------------------------------------------------------------------------------------------------------
	temp = MQTT_ReadLength(pro_buf + index, 4, &len);
	remain_cnt = temp;
	PRINT("��%d~%d�ֽ�---%s:[%s]ʣ����Ϣ����---%d�ֽ�\r\n", index, index + temp - 1,
															print_hexstring(pro_buf + index, temp), (len > 3) ? "OK": "ERR", len);
	if(len <= 3 || temp == 0)
		return 2;
	index += temp;
	
//--------------------------------------------------------------------------------------------------------------------------------------------
	topic_len = pro_buf[index] << 8 | pro_buf[index + 1];
	PRINT("��%d~%d�ֽ�---[0x%02X 0x%02X]:[%s]Topic����---%d�ֽ�\r\n", index, index + 1, pro_buf[index],
																		pro_buf[index + 1], (topic_len > 0) ? "OK" : "ERR", topic_len);
	if(topic_len == 0)
		return 3;
	index += 2;
	
//--------------------------------------------------------------------------------------------------------------------------------------------
	if(topic_len < 32)
		PRINT("��%d~%d�ֽ�---%s:[OK]", index, index + topic_len - 1, print_hexstring(pro_buf + index, topic_len));
	else
		PRINT("��%d~%d�ֽ�---[OK]", index, index + topic_len - 1);
	
	PRINT("Topic---%s\r\n", print_ascii(pro_buf + index, topic_len));
	index += topic_len;
	
	if(((mqtt_flag >> 1) & MQTT_QOS_LEVEL1) || ((mqtt_flag >> 1) & MQTT_QOS_LEVEL2))
	{
//--------------------------------------------------------------------------------------------------------------------------------------------
		PRINT("��%d~%d�ֽ�---[0x%02X 0x%02X]:[OK]PKT ID\r\n", index, index + 1, pro_buf[index], pro_buf[index + 1]);
		index += 2;
	}
	
	if(pro_buf[index] == 2)																	//�������ļ�
	{
//--------------------------------------------------------------------------------------------------------------------------------------------
		PRINT("��%d�ֽ�---[0x%02X]:[OK]��������\r\n", index, pro_buf[index]);
		index++;
		
//--------------------------------------------------------------------------------------------------------------------------------------------
		temp = pro_buf[index] << 8 | pro_buf[index + 1];
		PRINT("��%d~%d�ֽ�---[0x%02X 0x%02X]:[OK]Bin Head����---%d�ֽ�\r\n", index, index + 1, pro_buf[index],
																			pro_buf[index + 1], (temp > 12) ? "OK": "ERR", temp);
		if(temp <= 12)
			return 4;
		index += 2;
		
//--------------------------------------------------------------------------------------------------------------------------------------------
		PRINT("��%d~%d�ֽ�---%s:[OK]Bin Head", index, index + temp - 1, print_hexstring(pro_buf + index, temp));
		PRINT("---%s\r\n", print_ascii(pro_buf + index, temp));
		index += temp;
		
//--------------------------------------------------------------------------------------------------------------------------------------------
		temp = pro_buf[index] << 24 | pro_buf[index + 1] << 16 | pro_buf[index + 2] << 8 | pro_buf[index + 3];
		PRINT("��%d~%d�ֽ�---%s:[OK]Bin File����---%d�ֽ�\r\n", index, index + 3, print_hexstring(pro_buf + index, 4), temp);
		index += 4;
	}
	else if((pro_buf[index] == 1) || (pro_buf[index] > 2 && pro_buf[index] < 6))			//�ϴ����ݵ�
	{
//--------------------------------------------------------------------------------------------------------------------------------------------
		PRINT("��%d�ֽ�---[0x%02X]:[OK]��������\r\n", index, pro_buf[index]);
		index++;
		
//--------------------------------------------------------------------------------------------------------------------------------------------
		temp = pro_buf[index] << 8 | pro_buf[index + 1];
		PRINT("��%d~%d�ֽ�---[0x%02X 0x%02X]:[OK]���ݳ���---%d�ֽ�\r\n", index, index + 1, pro_buf[index], pro_buf[index + 1], temp);
		index += 2;
	}
	else																					//������Ϣ
	{
		unsigned int payload_len = 0;
																							//���Ƿ���Qos��־����Ϊ0��ʱ����û��pkt id��
		if(((mqtt_flag >> 1) & MQTT_QOS_LEVEL1) || ((mqtt_flag >> 1) & MQTT_QOS_LEVEL2))	//�������
			payload_len = len - topic_len - remain_cnt - 3;
		else																				//������Ϣ
			payload_len = len - topic_len - 2;
		
//--------------------------------------------------------------------------------------------------------------------------------------------
		PRINT("��%d~%d�ֽ�---[OK]������Ϣ---%d�ֽ�\r\n", index, index + payload_len - 1, payload_len);
		if(payload_len < 56)
			PRINT("%s\r\n", print_ascii(pro_buf + index, payload_len));
	}
	
	return 0;

}

//==========================================================
//	�������ƣ�	Protocol_Parser_Subscribe
//
//	�������ܣ�	�������ݰ�Э����������ӡ
//
//	��ڲ�����	pro_buf��Э���
//
//	���ز�����	0-�ɹ�	����-����
//
//	˵����		
//==========================================================
static unsigned char Protocol_Parser_Subscribe(const unsigned char *pro_buf)
{

	unsigned char index = 0;						//index��pro_buf����
	unsigned int temp = 0;							//��ʱ����һЩ��Ҫ������ж�����
	unsigned int len = 0;							//�˰�����ʣ�೤��(�ֽ�)
	
//--------------------------------------------------------------------------------------------------------------------------------------------
	temp = pro_buf[index] >> 4;						//��Ϣ�����ڹ̶�ͷ����һ���ֽڸ���λ
	PRINT("��%d�ֽ�---[0x%02X]:[%s]������Ϣ\r\n", index, pro_buf[index], (temp == MQTT_PKT_SUBSCRIBE) ? "OK": "ERR");
	if(temp != MQTT_PKT_SUBSCRIBE)
		return 1;
	index++;
	
//--------------------------------------------------------------------------------------------------------------------------------------------
	temp = MQTT_ReadLength(pro_buf + index, 4, &len);
	PRINT("��%d~%d�ֽ�---%s:[%s]ʣ����Ϣ����---%d�ֽ�\r\n", index, index + temp - 1,
															print_hexstring(pro_buf + index, temp), (len > 4) ? "OK": "ERR", len);
	if(len <= 4 || temp == 0)
		return 2;
	index += temp;
	
//--------------------------------------------------------------------------------------------------------------------------------------------
	PRINT("��%d~%d�ֽ�---[0x%02X 0x%02X]:[OK]PKT ID\r\n", index, index + 1, pro_buf[index], pro_buf[index + 1]);
	index += 2;
	len -= 2;
	
	while(len)										//������Ϣʣ�೤�����ж��Ƿ������һ��topic
	{
//--------------------------------------------------------------------------------------------------------------------------------------------
		temp = pro_buf[index] << 8 | pro_buf[index + 1];
		PRINT("��%d~%d�ֽ�---[0x%02X 0x%02X]:[%s]Topic����---%d�ֽ�\r\n", index, index + 1, pro_buf[index],
																			pro_buf[index + 1], (temp > 0) ? "OK" : "ERR" , temp);
		if(temp == 0)
			return 3;
		index += 2;
		len -= 2;
		
//--------------------------------------------------------------------------------------------------------------------------------------------
		PRINT("��%d~%d�ֽ�---%s:[OK]Topic", index, index + temp - 1, print_hexstring(pro_buf + index, temp));
		PRINT("---%s\r\n", print_ascii(pro_buf + index, temp));
		index += temp;
		len -= temp;
		
//--------------------------------------------------------------------------------------------------------------------------------------------
		PRINT("��%d�ֽ�---[0x%02X]:[%s]QoS\r\n", index, pro_buf[index], (pro_buf[index] <= 2) ? "OK" : "ERR");
		if(pro_buf[index] > 2)
			return 4;
		index++;
		len--;
	}
	
	return 0;

}

//==========================================================
//	�������ƣ�	Protocol_Parser_UnSubscribe
//
//	�������ܣ�	ȡ���������ݰ�Э����������ӡ
//
//	��ڲ�����	pro_buf��Э���
//
//	���ز�����	0-�ɹ�	����-����
//
//	˵����		
//==========================================================
static unsigned char Protocol_Parser_UnSubscribe(const unsigned char *pro_buf)
{

	unsigned char index = 0;						//index��pro_buf����
	unsigned int temp = 0;							//��ʱ����һЩ��Ҫ������ж�����
	unsigned int len = 0;							//�˰�����ʣ�೤��(�ֽ�)
	
//--------------------------------------------------------------------------------------------------------------------------------------------
	temp = pro_buf[index] >> 4;						//��Ϣ�����ڹ̶�ͷ����һ���ֽڸ���λ
	PRINT("��%d�ֽ�---[0x%02X]:[%s]ȡ��������Ϣ\r\n", index, pro_buf[index], (temp == MQTT_PKT_UNSUBSCRIBE) ? "OK": "ERR");
	if(temp != MQTT_PKT_UNSUBSCRIBE)
		return 1;
	index++;
	
//--------------------------------------------------------------------------------------------------------------------------------------------
	temp = MQTT_ReadLength(pro_buf + index, 4, &len);
	PRINT("��%d~%d�ֽ�---%s:[%s]ʣ����Ϣ����---%d�ֽ�\r\n", index, index + temp - 1,
															print_hexstring(pro_buf + index, temp), (len > 3) ? "OK": "ERR", len);
	if(len <= 3 || temp == 0)
		return 2;
	index += temp;
	
//--------------------------------------------------------------------------------------------------------------------------------------------
	PRINT("��%d~%d�ֽ�---[0x%02X 0x%02X]:[OK]PKT ID\r\n", index, index + 1, pro_buf[index], pro_buf[index + 1]);
	index += 2;
	len -= 2;
	
	while(len)										//������Ϣʣ�೤�����ж��Ƿ������һ��topic
	{
//--------------------------------------------------------------------------------------------------------------------------------------------
		temp = pro_buf[index] << 8 | pro_buf[index + 1];
		PRINT("��%d~%d�ֽ�---[0x%02X 0x%02X]:[%s]Topic����---%d�ֽ�\r\n", index, index + 1, pro_buf[index],
																			pro_buf[index + 1], (temp > 0) ? "OK" : "ERR" , temp);
		if(temp == 0)
			return 3;
		index += 2;
		len -= 2;
		
//--------------------------------------------------------------------------------------------------------------------------------------------
		PRINT("��%d~%d�ֽ�---%s:[OK]Topic", index, index + temp - 1, print_hexstring(pro_buf + index, temp));
		PRINT("---%s\r\n", print_ascii(pro_buf + index, temp));
		index += temp;
		len -= temp;
	}
	
	return 0;

}

//==========================================================
//	�������ƣ�	Protocol_Parser_Ping
//
//	�������ܣ�	�������ݰ�Э����������ӡ
//
//	��ڲ�����	pro_buf��Э���
//
//	���ز�����	0-�ɹ�	����-����
//
//	˵����		
//==========================================================
static unsigned char Protocol_Parser_Ping(const unsigned char *pro_buf)
{

	unsigned char index = 0, temp = 0;				//index��pro_buf����		temp����ʱ����һЩ��Ҫ������ж�����
	
//--------------------------------------------------------------------------------------------------------------------------------------------
	temp = pro_buf[index] >> 4;						//��Ϣ�����ڹ̶�ͷ����һ���ֽڸ���λ
	PRINT("��%d�ֽ�---[0x%02X]:[%s]��������\r\n", index, pro_buf[index], (temp == MQTT_PKT_PINGREQ) ? "OK": "ERR");
	if(temp != MQTT_PKT_PINGREQ)
		return 1;
	index++;
	
//--------------------------------------------------------------------------------------------------------------------------------------------
	PRINT("��%d�ֽ�---[0x%02X]:[%s]ʣ����Ϣ����---%d�ֽ�\r\n", index, pro_buf[index], (pro_buf[index] == 0) ? "OK": "ERR", pro_buf[index]);
	if(pro_buf[index] != 0)
		return 2;
	
	return 0;

}

#endif

#if(RECV_PASER_EN == 1)

//==========================================================
//	�������ƣ�	Protocol_Parser_ConnectResp
//
//	�������ܣ�	���ӻظ����ݰ�Э����������ӡ
//
//	��ڲ�����	pro_buf��Э���
//
//	���ز�����	0-�ɹ�	����-����
//
//	˵����		
//==========================================================
static unsigned char Protocol_Parser_ConnectResp(const unsigned char *pro_buf)
{

	unsigned char index = 0;						//index��pro_buf����
	unsigned int temp = 0;							//��ʱ����һЩ��Ҫ������ж�����
	unsigned int len = 0;							//�˰�����ʣ�೤��(�ֽ�)
	
//--------------------------------------------------------------------------------------------------------------------------------------------
	temp = pro_buf[index] >> 4;						//��Ϣ�����ڹ̶�ͷ����һ���ֽڸ���λ
	PRINT("��%d�ֽ�---[0x%02X]:[%s]���ӻظ�\r\n", index, pro_buf[index], (temp == MQTT_PKT_CONNACK) ? "OK": "ERR");
	if(temp != MQTT_PKT_CONNACK)
		return 1;
	index++;
	
//--------------------------------------------------------------------------------------------------------------------------------------------
	temp = MQTT_ReadLength(pro_buf + index, 4, &len);
	PRINT("��%d~%d�ֽ�---%s:[%s]ʣ����Ϣ����---%d�ֽ�\r\n", index, index + temp - 1,
															print_hexstring(pro_buf + index, temp), (len == 2) ? "OK": "ERR", len);
	if(len != 2 || temp == 0)
		return 2;
	index += temp;
	
//--------------------------------------------------------------------------------------------------------------------------------------------
	PRINT("��%d�ֽ�---[0x%02X]:[%s]Session Present��־\r\n", index, pro_buf[index], (pro_buf[index] < 2) ? "OK": "ERR");
	if(pro_buf[index] >= 2)
		return 3;
	index++;
	
//--------------------------------------------------------------------------------------------------------------------------------------------
	PRINT("��%d�ֽ�---[0x%02X]:[%s]�ظ����\r\n", index, pro_buf[index], (pro_buf[index] < 6) ? "OK": "ERR");
	if(pro_buf[index] >= 6)
		return 4;
#if 0
	{
		char *str = (void *)0;
		
		PRINT("��%d�ֽ�---[0x%02X]:[OK]���ӽ��---%d,", index, pro_buf[index], pro_buf[index]);
		
		switch(pro_buf[index])
		{
			case 0:		str = "���ӳɹ�";					break;
			case 1:		str = "Э��汾����";				break;
			case 2:		str = "�Ƿ���clientid";				break;
			case 3:		str = "���񲻿���";					break;
			case 4:		str = "�û������������";			break;
			case 5:		str = "�Ƿ�����(����token�Ƿ�)";		break;
		}
		
		PRINT("%s\r\n", str);
	}
#endif
	
	return 0;

}

//==========================================================
//	�������ƣ�	Protocol_Parser_PingResp
//
//	�������ܣ�	�����ظ����ݰ�Э����������ӡ
//
//	��ڲ�����	pro_buf��Э���
//
//	���ز�����	0-�ɹ�	����-����
//
//	˵����		
//==========================================================
static unsigned char Protocol_Parser_PingResp(const unsigned char *pro_buf)
{

	unsigned char index = 0, temp = 0;				//index��pro_buf����		temp����ʱ����һЩ��Ҫ������ж�����
	
//--------------------------------------------------------------------------------------------------------------------------------------------
	temp = pro_buf[index] >> 4;						//��Ϣ�����ڹ̶�ͷ����һ���ֽڸ���λ
	PRINT("��%d�ֽ�---[0x%02X]:[%s]�����ظ�\r\n", index, pro_buf[index], (temp == MQTT_PKT_PINGRESP) ? "OK": "ERR");
	if(temp != MQTT_PKT_PINGRESP)
		return 1;
	index++;
	
//--------------------------------------------------------------------------------------------------------------------------------------------
	PRINT("��%d�ֽ�---[0x%02X]:[%s]ʣ����Ϣ����---%d�ֽ�\r\n", index, pro_buf[index], (pro_buf[index] == 0) ? "OK": "ERR", pro_buf[index]);
	if(pro_buf[index] != 0)
		return 1;
	
	return 0;

}

//==========================================================
//	�������ƣ�	Protocol_Parser_PublishResp
//
//	�������ܣ�	������Ϣ�ظ����ݰ�Э����������ӡ
//
//	��ڲ�����	pro_buf��Э���
//
//	���ز�����	0-�ɹ�	����-����
//
//	˵����		
//==========================================================
static unsigned char Protocol_Parser_PublishResp(const unsigned char *pro_buf)
{

	unsigned char index = 0;						//index��pro_buf����
	unsigned int temp = 0;							//��ʱ����һЩ��Ҫ������ж�����
	unsigned int len = 0;							//�˰�����ʣ�೤��(�ֽ�)
	
//--------------------------------------------------------------------------------------------------------------------------------------------
	temp = pro_buf[index] >> 4;						//��Ϣ�����ڹ̶�ͷ����һ���ֽڸ���λ
	PRINT("��%d�ֽ�---[0x%02X]:[%s]������Ϣ�ظ�\r\n", index, pro_buf[index], (temp == MQTT_PKT_PUBACK) ? "OK": "ERR");
	if(temp != MQTT_PKT_PUBACK)
		return 1;
	index++;
	
//--------------------------------------------------------------------------------------------------------------------------------------------
	temp = MQTT_ReadLength(pro_buf + index, 4, &len);
	PRINT("��%d~%d�ֽ�---%s:[%s]ʣ����Ϣ����---%d�ֽ�\r\n", index, index + temp - 1,
															print_hexstring(pro_buf + index, temp), (len > 1) ? "OK": "ERR", len);
	if(len <= 1 || temp == 0)
		return 2;
	index += temp;
	
//--------------------------------------------------------------------------------------------------------------------------------------------
	PRINT("��%d~%d�ֽ�---[0x%02X 0x%02X]:[OK]PKT ID\r\n", index, index + 1, pro_buf[index], pro_buf[index + 1]);
	
	return 0;

}

//==========================================================
//	�������ƣ�	Protocol_Parser_SubscribeResp
//
//	�������ܣ�	���Ļظ����ݰ�Э����������ӡ
//
//	��ڲ�����	pro_buf��Э���
//
//	���ز�����	0-�ɹ�	����-����
//
//	˵����		
//==========================================================
static unsigned char Protocol_Parser_SubscribeResp(const unsigned char *pro_buf)
{

	unsigned char index = 0;						//index��pro_buf����
	unsigned int temp = 0;							//��ʱ����һЩ��Ҫ������ж�����
	unsigned int len = 0;							//�˰�����ʣ�೤��(�ֽ�)
	
//--------------------------------------------------------------------------------------------------------------------------------------------
	temp = pro_buf[index] >> 4;						//��Ϣ�����ڹ̶�ͷ����һ���ֽڸ���λ
	PRINT("��%d�ֽ�---[0x%02X]:[%s]���Ļظ�\r\n", index, pro_buf[index], (temp == MQTT_PKT_SUBACK) ? "OK": "ERR");
	if(temp != MQTT_PKT_SUBACK)
		return 1;
	index++;
	
//--------------------------------------------------------------------------------------------------------------------------------------------
	temp = MQTT_ReadLength(pro_buf + index, 4, &len);
	PRINT("��%d~%d�ֽ�---%s:[%s]ʣ����Ϣ����---%d�ֽ�\r\n", index, index + temp - 1,
															print_hexstring(pro_buf + index, temp), (len > 2) ? "OK": "ERR", len);
	if(len <= 2 || temp == 0)
		return 2;
	index += temp;
	
//--------------------------------------------------------------------------------------------------------------------------------------------
	PRINT("��%d~%d�ֽ�---[0x%02X 0x%02X]:[OK]PKT ID\r\n", index, index + 1, pro_buf[index], pro_buf[index + 1]);
	index += 2;
	
//--------------------------------------------------------------------------------------------------------------------------------------------
#if 0
	PRINT("��%d�ֽ�---[0x%02X]:[OK]���Ļظ����\r\n", index, pro_buf[index]);
#else
	{
		char *str = (void *)0;
		
		PRINT("��%d�ֽ�---[0x%02X]:[OK]���Ľ��---%d,", index, pro_buf[index], pro_buf[index]);
		
		switch(pro_buf[index])
		{
			case 0:		str = "���ĳɹ�(Qos = 0)";			break;
			case 1:		str = "���ĳɹ�(Qos = 1)";			break;
			case 2:		str = "���ĳɹ�(Qos = 2)";			break;
			case 0x80:	str = "����ʧ��";					break;
		}
		
		PRINT("%s\r\n", str);
	}
#endif
	
	
	return 0;

}

//==========================================================
//	�������ƣ�	Protocol_Parser_UnSubscribeResp
//
//	�������ܣ�	ȡ�����Ļظ����ݰ�Э����������ӡ
//
//	��ڲ�����	pro_buf��Э���
//
//	���ز�����	0-�ɹ�	����-����
//
//	˵����		
//==========================================================
static unsigned char Protocol_Parser_UnSubscribeResp(const unsigned char *pro_buf)
{

	unsigned char index = 0;						//index��pro_buf����
	unsigned int temp = 0;							//��ʱ����һЩ��Ҫ������ж�����
	unsigned int len = 0;							//�˰�����ʣ�೤��(�ֽ�)
	
//--------------------------------------------------------------------------------------------------------------------------------------------
	temp = pro_buf[index] >> 4;						//��Ϣ�����ڹ̶�ͷ����һ���ֽڸ���λ
	PRINT("��%d�ֽ�---[0x%02X]:[%s]ȡ�����Ļظ�\r\n", index, pro_buf[index], (temp == MQTT_PKT_UNSUBACK) ? "OK": "ERR");
	if(temp != MQTT_PKT_UNSUBACK)
		return 1;
	index++;
	
//--------------------------------------------------------------------------------------------------------------------------------------------
	temp = MQTT_ReadLength(pro_buf + index, 4, &len);
	PRINT("��%d~%d�ֽ�---%s:[%s]ʣ����Ϣ����---%d�ֽ�\r\n", index, index + temp - 1,
															print_hexstring(pro_buf + index, temp), (len > 1) ? "OK": "ERR", len);
	if(len <= 1 || temp == 0)
		return 2;
	index += temp;
	
//--------------------------------------------------------------------------------------------------------------------------------------------
	PRINT("��%d~%d�ֽ�---[0x%02X 0x%02X]:[OK]PKT ID\r\n", index, index + 1, pro_buf[index], pro_buf[index + 1]);
	
	return 0;

}

//==========================================================
//	�������ƣ�	Protocol_Parser_PublishRec
//
//	�������ܣ�	Publish Rec���ݰ�Э����������ӡ
//
//	��ڲ�����	pro_buf��Э���
//
//	���ز�����	0-�ɹ�	����-����
//
//	˵����		
//==========================================================
static unsigned char Protocol_Parser_PublishRec(const unsigned char *pro_buf)
{

	unsigned char index = 0;						//index��pro_buf����
	unsigned int temp = 0;							//��ʱ����һЩ��Ҫ������ж�����
	unsigned int len = 0;							//�˰�����ʣ�೤��(�ֽ�)
	
//--------------------------------------------------------------------------------------------------------------------------------------------
	temp = pro_buf[index] >> 4;						//��Ϣ�����ڹ̶�ͷ����һ���ֽڸ���λ
	PRINT("��%d�ֽ�---[0x%02X]:[%s]Publish Rec\r\n", index, pro_buf[index], (temp == MQTT_PKT_PUBREC) ? "OK": "ERR");
	if(temp != MQTT_PKT_PUBREC)
		return 1;
	index++;
	
//--------------------------------------------------------------------------------------------------------------------------------------------
	temp = MQTT_ReadLength(pro_buf + index, 4, &len);
	PRINT("��%d~%d�ֽ�---%s:[%s]ʣ����Ϣ����---%d�ֽ�\r\n", index, index + temp - 1,
															print_hexstring(pro_buf + index, temp), (len > 1) ? "OK": "ERR", len);
	if(len <= 1 || temp == 0)
		return 2;
	index += temp;
	
//--------------------------------------------------------------------------------------------------------------------------------------------
	PRINT("��%d~%d�ֽ�---[0x%02X 0x%02X]:[OK]PKT ID\r\n", index, index + 1, pro_buf[index], pro_buf[index + 1]);
	
	return 0;

}

//==========================================================
//	�������ƣ�	Protocol_Parser_PublishRel
//
//	�������ܣ�	Publish Rel���ݰ�Э����������ӡ
//
//	��ڲ�����	pro_buf��Э���
//
//	���ز�����	0-�ɹ�	����-����
//
//	˵����		
//==========================================================
static unsigned char Protocol_Parser_PublishRel(const unsigned char *pro_buf)
{

	unsigned char index = 0;						//index��pro_buf����
	unsigned int temp = 0;							//��ʱ����һЩ��Ҫ������ж�����
	unsigned int len = 0;							//�˰�����ʣ�೤��(�ֽ�)
	
//--------------------------------------------------------------------------------------------------------------------------------------------
	temp = pro_buf[index] >> 4;						//��Ϣ�����ڹ̶�ͷ����һ���ֽڸ���λ
	PRINT("��%d�ֽ�---[0x%02X]:[%s]Publish Rel\r\n", index, pro_buf[index], (temp == MQTT_PKT_PUBREL) ? "OK": "ERR");
	if(temp != MQTT_PKT_PUBREL)
		return 1;
	index++;
	
//--------------------------------------------------------------------------------------------------------------------------------------------
	temp = MQTT_ReadLength(pro_buf + index, 4, &len);
	PRINT("��%d~%d�ֽ�---%s:[%s]ʣ����Ϣ����---%d�ֽ�\r\n", index, index + temp - 1,
															print_hexstring(pro_buf + index, temp), (len > 1) ? "OK": "ERR", len);
	if(len <= 1 || temp == 0)
		return 2;
	index += temp;
	
//--------------------------------------------------------------------------------------------------------------------------------------------
	PRINT("��%d~%d�ֽ�---[0x%02X 0x%02X]:[OK]PKT ID\r\n", index, index + 1, pro_buf[index], pro_buf[index + 1]);
	
	return 0;

}

//==========================================================
//	�������ƣ�	Protocol_Parser_PublishComp
//
//	�������ܣ�	Publish Comp���ݰ�Э����������ӡ
//
//	��ڲ�����	pro_buf��Э���
//
//	���ز�����	0-�ɹ�	����-����
//
//	˵����		
//==========================================================
static unsigned char Protocol_Parser_PublishComp(const unsigned char *pro_buf)
{

	unsigned char index = 0;						//index��pro_buf����
	unsigned int temp = 0;							//��ʱ����һЩ��Ҫ������ж�����
	unsigned int len = 0;							//�˰�����ʣ�೤��(�ֽ�)
	
//--------------------------------------------------------------------------------------------------------------------------------------------
	temp = pro_buf[index] >> 4;						//��Ϣ�����ڹ̶�ͷ����һ���ֽڸ���λ
	PRINT("��%d�ֽ�---[0x%02X]:[%s]Publish Comp\r\n", index, pro_buf[index], (temp == MQTT_PKT_PUBCOMP) ? "OK": "ERR");
	if(temp != MQTT_PKT_PUBCOMP)
		return 1;
	index++;
	
//--------------------------------------------------------------------------------------------------------------------------------------------
	temp = MQTT_ReadLength(pro_buf + index, 4, &len);
	PRINT("��%d~%d�ֽ�---%s:[%s]ʣ����Ϣ����---%d�ֽ�\r\n", index, index + temp - 1,
															print_hexstring(pro_buf + index, temp), (len > 1) ? "OK": "ERR", len);
	if(len <= 1 || temp == 0)
		return 2;
	index += temp;
	
//--------------------------------------------------------------------------------------------------------------------------------------------
	PRINT("��%d~%d�ֽ�---[0x%02X 0x%02X]:[OK]PKT ID\r\n", index, index + 1, pro_buf[index], pro_buf[index + 1]);
	
	return 0;

}

#endif

//==========================================================
//	�������ƣ�	Protocol_Parser_Print
//
//	�������ܣ�	Э����������ӡ
//
//	��ڲ�����	pro_buf��Э���
//
//	���ز�����	
//
//	˵����		
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
				PRINT("\r\n�������ݴ���:%d\r\n", result);
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
				PRINT("\r\n��¼���Ĵ���:%d\r\n", result);
			PRINT("- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -\r\n");
			PRINT("----------------------------------------------------------------------\r\n\r\n");
		
		break;
			
		case MQTT_PKT_DISCONNECT:
		
			PRINT("\r\n----------------------------------------------------------------------\r\n");
			PRINT("- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -\r\n");
			result = Protocol_Parser_DisConnect(pro_buf);
			if(result)
				PRINT("\r\n�Ͽ����ӱ��Ĵ���:%d\r\n", result);
			PRINT("- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -\r\n");
			PRINT("----------------------------------------------------------------------\r\n\r\n");
		
		break;
		
		case MQTT_PKT_SUBSCRIBE:
		
			PRINT("\r\n----------------------------------------------------------------------\r\n");
			PRINT("- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -\r\n");
			result = Protocol_Parser_Subscribe(pro_buf);
			if(result)
				PRINT("\r\n������Ϣ����:%d\r\n", result);
			PRINT("- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -\r\n");
			PRINT("----------------------------------------------------------------------\r\n\r\n");
		
		break;
		
		case MQTT_PKT_UNSUBSCRIBE:
		
			PRINT("\r\n----------------------------------------------------------------------\r\n");
			PRINT("- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -\r\n");
			result = Protocol_Parser_UnSubscribe(pro_buf);
			if(result)
				PRINT("\r\nȡ��������Ϣ����:%d\r\n", result);
			PRINT("- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -\r\n");
			PRINT("----------------------------------------------------------------------\r\n\r\n");
		
		break;
		
		case MQTT_PKT_PINGREQ:
		
			PRINT("\r\n----------------------------------------------------------------------\r\n");
			PRINT("- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -\r\n");
			result = Protocol_Parser_Ping(pro_buf);
			if(result)
				PRINT("\r\n������Ϣ����:%d\r\n", result);
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
				PRINT("\r\n���ӻظ�����:%d\r\n", result);
			PRINT("- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -\r\n");
			PRINT("----------------------------------------------------------------------\r\n\r\n");
		
		break;
		
		case MQTT_PKT_PINGRESP:
		
			PRINT("\r\n----------------------------------------------------------------------\r\n");
			PRINT("- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -\r\n");
			result = Protocol_Parser_PingResp(pro_buf);
			if(result)
				PRINT("\r\n�����ظ�����:%d\r\n", result);
			PRINT("- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -\r\n");
			PRINT("----------------------------------------------------------------------\r\n\r\n");
		
		break;
		
		case MQTT_PKT_PUBACK:
		
			PRINT("\r\n----------------------------------------------------------------------\r\n");
			PRINT("- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -\r\n");
			result = Protocol_Parser_PublishResp(pro_buf);
			if(result)
				PRINT("\r\n������Ϣ�ظ�����:%d\r\n", result);
			PRINT("- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -\r\n");
			PRINT("----------------------------------------------------------------------\r\n\r\n");
		
		break;
		
		case MQTT_PKT_SUBACK:
		
			PRINT("\r\n----------------------------------------------------------------------\r\n");
			PRINT("- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -\r\n");
			result = Protocol_Parser_SubscribeResp(pro_buf);
			if(result)
				PRINT("\r\n���Ļظ�����:%d\r\n", result);
			PRINT("- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -\r\n");
			PRINT("----------------------------------------------------------------------\r\n\r\n");
		
		break;
		
		case MQTT_PKT_UNSUBACK:
		
			PRINT("\r\n----------------------------------------------------------------------\r\n");
			PRINT("- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -\r\n");
			result = Protocol_Parser_UnSubscribeResp(pro_buf);
			if(result)
				PRINT("\r\nȡ�����Ļظ�����:%d\r\n", result);
			PRINT("- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -\r\n");
			PRINT("----------------------------------------------------------------------\r\n\r\n");
		
		break;
		
		case MQTT_PKT_PUBREC:
		
			PRINT("\r\n----------------------------------------------------------------------\r\n");
			PRINT("- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -\r\n");
			result = Protocol_Parser_PublishRec(pro_buf);
			if(result)
				PRINT("\r\nPublish Rec����:%d\r\n", result);
			PRINT("- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -\r\n");
			PRINT("----------------------------------------------------------------------\r\n\r\n");
		
		break;
		
		case MQTT_PKT_PUBREL:
		
			PRINT("\r\n----------------------------------------------------------------------\r\n");
			PRINT("- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -\r\n");
			result = Protocol_Parser_PublishRel(pro_buf);
			if(result)
				PRINT("\r\nPublish Rel����:%d\r\n", result);
			PRINT("- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -\r\n");
			PRINT("----------------------------------------------------------------------\r\n\r\n");
		
		break;
		
		case MQTT_PKT_PUBCOMP:
		
			PRINT("\r\n----------------------------------------------------------------------\r\n");
			PRINT("- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -\r\n");
			result = Protocol_Parser_PublishComp(pro_buf);
			if(result)
				PRINT("\r\nPublish Comp����:%d\r\n", result);
			PRINT("- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -\r\n");
			PRINT("----------------------------------------------------------------------\r\n\r\n");
		
		break;
#endif
		
		default:
		break;
	}
	
	return 0;
}
