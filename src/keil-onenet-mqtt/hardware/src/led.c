/**
	************************************************************
	************************************************************
	************************************************************
	*	�ļ����� 	led.c
	*
	*	���ߣ� 		�ż���
	*
	*	���ڣ� 		2017-02-27
	*
	*	�汾�� 		V1.0
	*
	*	˵���� 		LED��ʼ��������LED
	*
	*	�޸ļ�¼��	
	************************************************************
	************************************************************
	************************************************************
**/

//��Ƭ��ͷ�ļ�
#include "stm32f10x.h"

//��Ƭ��������
#include "mcu_gpio.h"

//����
#include "delay.h"
#include "led.h"

LED_STATUS led_status;

LED_PWM led_pwm;

const static GPIO_LIST led_gpio_list[LED_NUM] = {
	{GPIOC, GPIO_Pin_7, "led1"},
	{GPIOC, GPIO_Pin_8, "led2"},
	{GPIOA, GPIO_Pin_12, "led3"},
	{GPIOC, GPIO_Pin_10, "led4"},
};

/*
************************************************************
*	�������ƣ�	LED_Init
*
*	�������ܣ�	LED��ʼ��
*
*	��ڲ�����	��
*
*	���ز�����	��
*
*	˵����		�ߵ�ƽ�ص�		�͵�ƽ����
************************************************************
*/
void LED_Init(void)
{

	unsigned char i = 0;

	for (; i < LED_NUM; i++)
		MCU_GPIO_Init(led_gpio_list[i].gpio_group, led_gpio_list[i].gpio_pin, GPIO_Mode_Out_PP, GPIO_Speed_50MHz, led_gpio_list[i].gpio_name);

	LED_Ctl(LED_ALL, LED_OFF);

	led_pwm.redpwm = 0;
	led_pwm.greenpwm = 0;

}

/*
************************************************************
*	�������ƣ�	LED_Ctl
*
*	�������ܣ�	LED����
*
*	��ڲ�����	num��LED���
*			   status������״̬
*
*	���ز�����	��
*
*	˵����		
************************************************************
*/
void LED_Ctl(unsigned char num, LED_ENUM status)
{

	unsigned char i = 0;

	if (num < LED_NUM)
	{
		MCU_GPIO_Output_Ctl(led_gpio_list[num].gpio_name, !status);
		led_status.led_status[num] = status;
		if (num == 0 && status == LED_ON)
			led_pwm.redpwm = 250;
		if (num == 0 && status == LED_OFF)
			led_pwm.redpwm = 0;
		if (num == 1 && status == LED_ON)
			led_pwm.greenpwm = 250;
		if (num == 1 && status == LED_OFF)
			led_pwm.greenpwm = 0;
	}
	else if (num == LED_ALL)
	{
		for (; i < LED_NUM; i++)
		{
			MCU_GPIO_Output_Ctl(led_gpio_list[i].gpio_name, !status);
			led_status.led_status[i] = status;
		}
	}
}
