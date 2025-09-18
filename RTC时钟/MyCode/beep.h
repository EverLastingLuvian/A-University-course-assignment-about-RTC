#ifndef __BEEP_H
#define __BEEP_H
#include "sys.h"

//////////////////////////////////////////////////////////////////////////////////	 
// 本文件用于 ALIENTEK STM32F407 开发板
// 蜂鸣器驱动程序
// 作者：ALIENTEK
// 官网：www.openedv.com
// 创建日期：2017/4/6
// 版本：V1.0
// 版权所有：ALIENTEK 2014-2024
// All rights reserved									  
////////////////////////////////////////////////////////////////////////////////// 	

// 定义蜂鸣器控制宏
#define BEEP PFout(8)  // 定义蜂鸣器控制的 GPIO 引脚为 PF8

// 初始化蜂鸣器
void BEEP_Init(void);

// 打开蜂鸣器
void BEEP_On(void);

// 关闭蜂鸣器
void BEEP_Off(void);

// 蜂鸣器任务，用于控制蜂鸣器的持续时间
void Alarm_BEEP_Task(void);

#endif









