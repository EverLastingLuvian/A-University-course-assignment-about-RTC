#ifndef MENU_H
#define MENU_H
#include "main.h"


void Key_Init(void);// 初始化按键硬件
void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin);// 按键中断回调函数，处理按键事件
void Menu_Update(void);// 更新菜单状态，处理用户输入并绘制相应菜单
void Menu_DrawMain(int selection);// 绘制主菜单界面
void Menu_DrawClockSetting(int selection);// 绘制时钟设置菜单界面
void Menu_ChangClockSetting(int selection);// 更新菜单状态，处理用户输入并绘制相应菜单
void Menu_DrawClockMain(int selection);// 绘制时钟主菜单界面
void Menu_DrawClock_Alarm_A_Main(int selection);// 绘制闹钟 A 设置菜单界面
void Menu_ChangClock_Alarm_A_Main(int selection);// 根据用户输入修改闹钟 A 设置
void Menu_DrawClock_Alarm_B_Main(int selection);// 绘制闹钟 B 设置菜单界面
void Menu_ChangClock_Alarm_B_Main(int selection);// 根据用户输入修改闹钟 B 设置
void Menu_DrawADC(void);// 绘制 ADC 菜单界面
void Menu_ChangClock_WIFI(int selection);// 根据用户选择切换 WIFI 模式
void Menu_DrawClock_WIFI(int selection);// 绘制 WIFI 菜单界面
// 按键状态枚举
typedef enum {
    KEY_UP,       // 上键
    KEY_DOWM,     // 下键
    KEY_LEFT,     // 左键
    KEY_RIGHT,    // 右键
    KEY_DISABLE   // 无按键
} KeyState;

#endif









