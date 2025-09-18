#include "lcd.h"
#include "RTC.h"
#include "math.h"
#include <stdio.h>
#include <string.h>
#include "beep.h"
#include "usart3.h"

#define PI 3.14159265358979323846f
#define LCD_W lcddev.width
#define LCD_H lcddev.height

uint32_t beep_start_time = 0;  // 蜂鸣器启动时间记录
uint8_t beep_flag = 0;         // 蜂鸣器状态标志位

RTC_HandleTypeDef RTC_Handler; 

/**
  * @brief  RTC初始化函数
  * @param  无
  * @retval 无
  */
void RTC_Init(void)
{
  RTC_TimeTypeDef sTime = {0};    // 时间结构体初始化
  RTC_DateTypeDef sDate = {0};    // 日期结构体初始化
  RTC_AlarmTypeDef sAlarm = {0};  // 闹钟结构体初始化

  // RTC外设基本配置
  RTC_Handler.Instance = RTC;
  RTC_Handler.Init.HourFormat = RTC_HOURFORMAT_24;      // 24小时制
  RTC_Handler.Init.AsynchPrediv = 127;                  // 异步预分频器
  RTC_Handler.Init.SynchPrediv = 255;                   // 同步预分频器
  RTC_Handler.Init.OutPut = RTC_OUTPUT_DISABLE;         // 输出禁用
  RTC_Handler.Init.OutPutPolarity = RTC_OUTPUT_POLARITY_HIGH; // 输出极性高
  RTC_Handler.Init.OutPutType = RTC_OUTPUT_TYPE_OPENDRAIN;    // 开漏输出
  
  // 初始化RTC
  if (HAL_RTC_Init(&RTC_Handler) != HAL_OK)
  {
    Error_Handler();  // 初始化失败，调用错误处理函数
  }

  // 检查备份寄存器，判断是否是第一次配置
  if(HAL_RTCEx_BKUPRead(&RTC_Handler,RTC_BKP_DR0)!=0X5050)
  {	
    // 设置初始时间：17:00:00
    sTime.Hours = 0x17;
    sTime.Minutes = 0x0;
    sTime.Seconds = 0x0;
    sTime.DayLightSaving = RTC_DAYLIGHTSAVING_NONE;    // 无夏令时
    sTime.StoreOperation = RTC_STOREOPERATION_RESET;   // 存储操作复位

    // 设置时间
    if (HAL_RTC_SetTime(&RTC_Handler, &sTime, RTC_FORMAT_BCD) != HAL_OK)
    {
      Error_Handler();
    }
    
    // 设置初始日期：2025年9月8日星期一
    sDate.WeekDay = RTC_WEEKDAY_MONDAY;    // 星期一
    sDate.Month = RTC_MONTH_SEPTEMBER;     // 九月
    sDate.Date = 0x8;                      // 8号
    sDate.Year = 0x25;                     // 25年（2025）
    
    // 设置日期
    if (HAL_RTC_SetDate(&RTC_Handler, &sDate, RTC_FORMAT_BCD) != HAL_OK)
    {
      Error_Handler();
    }
    
    // 在备份寄存器中写入标志，表示已初始化
    HAL_RTCEx_BKUPWrite(&RTC_Handler,RTC_BKP_DR0,0X5050);
  }
  
  /** 启用闹钟A
  */
  sAlarm.AlarmTime.Hours = 0x0;            // 闹钟小时
  sAlarm.AlarmTime.Minutes = 0x0;          // 闹钟分钟
  sAlarm.AlarmTime.Seconds = 0x0;          // 闹钟秒钟
  sAlarm.AlarmTime.SubSeconds = 0x0;       // 子秒
  sAlarm.AlarmTime.DayLightSaving = RTC_DAYLIGHTSAVING_NONE;    // 无夏令时
  sAlarm.AlarmTime.StoreOperation = RTC_STOREOPERATION_RESET;   // 存储操作复位
  sAlarm.AlarmMask = RTC_ALARMMASK_DATEWEEKDAY;                 // 忽略日期和星期
  sAlarm.AlarmSubSecondMask = RTC_ALARMSUBSECONDMASK_ALL;       // 所有子秒位掩码
  sAlarm.AlarmDateWeekDaySel = RTC_ALARMDATEWEEKDAYSEL_DATE;    // 按日期选择
  sAlarm.AlarmDateWeekDay = 0x1;           // 日期为1号
  sAlarm.Alarm = RTC_ALARM_A;              // 闹钟A
  if (HAL_RTC_SetAlarm_IT(&RTC_Handler, &sAlarm, RTC_FORMAT_BCD) != HAL_OK)
  {
    Error_Handler();
  }

  /** 启用闹钟B
  */
  sAlarm.Alarm = RTC_ALARM_B;              // 闹钟B
  if (HAL_RTC_SetAlarm_IT(&RTC_Handler, &sAlarm, RTC_FORMAT_BCD) != HAL_OK)
  {
    Error_Handler();
  }

  /** 启用唤醒定时器
  */
  if (HAL_RTCEx_SetWakeUpTimer_IT(&RTC_Handler, 0, RTC_WAKEUPCLOCK_CK_SPRE_16BITS) != HAL_OK)
  {
    Error_Handler();
  }
}

/**
  * @brief  设置RTC时间
  * @param  hour: 小时(0-23)
  * @param  min: 分钟(0-59)
  * @param  sec: 秒钟(0-59)
  * @retval HAL状态
  */
HAL_StatusTypeDef RTC_Set_Time(uint8_t hour,uint8_t min,uint8_t sec)
{
  RTC_TimeTypeDef RTC_TimeStructure;
  
  // 填充时间结构体
  RTC_TimeStructure.Hours = hour;
  RTC_TimeStructure.Minutes = min;
  RTC_TimeStructure.Seconds = sec;
  RTC_TimeStructure.TimeFormat = 0;                        // 24小时制
  RTC_TimeStructure.DayLightSaving = RTC_DAYLIGHTSAVING_NONE;    // 无夏令时
  RTC_TimeStructure.StoreOperation = RTC_STOREOPERATION_RESET;   // 存储操作复位
  
  return HAL_RTC_SetTime(&RTC_Handler,&RTC_TimeStructure,RTC_FORMAT_BIN);	
}

/**
  * @brief  蔡勒公式计算星期几
  * @param  year: 年份(0-99表示2000-2099)
  * @param  month: 月份(1-12)
  * @param  date: 日期(1-31)
  * @retval 星期几(RTC_WEEKDAY枚举值)
  */
static uint8_t Zeller(int year, int month, int date)
{
  int C = 20;  // 世纪常数

  // 蔡勒公式要求1月和2月当作上一年的13月和14月
  if(month == 1 || month == 2) 
  {
    month += 12;
    year--;
  }
  
  // 年份边界检查
  if(year < 0)
  {
    year = 99;
    C--;
  }
  else if(year > 99)
  {
    year = 0;
    C++;
  }

  // 蔡勒公式计算
  int h = (date + (26 * (month + 1)) / 10 + year + year/4 + C/4 + 5*C) % 7;

  // 将计算结果转换为RTC星期枚举值
  switch(h)
  {
    case 0: return RTC_WEEKDAY_SATURDAY;   // 星期六
    case 1: return RTC_WEEKDAY_SUNDAY;     // 星期日
    case 2: return RTC_WEEKDAY_MONDAY;     // 星期一
    case 3: return RTC_WEEKDAY_TUESDAY;    // 星期二
    case 4: return RTC_WEEKDAY_WEDNESDAY;  // 星期三
    case 5: return RTC_WEEKDAY_THURSDAY;   // 星期四
    case 6: return RTC_WEEKDAY_FRIDAY;     // 星期五
  }
  return 0;
}

/**
  * @brief  设置RTC日期
  * @param  year: 年份(0-99表示2000-2099)
  * @param  month: 月份(1-12)
  * @param  date: 日期(1-31)
  * @retval HAL状态
  */
HAL_StatusTypeDef RTC_Set_Date(uint8_t year,uint8_t month,uint8_t date)
{
  RTC_DateTypeDef RTC_DateStructure;
  
  // 使用蔡勒公式计算星期几
  uint8_t week = Zeller(year, month, date);
  
  // 填充日期结构体
  RTC_DateStructure.Date = date;
  RTC_DateStructure.Month = month;
  RTC_DateStructure.WeekDay = week;
  RTC_DateStructure.Year = year;
  
  return HAL_RTC_SetDate(&RTC_Handler,&RTC_DateStructure,RTC_FORMAT_BIN);
}

/**
  * @brief  在LCD上显示RTC时间和模拟时钟
  * @param  无
  * @retval 无
  */
void LCD_RCT_Show(void)
{
  RTC_TimeTypeDef now_time;  // 当前时间
  RTC_DateTypeDef now_date;  // 当前日期
  
  // 获取当前时间和日期
  HAL_RTC_GetTime(&RTC_Handler, &now_time, RTC_FORMAT_BIN);
  HAL_RTC_GetDate(&RTC_Handler, &now_date, RTC_FORMAT_BIN);
  
  //==================== 时间显示部分 ====================//
  // 设置字体大小
  uint8_t size = 32;
  POINT_COLOR = BLUE;    // 设置画笔颜色为蓝色
  BACK_COLOR  = WHITE;   // 设置背景颜色为白色

  char buf[64];          // 显示缓冲区
  // 起始坐标（上半部分居中显示）
  uint16_t y = LCD_H / 6;
  uint16_t x = (LCD_W - (8*(size/2) + 3*size))/2;   

  // ==== 年份显示 ====//
  sprintf(buf, "20%02d", now_date.Year);  // 格式化为20XX年
  LCD_ShowString(x, y, LCD_W, size, size, (uint8_t*)buf);
  x += strlen(buf) * (size / 2);
  LCD_Show_Chinese(x, y, 0, size);  // 显示"年"字
  x += size;

  // ==== 月份显示 ====//
  sprintf(buf, "%02d", now_date.Month);   // 格式化为两位数月份
  LCD_ShowString(x, y, LCD_W, size, size, (uint8_t*)buf);
  x += 2 * (size / 2);
  LCD_Show_Chinese(x, y, 1, size);  // 显示"月"字
  x += size;

  // ==== 日期显示 ====//
  sprintf(buf, "%02d", now_date.Date);    // 格式化为两位数日期
  LCD_ShowString(x, y, LCD_W, size, size, (uint8_t*)buf);
  x += 2 * (size / 2);
  LCD_Show_Chinese(x, y, 2, size);  // 显示"日"字
  x += size + 8;
  
  // 时间显示位置
  uint8_t y_time = LCD_H / 6 + 60;
  uint8_t x_time = (LCD_W - 8*(size/2))/2;
  
  // 格式化时间为HH:MM:SS
  sprintf(buf, "%02d:%02d:%02d", now_time.Hours, now_time.Minutes, now_time.Seconds);
  LCD_ShowString(x_time, y_time, LCD_W, size, size, (uint8_t*)buf);
  
  //==================== 模拟时钟显示部分 ====================//
  uint16_t Cx = LCD_W/2;           // 表盘中心X坐标
  uint16_t Cy = (LCD_H/4)*3;       // 表盘中心Y坐标
  uint8_t Dial_Radius = (LCD_H/2); // 表盘半径

  // 绘制表盘外圆
  LCD_Draw_Circle(Cx, Cy, Dial_Radius);
  
  // 绘制表盘刻度（每5分钟一个大刻度）
  for(int i = 0; i<60; i++)
  {
    float angle = i * (360.0f / 60.0f) * (PI / 180.0f);  // 计算刻度角度
    int x1 = Cx + Dial_Radius*cos(angle);                // 刻度外端点X
    int y1 = Cy - Dial_Radius*sin(angle);                // 刻度外端点Y
    int x2 = Cx + (Dial_Radius - 5)*cos(angle);          // 刻度内端点X
    int y2 = Cy - (Dial_Radius - 5)*sin(angle);          // 刻度内端点Y
    
    if(i%5==0)
      LCD_DrawLine(x1, y1, x2, y2);  // 大刻度（每小时刻度）
    else
      LCD_Fast_DrawPoint(x2, y2, BLACK);  // 小刻度点（每分钟刻度）
  }
  
  // 计算指针角度（转换为弧度）
  float theta_s = now_time.Seconds*(360.0f / 60.0f);  // 秒针角度
  float angle_s = -(theta_s - 90) * (PI/180.0f);      // 调整角度（从12点开始）
  
  float theta_m = now_time.Minutes*(360.0f / 60.0f) + now_time.Seconds*(360.0f / 60.0f)/60.0f;  // 分针角度
  float angle_m = -(theta_m - 90) * (PI/180.0f);      // 调整角度
  
  float theta_h = now_time.Hours*(360.0f / 12.0f) + now_time.Minutes*(360.0f / 12.0f)/60.0f;    // 时针角度
  float angle_h = -(theta_h - 90) * (PI/180.0f);      // 调整角度

  // 计算指针端点坐标
  int sx = Cx + (Dial_Radius*0.9)*cos(angle_s);  // 秒针端点X
  int sy = Cy - (Dial_Radius*0.9)*sin(angle_s);  // 秒针端点Y
  
  int mx = Cx + (Dial_Radius*0.8)*cos(angle_m);  // 分针端点X
  int my = Cy - (Dial_Radius*0.8)*sin(angle_m);  // 分针端点Y
  
  int hx = Cx + (Dial_Radius*0.65)*cos(angle_h); // 时针端点X
  int hy = Cy - (Dial_Radius*0.65)*sin(angle_h); // 时针端点Y
  
  uint32_t tmp_color = POINT_COLOR;  // 保存当前画笔颜色
  static int old_sx = -1, old_sy = -1;  // 上一次秒针坐标
  static int old_mx = -1, old_my = -1;  // 上一次分针坐标
  static int old_hx = -1, old_hy = -1;  // 上一次时针坐标
  
  // 如果指针位置发生变化，需要擦除旧指针
  if(old_sx != sx || old_mx != mx || old_hx != hx)
  {
    POINT_COLOR = WHITE;  // 设置为背景色（白色）
    // 擦除旧指针
    LCD_DrawLine(Cx, Cy, old_sx, old_sy);  // 擦除秒针
    LCD_DrawLine(Cx, Cy, old_mx, old_my);  // 擦除分针
    LCD_DrawLine(Cx, Cy, old_hx, old_hy);  // 擦除时针
  }
  
  // 恢复画笔颜色
  POINT_COLOR = tmp_color;
  // 绘制新指针
  LCD_DrawLine(Cx, Cy, sx, sy);  // 绘制秒针
  LCD_DrawLine(Cx, Cy, mx, my);  // 绘制分针
  LCD_DrawLine(Cx, Cy, hx, hy);  // 绘制时针
  
  // 保存当前指针坐标，用于下一次擦除
  old_sx = sx; old_sy = sy;
  old_mx = mx; old_my = my;
  old_hx = hx; old_hy = hy;
}

/**
  * @brief  设置闹钟A时间
  * @param  hour: 小时(0-23)
  * @param  min: 分钟(0-59)
  * @param  sec: 秒钟(0-59)
  * @retval 无
  */
void RTC_Set_AlarmA(uint8_t hour,uint8_t min,uint8_t sec)
{ 
  RTC_AlarmTypeDef RTC_AlarmStruct = {0};
  
  // 设置闹钟时间
  RTC_AlarmStruct.AlarmTime.Hours = hour;  
  RTC_AlarmStruct.AlarmTime.Minutes = min; 
  RTC_AlarmStruct.AlarmTime.Seconds = sec; 
  RTC_AlarmStruct.AlarmTime.SubSeconds = 0;
  RTC_AlarmStruct.AlarmTime.TimeFormat = RTC_HOURFORMAT_24;  // 24小时制
  
  // 设置闹钟掩码和参数
  RTC_AlarmStruct.AlarmMask = RTC_ALARMMASK_DATEWEEKDAY;      // 忽略日期或星期
  RTC_AlarmStruct.AlarmSubSecondMask = RTC_ALARMSUBSECONDMASK_ALL;
  RTC_AlarmStruct.AlarmDateWeekDaySel = RTC_ALARMDATEWEEKDAYSEL_DATE;
  RTC_AlarmStruct.AlarmDateWeekDay = 1; // 当天日期
  RTC_AlarmStruct.Alarm = RTC_ALARM_A;  // 闹钟A
  
  // 设置闹钟并启用中断
  HAL_RTC_SetAlarm_IT(&RTC_Handler,&RTC_AlarmStruct,RTC_FORMAT_BIN);
  
  // 设置闹钟中断优先级和使能
  HAL_NVIC_SetPriority(RTC_Alarm_IRQn,0x01,0x02);
  HAL_NVIC_EnableIRQ(RTC_Alarm_IRQn);
}

/**
  * @brief  RTC闹钟A中断回调函数
  * @param  hrtc: RTC句柄
  * @retval 无
  */
void HAL_RTC_AlarmAEventCallback(RTC_HandleTypeDef *hrtc)
{
  if(hrtc->Instance == RTC)
  {
    RTC_TimeTypeDef now_time;
    RTC_DateTypeDef now_Date;
    
    // 获取当前时间
    HAL_RTC_GetTime(hrtc, &now_time, RTC_FORMAT_BIN);
    HAL_RTC_GetDate(hrtc, &now_Date, RTC_FORMAT_BIN);
    
    // 记录蜂鸣器启动时间
    beep_start_time = now_time.Seconds;
    
    // 到闹钟时间，启动蜂鸣器
    BEEP_On();
    
    // 在LCD上显示闹钟提示
    LCD_ShowString(10, 70, LCD_W, 32, 32, (uint8_t *)"Alarm A Ringing!");
    
    // 设置蜂鸣器标志
    beep_flag = 1;
  }
}

/**
  * @brief  设置闹钟B时间
  * @param  hour: 小时(0-23)
  * @param  min: 分钟(0-59)
  * @param  sec: 秒钟(0-59)
  * @retval 无
  */
void RTC_Set_AlarmB(uint8_t hour, uint8_t min, uint8_t sec)
{ 
  RTC_AlarmTypeDef RTC_AlarmStruct = {0};
  
  // 设置闹钟时间
  RTC_AlarmStruct.AlarmTime.Hours   = hour;  
  RTC_AlarmStruct.AlarmTime.Minutes = min; 
  RTC_AlarmStruct.AlarmTime.Seconds = sec; 
  RTC_AlarmStruct.AlarmTime.SubSeconds = 0;
  RTC_AlarmStruct.AlarmTime.TimeFormat = RTC_HOURFORMAT_24;  // 24小时制
  
  // 设置闹钟掩码和参数
  RTC_AlarmStruct.AlarmMask = RTC_ALARMMASK_DATEWEEKDAY;      // 忽略日期或星期
  RTC_AlarmStruct.AlarmSubSecondMask = RTC_ALARMSUBSECONDMASK_ALL;
  RTC_AlarmStruct.AlarmDateWeekDaySel = RTC_ALARMDATEWEEKDAYSEL_DATE;
  RTC_AlarmStruct.AlarmDateWeekDay = 1; // 当天日期
  RTC_AlarmStruct.Alarm = RTC_ALARM_B;  // 闹钟B

  // 设置闹钟并启用中断
  HAL_RTC_SetAlarm_IT(&RTC_Handler, &RTC_AlarmStruct, RTC_FORMAT_BIN);
  
  // 设置闹钟中断优先级和使能
  HAL_NVIC_SetPriority(RTC_Alarm_IRQn, 0x01, 0x02);
  HAL_NVIC_EnableIRQ(RTC_Alarm_IRQn);
}

/**
  * @brief  RTC闹钟B中断回调函数
  * @param  hrtc: RTC句柄
  * @retval 无
  */
void HAL_RTCEx_AlarmBEventCallback(RTC_HandleTypeDef *hrtc)
{
  if(hrtc->Instance == RTC)
  {
    RTC_TimeTypeDef now_time;
    RTC_DateTypeDef now_Date;
    
    // 获取当前时间
    HAL_RTC_GetTime(hrtc, &now_time, RTC_FORMAT_BIN);
    HAL_RTC_GetDate(hrtc, &now_Date, RTC_FORMAT_BIN);
    
    // 记录蜂鸣器启动时间
    beep_start_time = now_time.Seconds;

    // 到闹钟时间，启动蜂鸣器
    BEEP_On();
    
    // 在LCD上显示闹钟提示
    LCD_ShowString(10, 35, LCD_W, 32, 32, (uint8_t *)"Alarm B Ringing!");
    
    // 设置蜂鸣器标志
    beep_flag = 1;
  }
}














