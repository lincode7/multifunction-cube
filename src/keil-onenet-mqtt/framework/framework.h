#ifndef _FRAMEWORK_H_
#define _FRAMEWORK_H_





//=============================配置==============================
#define FW_MAX_TASKS		16			//最大任务数

#define FW_RUN_HZ			200			//运行频率
//==========================================================


#define FW_NULL			(void *)0


typedef void (*task_fun)(void);


enum RUN_STA
{

	FW_RUNNING = 0,
	FW_READY,
	FW_SUSPEND,

};


struct FW_TASK_TCB
{

	task_fun task;
	
	unsigned int ticks;
	
	unsigned int ticks_count;
	
	unsigned char status;
	
	struct FW_TASK_TCB *next;

};


typedef struct
{
	
	unsigned int fw_ticks;
	
	unsigned char task_cnt;

	struct FW_TASK_TCB *fw_task_ctb_h, *fw_task_ctb_e;

} FW_TASK_INFO;


void FW_Init(void);

unsigned char FW_CreateTask(task_fun task, unsigned short ticks);

void FW_StartSchedule(void);

unsigned int FW_GetTicks(void);


#endif
