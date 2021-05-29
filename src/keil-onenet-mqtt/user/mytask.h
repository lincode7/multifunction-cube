#ifndef _MYTASK_H_
#define _MYTASK_H_

//当下板子朝上一面
#define UNKONWN -1
#define TOP 0
#define BOTTOM 1
#define LEFT 2
#define RIGHT 3
#define FONT 4
#define BACK 5
// 呼吸灯模式
#define BREATH_OFF 0
#define BREATH_RED 1
#define BREATH_GREEN 2
#define BREATH_RED_GREEN 3
#define BREATH_RED_AFFTER_GREEN 4
// 倒计时
#define TIMER_OFF 0
#define TIMER_READY -1

typedef struct
{
    unsigned char curTop;          // top bottom font back left right
    unsigned char redbreathflag;   // 红呼吸灯标志
    unsigned char greenbreathflag; // 绿呼吸灯标志
    unsigned char breathled;       // 呼吸灯的根据，0关闭，1红灯，2绿灯，3红绿同时，4红绿交替
    unsigned int timer_s;          // 倒计时时间，秒为单位
    char timer_count[9];           // 格式化倒计时时间 00:00:00
    char tempreture_tips[9];       // 温度报警提示 too hot/cold
} MyTaskInfo;
extern MyTaskInfo task_info;

void breath(unsigned char tp, unsigned char st_pwm, unsigned char h_x);
void getCurTop(void);
void menu(void);
void fuc_top(void);
void fuc_bottom(void);
void fuc_front(void);
void fuc_back(void);
void fuc_left(void);
void fuc_right(void);
void timer(unsigned int s);
void SHT(void);
void light(void);
void beep(unsigned short ms, int times);
void lcdshow(void);

#endif