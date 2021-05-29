//单片机头文件
#include "stm32f10x.h"

//单片机相关组件
#include "mcu_gpio.h"

//框架
#include "framework.h"

//网络协议层
#include "fault.h"
#include "onenet.h"

//网络设备
#include "net_device.h"

//网络任务
#include "net_task.h"

//驱动
#include "delay.h"
#include "hwtimer.h"
#include "i2c.h"
#include "iwdg.h"
#include "rtc.h"
#include "usart.h"

//硬件
#include "adxl362.h"
#include "at24c02.h"
#include "beep.h"
#include "ir.h"
#include "key.h"
#include "lcd1602.h"
#include "led.h"
#include "light.h"
#include "nec.h"
#include "sht20.h"
#include "spilcd.h"

//中文数据流
#include "dataStreamName.h"

//图片
#include "image_2k.h"

//字库
#include "font.h"

// TLSF动态内存管理算法
#include "tlsf.h"

// C库
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

//自定义任务
#include "mytask.h"

#define SPILCD_EN 0 // 1-使用SPILCD		0-使用LCD1602

#define TLSF_MEM_POOL_SIZE 1024 * 15

static unsigned char tlsf_mem_pool[TLSF_MEM_POOL_SIZE];

char myTime[20];

//自定义任务信息
MyTaskInfo task_info;

DATA_LBS data_lbs = {"", "", 0, 0};

//数据流
DATA_STREAM data_stream[] = {
    // 灯
    {ZW_REDLED, &led_status.led_status[0], TYPE_BOOL, 1},
    {ZW_GREENLED, &led_status.led_status[1], TYPE_BOOL, 1},
    {ZW_YELLOWLED, &led_status.led_status[2], TYPE_BOOL, 1},
    {ZW_BLUELED, &led_status.led_status[3], TYPE_BOOL, 1},
     // 蜂鸣器
    {ZW_BEEP, &beep_info.beep_status[0], TYPE_BOOL, 1},
    // 温湿度
    {ZW_TEMPERATURE, &sht20_info.tempreture, TYPE_FLOAT, 1},
    {ZW_HUMIDITY, &sht20_info.humidity, TYPE_FLOAT, 1},
    // 加速度
    {ZW_X, &adxl362_info.x, TYPE_FLOAT, 1},
    {ZW_Y, &adxl362_info.y, TYPE_FLOAT, 1},
    {ZW_Z, &adxl362_info.z, TYPE_FLOAT, 1},
    // 光强
    {ZW_LIGHT, &light_info.voltag, TYPE_FLOAT, 1},

    {ZW_BG, &spilcd_info.blSta, TYPE_UCHAR, 0},

    {ZW_TIME, myTime, TYPE_STRING, 1},

    {"GPS", &gps, TYPE_GPS, 0},
    {"$OneNET_LBS", &data_lbs, TYPE_LBS, 0}, //$OneNET_LBS 为OneNET-LBS专用

    {ZW_SIGNAL, &net_device_info.signal, TYPE_CHAR, 0},
    // 错误类型
    {ZW_ERRTYPE, &net_fault_info.net_fault_level_r, TYPE_UCHAR, 1},
    // 当前朝向
    {FACE_MODE, &task_info.curTop, TYPE_UCHAR, 1},
    // 计时器
    {TIMER_S, &task_info.timer_count, TYPE_STRING, 1},
    // 温度报警
    {TEMPERATURE_TIPS, &task_info.tempreture_tips, TYPE_STRING, 1},
};
unsigned char data_stream_cnt = sizeof(data_stream) / sizeof(data_stream[0]);

/*
************************************************************
*	函数名称：	Hardware_Init
*
*	函数功能：	硬件初始化
*
*	入口参数：	无
*
*	返回参数：	无
*
*	说明：		初始化单片机功能以及外接设备
************************************************************
*/
void Hardware_Init(void)
{

    NVIC_PriorityGroupConfig(NVIC_PriorityGroup_2); //中断控制器分组设置

    Delay_Init(); // systick初始化

    Usart1_Init(115200); //初始化串口   115200bps
#if (USART_DMA_RX_EN)
    USARTx_ResetMemoryBaseAddr(USART_DEBUG, (unsigned int)alter_info.alter_buf,
                               sizeof(alter_info.alter_buf), USART_RX_TYPE);
#endif

    init_memory_pool(TLSF_MEM_POOL_SIZE, tlsf_mem_pool); //内存池初始化

    LED_Init(); // LED初始化

    KEY_Init(); //按键初始化

    BEEP_Init(); //蜂鸣器初始化

    IIC_Init(I2C2); // IIC总线初始化

    LIGHT_Init(); //光敏电阻初始化

    IR_Init(38000); //红外发射管初始化

#if (SPILCD_EN == 1)
    SPILCD_Init(); // SPILCD初始化
#else
    LCD1602_Init(); // LCD1602初始化
#endif

    RTC_Init(); //初始化RTC

    UsartPrintf(USART_DEBUG, "EEPROM: %s\r\n",
                AT24C02_Exist() ? "Ok" : "Err"); // EEPROM检测

    UsartPrintf(USART_DEBUG, "SHT20: %s\r\n",
                SHT20_Exist() ? "Ok" : "Err"); // SHT20检测

    UsartPrintf(USART_DEBUG, "ADXL362: %s\r\n",
                ADXL362_Init() ? "Ok" : "Err"); // ADXL362检测

    if (RCC_GetFlagStatus(RCC_FLAG_IWDGRST) == SET) //如果是看门狗复位则提示
    {
        UsartPrintf(USART_DEBUG, "WARN:	IWDG Reboot\r\n");

        RCC_ClearFlag(); //清除看门狗复位标志位

        net_fault_info.net_fault_level = net_fault_info.net_fault_level_r =
            NET_FAULT_LEVEL_5; //错误等级5

        net_device_info.reboot = 1;
    }
    else
    {
        UsartPrintf(USART_DEBUG,
                    "DEVID: %s,	APIKEY: %s\r\nPROID:%s,	AUIF:%s\r\n",
                    onenet_info.dev_id, onenet_info.api_key, onenet_info.pro_id,
                    onenet_info.auif);

        net_device_info.reboot = 0;
    }

    // Iwdg_Init(4, 1250);
    // //64分频，每秒625次，重载1250次，2s

    Timer_X_Init(TIM6, 49, 35999, 1, 0); // 72MHz，36000分频-500us，50重载值。则中断周期为500us * 50 = 25ms

    // 呼吸灯准备
    Timer_X_PWM_Init(TIM8, 2 | 4, TIM_OCMode_PWM1, GPIOC, GPIO_Pin_7 | GPIO_Pin_8, 71, 250);

    UsartPrintf(USART_DEBUG, "Hardware init OK\r\n"); //提示初始化完成
}

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
void NET_Event_CallBack(NET_EVENT net_event)
{

    switch ((unsigned char)net_event)
    {
    case NET_EVENT_Timer_Check_Err: //网络定时检查超时错误
        UsartPrintf(USART_DEBUG, "Tips:	Timer Check Err\r\n");
        break;

    case NET_EVENT_Timer_Send_Err: //网络发送失败错误
        UsartPrintf(USART_DEBUG, "Tips:	Timer Check Err-Send\r\n");
        break;

    case NET_EVENT_Send_HeartBeat: //即将发送心跳包
        break;

    case NET_EVENT_Send_Data: //即将发送数据点
        break;

    case NET_EVENT_Send_Subscribe: //即将发送订阅数据
        break;

    case NET_EVENT_Send_UnSubscribe: //即将发送取消订阅数据
        break;

    case NET_EVENT_Send_Publish: //即将发送推送数据
        break;

    case NET_EVENT_Send: //开始发送数据
        break;

    case NET_EVENT_Recv: // Modbus用-收到数据查询指令
        break;

    case NET_EVENT_Check_Status: //进入网络模组状态检查
        break;

    case NET_EVENT_Device_Ok: //网络模组检测Ok
        UsartPrintf(USART_DEBUG, "NET Device :Ok\r\n");
        break;
    case NET_EVENT_Device_Err: //网络模组检测错误
        UsartPrintf(USART_DEBUG, "NET Device :Error\r\n");
        break;

    case NET_EVENT_Initialize: //正在初始化网络模组
#if (SPILCD_EN == 1)
        SPILCD_DisZW(0, 80, BLUE, lian);
        SPILCD_DisZW(16, 80, BLUE, jie);
        SPILCD_DisZW(32, 80, BLUE, zhong);
#endif
        break;

    case NET_EVENT_Init_Ok: //网络模组初始化成功
        break;

    case NET_EVENT_Auto_Create_Ok: //自动创建设备成功
        UsartPrintf(USART_DEBUG, "Tips:	Auto Create Device Ok\r\n");
        break;

    case NET_EVENT_Auto_Create_Err: //自动创建设备失败
        UsartPrintf(USART_DEBUG, "WARN:	Auto Create Device Err\r\n");
        break;

    case NET_EVENT_Connect: //正在连接、登录OneNET
        break;

    case NET_EVENT_Connect_Ok: //连接、登录成功
        BEEP_Ctl(0, BEEP_ON);
        DelayXms(200);
        BEEP_Ctl(0, BEEP_OFF);

#if (SPILCD_EN == 1)
        SPILCD_DisZW(0, 80, BLUE, yi);
        SPILCD_DisZW(16, 80, BLUE, lian);
        SPILCD_DisZW(32, 80, BLUE, jie);
#endif
        if (gps.flag)
            data_stream[12].flag = 1; // GPS就绪，准备上传

#if (LBS_EN == 1)
        if (lbs_info.lbs_ok == 1)
        {
            strncpy(data_lbs.cell_id, lbs_info.cell_id,
                    strlen(lbs_info.cell_id));
            strncpy(data_lbs.lac, lbs_info.lac, strlen(lbs_info.lac));
            data_lbs.network_type = lbs_info.network_type;
            data_lbs.flag = lbs_info.flag;

            data_stream[13].flag = 1;
        }
#endif
        break;

    case NET_EVENT_Connect_Err: //连接、登录错误
        BEEP_Ctl(0, BEEP_ON);
        DelayXms(500);
        BEEP_Ctl(0, BEEP_OFF);

#if (SPILCD_EN == 1)
        SPILCD_DisZW(0, 80, BLUE, wei);
        SPILCD_DisZW(16, 80, BLUE, lian);
        SPILCD_DisZW(32, 80, BLUE, jie);
#endif
        break;

    case NET_EVENT_Fault_Process: //错误处理
#if (SPILCD_EN == 1)
        SPILCD_DisZW(16, 80, BLUE, duan);
        SPILCD_DisZW(32, 80, BLUE, kai);
#endif
        UsartPrintf(USART_DEBUG, "WARN:	NET Fault Process\r\n");
        break;

    default: //无
        break;
    }
}

/*
************************************************************
*	函数名称：	KEY_Task
*
*	函数功能：	扫描按键是否按下，如果有按下，进行对应的处理
*
*	入口参数：	无
*
*	返回参数：	无
*
*	说明：		按键任务
************************************************************
*/
void KEY_Task(void)
{

    static unsigned char key_event_r = KEY_NONE;

    key_event_r = Keyboard();

    //单击判断-------------------------------------------------------
    if (key_event_r == key_event[0][KEY_X_DOWN])
    {
        if (led_status.led_status[0] == LED_OFF)
            LED_Ctl(0, LED_ON);
        else
            LED_Ctl(0, LED_OFF);

        onenet_info.send_data = SEND_TYPE_DATA;
    }
    else if (key_event_r == key_event[1][KEY_X_DOWN])
    {
        if (led_status.led_status[1] == LED_OFF)
            LED_Ctl(1, LED_ON);
        else
            LED_Ctl(1, LED_OFF);

        onenet_info.send_data = SEND_TYPE_DATA;
    }
    else if (key_event_r == key_event[2][KEY_X_DOWN])
    {
        if (led_status.led_status[2] == LED_OFF)
            LED_Ctl(2, LED_ON);
        else
            LED_Ctl(2, LED_OFF);

        onenet_info.send_data = SEND_TYPE_DATA;
    }
    else if (key_event_r == key_event[3][KEY_X_DOWN])
    {
        if (led_status.led_status[3] == LED_OFF)
            LED_Ctl(3, LED_ON);
        else
            LED_Ctl(3, LED_OFF);

        onenet_info.send_data = SEND_TYPE_DATA;
    }

    //双击判断-------------------------------------------------------
    if (key_event_r == key_event[0][KEY_X_DOUBLE])
    {
        // 关闭倒计时
        if (task_info.timer_s != TIMER_OFF)
            task_info.timer_s = TIMER_OFF;
        // 启用倒计时
        else if (task_info.timer_s == TIMER_OFF)
            task_info.timer_s = TIMER_READY;
    }

    //长按判断-------------------------------------------------------
    if (key_event_r == key_event[0][KEY_X_DOWNLONG])
    {
        onenet_info.file_bin_name = "image";
        onenet_info.file_bin = Array;
        onenet_info.file_bin_size = sizeof(Array);

        onenet_info.send_data = SEND_TYPE_BINFILE;
    }
}

/*
************************************************************
*	函数名称：	SENSOR_Task
*
*	函数功能：	传感器数据采集、显示
*
*	入口参数：	无
*
*	返回参数：	无
*
*	说明：		传感器数据采集任务。进行外接传感器的数据采集、读取、显示
************************************************************
*/
void SENSOR_Task(void)
{

    static unsigned char count = 0;

    if (adxl362_info.device_ok) //只有设备存在时，才会读取值和显示
    {
        ADXL362_GetValue(); //采集传感器数据

#if (SPILCD_EN == 1)
        SPILCD_DisString(0, 16, 16, BLUE, 1, "X%0.1f,Y%0.1f,Z%0.1f    ",
                         adxl362_info.x, adxl362_info.y, adxl362_info.z);
#else
        LCD1602_DisString(0x80, "X%0.1f,Y%0.1f,Z%0.1f", adxl362_info.x,
                          adxl362_info.y, adxl362_info.z);
#endif
    }

    if (sht20_info.device_ok) //只有设备存在时，才会读取值和显示
    {
        SHT20_GetValue(); //采集传感器数据

#if (SPILCD_EN == 1)
        SPILCD_DisString(0, 48, 16, BLUE, 1, "%0.1fC,%0.1f%%    ",
                         sht20_info.tempreture, sht20_info.humidity);
#else
        LCD1602_DisString(0xC0, "%0.1fC,%0.1f%%", sht20_info.tempreture,
                          sht20_info.humidity);
#endif
    }

    LIGHT_GetVoltag();

#if (SPILCD_EN == 1)
    SPILCD_DisString(95, 80, 16, BLUE, 1, "%0.2f%", light_info.voltag);
#else
    LCD1602_DisString(0xCC, "%0.2f%", light_info.voltag);
#endif

    if (++count >= 10) //每隔一段时间发送一次红外数据
    {
        count = 0;

        if (NET_DEVICE_GetSignal())
            data_stream[14].flag = 1;
        else
            data_stream[14].flag = 0;

        // NEC_SendData(0, send_data++);
    }
}

/*
************************************************************
*	函数名称：	CLOCK_Task
*
*	函数功能：	网络校时、时间显示
*
*	入口参数：	无
*
*	返回参数：	无
*
*	说明：
************************************************************
*/
void CLOCK_Task(void)
{

#if (NET_TIME_EN == 1)
    static unsigned int
        second = 0,
        second_pre = 0,
        err_count =
            0; // second是实时时间，second_pre差值比较。err_count获取计时
    static struct tm *time;
    static _Bool get_net_time = 1;
#endif

#if (NET_TIME_EN == 1)
    if (get_net_time) //需要获取时间
    {
        data_stream[11].flag = 0; //不上传时间

        if (FW_GetTicks() - err_count >=
            24000) //十分钟还获取不到则重新获取(25ms一次)
        {
            err_count = FW_GetTicks();
            net_device_info.net_time = 0;
            onenet_info.net_work = 0;
            NET_DEVICE_ReConfig(0);
            onenet_info.connect_ip = 0;
        }

        if (net_device_info.net_time)
        {
            second = RTC_GetCounter();

            if (((net_device_info.net_time <= second + 300) &&
                 (net_device_info.net_time >= second - 300)) ||
                (second <= 100))
            { //如果在±5分钟内，则认为时间正确
                RTC_SetTime(net_device_info.net_time +
                            4); //设置RTC时间，加4是补上大概的时间差

                get_net_time = 0;
                err_count = 0;

                data_stream[11].flag = 1; //上传时间
            }
        }
    }

    second = RTC_GetCounter(); //获取秒值

    if (second > second_pre)
    {
        second_pre = second;
        time =
            localtime((const time_t *)&second); //将秒值转为tm结构所表示的时间

        memset(myTime, 0, sizeof(myTime));
        snprintf(myTime, sizeof(myTime), " %d-%d %d:%d:%d",
                 time->tm_mon + 1, time->tm_mday,
                 time->tm_hour, time->tm_min, time->tm_sec);

        if (time->tm_hour == 0 && time->tm_min == 0 &&
            time->tm_sec == 0) //每天0点时，更新一次时间
        {
            get_net_time = 1;
            err_count = FW_GetTicks();
            net_device_info.net_time = 0;
            onenet_info.net_work = 0;
            NET_DEVICE_ReConfig(0);
            onenet_info.connect_ip = 0;
        }
    }
#endif
}

// 自定义任务
void init_mytask(void);
void BREATH_Task(void);
void COUNT_DOWN_Task(void);
void MY_Task(void);

/*
************************************************************
*	函数名称：	main
*
*	函数功能：
*
*	入口参数：	无
*
*	返回参数：	0
*
*	说明：
************************************************************
*/
int main(void)
{

    Hardware_Init(); //硬件初始化

#if (SPILCD_EN == 1)
    SPILCD_Clear(BGC); //清屏

    //标题显示
    SPILCD_DisZW(0, 0, RED, san);   //显示“三”
    SPILCD_DisZW(16, 0, RED, zhou); //显示“轴”

    SPILCD_DisZW(0, 32, RED, wen);  //显示“温”
    SPILCD_DisZW(16, 32, RED, shi); //显示“湿”
    SPILCD_DisZW(32, 32, RED, du);  //显示“度”

    SPILCD_DisZW(96, 64, RED, guang); //显示“光”
    SPILCD_DisZW(112, 64, RED, min);  //显示“敏”

    SPILCD_DisZW(0, 64, RED, zhuang); //显示“状”
    SPILCD_DisZW(16, 64, RED, tai);   //显示“态”
#else
    LCD1602_Clear(0xff); //清屏
#endif

    NET_DEVICE_IO_Init(); //网络设备IO初始化
    NET_DEVICE_Reset();   //网络设备复位

    FW_Init(); //框架层初始化
    //创建任务
    FW_CreateTask(KEY_Task, 15);

    // FW_CreateTask(SENSOR_Task, 100);

    init_mytask(); //初始化自定义任务

    FW_CreateTask(CLOCK_Task, 15); //获取网络时间 最小单位分

    NET_Task_Init();

    UsartPrintf(USART_DEBUG, "Running...\r\n");

    FW_StartSchedule(); //开始任务调度
}

/*
************************************************************
*	函数名称：	TIM6_IRQHandler
*
*	函数功能：	Timer6中断
*
*	入口参数：	无
*
*	返回参数：	无
*
*	说明：
************************************************************
*/
void TIM6_IRQHandler(void)
{

    if (TIM_GetITStatus(TIM6, TIM_IT_Update) == SET)
    {
        TIM_ClearITPendingBit(TIM6, TIM_IT_Update);

        OneNET_CmdHandle();
    }
}

/*实现myTask.h*/

// 识别姿态
void getCurTop(void)
{
    if (adxl362_info.device_ok) //加速度传感器正常运行
    {
        ADXL362_GetValue(); //采集传感器数据
        float x = adxl362_info.x;
        float y = adxl362_info.y;
        float z = adxl362_info.z;

        // task_info.curTop = 'TOP';
        if (fabs(x - 0.0) < 0.2 && fabs(y - 0.0) < 0.2 && fabs(z - 1.2) < 0.1)
        {
            task_info.curTop = TOP;
        }
        else if (fabs(x - 0.0) < 0.3 && fabs(y - 0.0) < 0.3 &&
                 fabs(z + 1.0) < 0.2)
        {
            task_info.curTop = BOTTOM;
        }
        else if (fabs(x - 0.0) < 0.2 && fabs(y - 1.1) < 0.1 &&
                 fabs(z - 0.1) < 0.3)
        {
            task_info.curTop = FONT;
        }
        else if (fabs(x - 0.0) < 0.1 && fabs(y + 1.1) < 0.1 &&
                 fabs(z - 0.1) < 0.3)
        {
            task_info.curTop = BACK;
        }
        else if (fabs(x - 1.1) < 0.1 && fabs(y - 0.0) < 0.2 &&
                 fabs(z - 0.0) < 0.3)
        {
            task_info.curTop = LEFT;
        }
        else if (fabs(x + 1.1) < 0.1 && fabs(y - 0.0) < 0.2 &&
                 fabs(z - 0.0) < 0.3)
        {
            task_info.curTop = RIGHT;
        }
    }
}
/*tp 呼吸模式 st_pwm 呼吸启示状态 h_x 呼吸*/
void breath(unsigned char tp, unsigned char st_pwm, unsigned char h_x)
{
    // 只有在呼吸模式切换时才进行配置
    if (task_info.breathled != tp)
    {
        st_pwm = st_pwm == 250 ? 249 : st_pwm;
        st_pwm = st_pwm == 0 ? 1 : st_pwm;
        if (tp == BREATH_RED)
        {
            led_pwm.redpwm = st_pwm;
            task_info.redbreathflag = h_x;
            led_pwm.greenpwm = 0;
        }
        else if (tp == BREATH_GREEN)
        {
            led_pwm.greenpwm = st_pwm;
            task_info.greenbreathflag = h_x;
            led_pwm.redpwm = 0;
        }
        else if (tp == BREATH_RED_GREEN)
        {
            led_pwm.redpwm = st_pwm;
            task_info.greenbreathflag = h_x;
            led_pwm.greenpwm = st_pwm;
            task_info.redbreathflag = h_x;
        }
        else if (tp == BREATH_RED_AFFTER_GREEN)
        {
            led_pwm.redpwm = st_pwm;
            task_info.redbreathflag = h_x;
            led_pwm.greenpwm = 250 - st_pwm;
            task_info.greenbreathflag = 1 - h_x;
        }

        task_info.breathled = tp;
    }
}
/*s 倒计时时间*/
void timer(unsigned int s)
{
    // 只有在倒计时准备好时才进行配置，按键2启用
    if (task_info.timer_s == TIMER_READY)
    {
        task_info.timer_s = s;
    }
    // 格式化数据
    snprintf(task_info.timer_count, sizeof(task_info.timer_count), "%02d:%02d:%02d", task_info.timer_s / 3600, (task_info.timer_s % 3600) / 60, task_info.timer_s % 60);
}
/*ms 蜂鸣频率  times 蜂鸣次数*/
void beep(unsigned short ms, int times)
{
    for (int i = 0; i < times; i++)
    {
        BEEP_Ctl(0, BEEP_ON);
        DelayXms(ms);
        BEEP_Ctl(0, BEEP_OFF);
    }
}
// 温湿度
void SHT(void)
{
    if (sht20_info.device_ok) //只有设备存在时，才会读取值和显示
    {
        SHT20_GetValue(); //采集传感器数据
    }
}
// 光敏
void light(void) { LIGHT_GetVoltag(); }
// 各姿态任务
void fuc_top(void)
{
    breath(BREATH_GREEN, 250, 1); // 启用呼吸灯
    SHT();                        // 启用温湿度
}
void fuc_bottom(void)
{
    breath(BREATH_RED_AFFTER_GREEN, 250, 1); // 启用呼吸灯
    timer(300);                              // 启用倒计时
}
void fuc_left(void)
{
    breath(BREATH_RED_AFFTER_GREEN, 250, 1); // 启用呼吸灯
    timer(30);                               // 启用倒计时
}
void fuc_right(void)
{
    breath(BREATH_RED_AFFTER_GREEN, 250, 1); // 启用呼吸灯
    timer(60);                               // 启用倒计时
}
void fuc_front(void)
{
    task_info.breathled = BREATH_OFF; // 关闭呼吸灯
    task_info.timer_s = TIMER_OFF;    // 关闭倒计时
    light();                          // 启用光敏
    // 控制蓝灯
}
void fuc_back(void)
{
    task_info.breathled = BREATH_OFF; // 关闭呼吸灯
    task_info.timer_s = TIMER_OFF;    // 关闭倒计时
    SHT();                            // 启用温湿度
}
// 分配姿态任务
void menu(void)
{
    switch (task_info.curTop)
    {
    case TOP:
        fuc_top();
        break;
    case BOTTOM:
        fuc_bottom();
        break;
    case LEFT:
        fuc_left();
        break;
    case RIGHT:
        fuc_right();
        break;
    case FONT:
        fuc_front();
        break;
    case BACK:
        fuc_back();
        break;
    default:
        break;
    }
}
// 显示
void lcdshow(void)
{
    switch (task_info.curTop)
    {
    case TOP:
#if (SPILCD_EN == 0)
        LCD1602_Clear(0xff); //清屏
        LCD1602_DisString(0x80, "%s", myTime);
        LCD1602_DisString(0xC0, "T:%0.1f,H:%0.1f", sht20_info.tempreture,
                          sht20_info.humidity);
#endif
        break;
    case BOTTOM:
#if (SPILCD_EN == 0)
        LCD1602_Clear(0xff); //清屏
        LCD1602_DisString(0x80, "%s", myTime);
        LCD1602_DisString(0xC0, "COUNT_D:%s", task_info.timer_count);
#endif
        break;
    case LEFT:
#if (SPILCD_EN == 0)
        LCD1602_Clear(0xff); //清屏
        LCD1602_DisString(0x80, "%s", myTime);
        LCD1602_DisString(0xC0, "COUNT_D:%s", task_info.timer_count);
#endif
        break;
    case RIGHT:
#if (SPILCD_EN == 0)
        LCD1602_Clear(0xff); //清屏
        LCD1602_DisString(0x80, "%s", myTime);
        LCD1602_DisString(0xC0, "COUNT_D:%s", task_info.timer_count);
#endif
        break;
    case FONT:
#if (SPILCD_EN == 0)
        LCD1602_Clear(0xff); //清屏
        LCD1602_DisString(0x80, "%s", myTime);
        LCD1602_DisString(0xC0, "Light:%0.2f%", light_info.voltag);
#endif
        break;
    case BACK:
#if (SPILCD_EN == 0)
        LCD1602_Clear(0xff); //清屏
        LCD1602_DisString(0x80, "%s", myTime);
        LCD1602_DisString(0xC0, "T:%0.1f,H:%0.1f", sht20_info.tempreture,
                          sht20_info.humidity);
        if (sht20_info.tempreture < 8.0)
        {
            LCD1602_DisString(0xCD, "!!!");
            snprintf(task_info.tempreture_tips, sizeof(task_info.tempreture_tips), "too cold");
        }
        else if (sht20_info.tempreture > 28.0)
        {
            LCD1602_DisString(0xCD, "!!!");
            snprintf(task_info.tempreture_tips, sizeof(task_info.tempreture_tips), "too hot");
        }
#endif
        break;
    default:
        break;
    }
}

// 自定义任务
void init_mytask(void)
{
    task_info.curTop = UNKONWN;
    task_info.breathled = BREATH_OFF;
    task_info.redbreathflag = 1;
    task_info.greenbreathflag = 1;
    task_info.timer_s = TIMER_OFF;
    memset(&task_info.timer_count, 0, sizeof(task_info.timer_count));
    snprintf(task_info.timer_count, sizeof(task_info.timer_count), "00:00:00");
    memset(&task_info.tempreture_tips, 0, sizeof(task_info.tempreture_tips));

    FW_CreateTask(MY_Task, 100);        //将自定义任务加入任务调度
    FW_CreateTask(BREATH_Task, 5);      //将自定义任务加入任务调度
    FW_CreateTask(COUNT_DOWN_Task, 20); //将自定义任务加入任务调度
}
/* 呼吸灯任务 */
void BREATH_Task(void)
{
    if (task_info.breathled != BREATH_OFF)
    {

        if (task_info.breathled == BREATH_RED || task_info.breathled == BREATH_RED_GREEN ||
            task_info.breathled == BREATH_RED_AFFTER_GREEN)
        {
            if (task_info.redbreathflag)
            {
                led_pwm.redpwm += 5;
                if (led_pwm.redpwm >= 250)
                {
                    led_pwm.redpwm = 250;
                    task_info.redbreathflag = 0;
                }
            }
            else
            {
                led_pwm.redpwm -= 5;
                if (led_pwm.redpwm <= 1)
                {
                    led_pwm.redpwm = 1;
                    task_info.redbreathflag = 1;
                }
            }
        }
        if (task_info.breathled == BREATH_GREEN || task_info.breathled == BREATH_RED_GREEN ||
            task_info.breathled == BREATH_RED_AFFTER_GREEN)
        {
            if (task_info.greenbreathflag)
            {
                led_pwm.greenpwm += 5;
                if (led_pwm.greenpwm >= 250)
                {
                    led_pwm.greenpwm = 250;
                    task_info.greenbreathflag = 0;
                }
            }
            else
            {
                led_pwm.greenpwm -= 5;
                if (led_pwm.greenpwm <= 1)
                {
                    led_pwm.greenpwm = 1;
                    task_info.greenbreathflag = 1;
                }
            }
        }
        TIM_SetCompare2(TIM8, led_pwm.redpwm);
        TIM_SetCompare3(TIM8, led_pwm.greenpwm);
    }
    else
    {
        if (led_status.led_status[0] == LED_OFF)
        {
            LED_Ctl(0, LED_OFF);
            led_pwm.redpwm = 0;
        }
        if (led_status.led_status[1] == LED_OFF)
        {
            LED_Ctl(1, LED_OFF);
            led_pwm.greenpwm = 0;
        }
    }
}
/* 倒计时任务 */
void COUNT_DOWN_Task(void)
{
    if (task_info.timer_s != TIMER_OFF && task_info.timer_s != TIMER_READY)
    {
        --task_info.timer_s;
        DelayXms(1000);
        if (task_info.timer_s == TIMER_OFF)
        {
            beep(50, 5);
            task_info.timer_s = TIMER_OFF; // 避免连续触发
        }
    }
}
/* 传感器任务 */
void MY_Task(void)
{
    getCurTop(); //识别当前开发板姿态
    menu();      //根据当前姿态执行不同功能
    lcdshow();   //执行完功能后根据不同功能显示不同结果
}