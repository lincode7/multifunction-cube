/**
	************************************************************
	************************************************************
	************************************************************
	*	文件名： 	framework.c
	*
	*	作者： 		张继瑞
	*
	*	日期： 		2017-12-11
	*
	*	版本： 		V1.0
	*
	*	说明： 		裸机框架层
	*
	*	修改记录：	
	************************************************************
	************************************************************
	************************************************************
**/

//框架
#include "framework.h"

//硬件驱动
#include "hwtimer.h"

//C库
#include <stdlib.h>


FW_TASK_INFO fw_task_info;


/*
************************************************************
*	函数名称：	FW_Init
*
*	函数功能：	框架初始化
*
*	入口参数：	无
*
*	返回参数：	无
*
*	说明：		
************************************************************
*/
void FW_Init(void)
{

	Timer_X_Init(TIM7, (10000 / FW_RUN_HZ) - 1, 7199, 1, 0);		//72MHz，7200分频-100us
	
}

/*
************************************************************
*	函数名称：	FW_CreateTask
*
*	函数功能：	任务创建
*
*	入口参数：	task：任务指针
*				ticks：任务挂起的周期
*
*	返回参数：	0-成功	1-失败
*
*	说明：		先创建的任务优先级高
************************************************************
*/
unsigned char FW_CreateTask(task_fun task, unsigned short ticks)
{

	struct FW_TASK_TCB *current = (struct FW_TASK_TCB *)malloc(sizeof(struct FW_TASK_TCB));
																//分配内存
	
	if(current == NULL)
		return 1;
	
	if(++fw_task_info.task_cnt > FW_MAX_TASKS)					//最大任务数
	{
		fw_task_info.task_cnt--;
		free(current);
		
		return 2;
	}
	
	if(task == FW_NULL)
	{
		free(current);
		
		return 3;
	}
	
	if(fw_task_info.fw_task_ctb_h == NULL)						//如果head为NULL
		fw_task_info.fw_task_ctb_h = current;					//head指向当前分配的内存区
	else														//如果head不为NULL
		fw_task_info.fw_task_ctb_e->next = current;				//则end指向当前分配的内存区
	
	current->task = task;
	current->ticks = ticks;
	current->ticks_count = fw_task_info.fw_ticks;
	current->status = FW_READY;
	current->next = NULL;										//下一段为NULL
	
	fw_task_info.fw_task_ctb_e = current;						//end指向当前分配的内存区
	
	return 0;

}

/*
************************************************************
*	函数名称：	FW_TickHandler
*
*	函数功能：	
*
*	入口参数：	无
*
*	返回参数：	无
*
*	说明：		
************************************************************
*/
void FW_TickHandler(void)
{
	
	unsigned char i = 0;
	
	struct FW_TASK_TCB *current = fw_task_info.fw_task_ctb_h;

	fw_task_info.fw_ticks++;
	
	for(i = 0; i < fw_task_info.task_cnt; i++)
	{
		if(fw_task_info.fw_ticks >= current->ticks + current->ticks_count)
		{
			current->status = FW_READY;
		}
		
		if(current->next == FW_NULL)
			break;
		else
			current = current->next;
	}

}

/*
************************************************************
*	函数名称：	FW_StartSchedule
*
*	函数功能：	负责各任务的调度
*
*	入口参数：	无
*
*	返回参数：	无
*
*	说明：		总是运行最高优先级的任务
************************************************************
*/
void FW_StartSchedule(void)
{

	unsigned char i = 0;
	
	struct FW_TASK_TCB *current = fw_task_info.fw_task_ctb_h;
	
	while(1)
	{
		for(i = 0; i < fw_task_info.task_cnt; i++)
		{
			if(current->status == FW_READY)
			{
				if(current->task != FW_NULL)
				{
					current->status = FW_RUNNING;
					
					current->task();
					
					current->status = FW_SUSPEND;
					current->ticks_count = fw_task_info.fw_ticks;
					
					current = fw_task_info.fw_task_ctb_h;
					
					break;
				}
			}
			else
			{
				if(fw_task_info.fw_ticks >= current->ticks + current->ticks_count)
				{
					current->status = FW_READY;
				}
				
				if(current->next == FW_NULL)
					current = fw_task_info.fw_task_ctb_h;
				else
					current = current->next;
			}
		}
	}

}

/*
************************************************************
*	函数名称：	FW_GetTicks
*
*	函数功能：	获取当前ticks
*
*	入口参数：	无
*
*	返回参数：	ticks
*
*	说明：		总是运行最高优先级的任务
************************************************************
*/
unsigned int FW_GetTicks(void)
{

	return fw_task_info.fw_ticks;

}

/*
************************************************************
*	函数名称：	TIM7_IRQHandler
*
*	函数功能：	Timer7更新中断服务函数
*
*	入口参数：	无
*
*	返回参数：	无
*
*	说明：		
************************************************************
*/
void TIM7_IRQHandler(void)
{

	if(TIM_GetITStatus(TIM7, TIM_IT_Update) == SET)
	{
		
		TIM7->SR &= 0xFFFE;
		
		FW_TickHandler();
	}

}
