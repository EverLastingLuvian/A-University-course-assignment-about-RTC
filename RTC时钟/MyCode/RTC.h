#ifndef RTC_H
#define RTC_H
#include "main.h"

// RTC初始化
void RTC_Init(void);

// 设置RTC时间
HAL_StatusTypeDef RTC_Set_Time(uint8_t hour,uint8_t min,uint8_t sec);

// 设置RTC日期
HAL_StatusTypeDef RTC_Set_Date(uint8_t year,uint8_t month,uint8_t date);

// LCD显示RTC时间和时钟
void LCD_RCT_Show(void);

// 设置闹钟A
void RTC_Set_AlarmA(uint8_t hour,uint8_t min,uint8_t sec);

// 闹钟A中断回调
void HAL_RTC_AlarmAEventCallback(RTC_HandleTypeDef *hrtc);

// 闹钟B中断回调
void HAL_RTCEx_AlarmBEventCallback(RTC_HandleTypeDef *hrtc);

// 设置闹钟B
void RTC_Set_AlarmB(uint8_t hour, uint8_t min, uint8_t sec);

#endif





