#include "beep.h"
#include "RTC.h"
#include "lcd.h"

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

extern uint32_t beep_start_time;  // 蜂鸣器开始时间
extern uint8_t beep_flag;         // 蜂鸣器标志位
extern RTC_HandleTypeDef RTC_Handler; // RTC 句柄

// 初始化蜂鸣器
// 配置 PF8 为推挽输出，用于控制蜂鸣器
void BEEP_Init(void)
{
    GPIO_InitTypeDef GPIO_Initure;
    __HAL_RCC_GPIOF_CLK_ENABLE();  // 使能 GPIOF 时钟
	
    GPIO_Initure.Pin = GPIO_PIN_8;  // PF8
    GPIO_Initure.Mode = GPIO_MODE_OUTPUT_PP;  // 推挽输出
    GPIO_Initure.Pull = GPIO_PULLUP;  // 上拉
    GPIO_Initure.Speed = GPIO_SPEED_HIGH;  // 高速
    HAL_GPIO_Init(GPIOF, &GPIO_Initure);
	
    HAL_GPIO_WritePin(GPIOF, GPIO_PIN_8, GPIO_PIN_RESET);  // 默认关闭蜂鸣器
}

// 打开蜂鸣器
void BEEP_On(void)
{
    HAL_GPIO_WritePin(GPIOF, GPIO_PIN_8, GPIO_PIN_SET);  // 打开蜂鸣器
}

// 关闭蜂鸣器
void BEEP_Off(void)
{
    HAL_GPIO_WritePin(GPIOF, GPIO_PIN_8, GPIO_PIN_RESET);  // 关闭蜂鸣器
}

// 蜂鸣器任务
// 每隔一定时间检查是否需要关闭蜂鸣器
void Alarm_BEEP_Task(void)
{
    RTC_TimeTypeDef now_time;  // 当前时间
    RTC_DateTypeDef now_Date;  // 当前日期
    HAL_RTC_GetTime(&RTC_Handler, &now_time, RTC_FORMAT_BIN);  // 获取当前时间
    HAL_RTC_GetDate(&RTC_Handler, &now_Date, RTC_FORMAT_BIN);  // 获取当前日期
	
    uint8_t elapsed = (now_time.Seconds + 60 - beep_start_time) % 60;  // 计算已过时间
	
    if (beep_flag && elapsed >= 5)  // 如果蜂鸣器标志位为 1 且已过时间大于等于 5 秒
    {
        LCD_ShowString(10, 70, lcddev.width, 32, 32, (uint8_t*)"                ");  // 清除屏幕
        LCD_ShowString(10, 35, lcddev.width, 32, 32, (uint8_t*)"                ");  // 清除屏幕
        BEEP_Off();  // 关闭蜂鸣器
        beep_flag = 0;  // 清除蜂鸣器标志位
    }
}












