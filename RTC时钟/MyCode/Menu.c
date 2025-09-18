#include "Menu.h"
#include "lcd.h"
#include "RTC.h"
#include <stdio.h>
#include <string.h>
#include "ADC.h"
#include "common.h"
#include "dac_wave.h"
#include "tetrisGame.h"

/* ===================== LCD / 菜单相关宏定义 ===================== */
#define DEBOUNCE_DELAY 60      // 按键消抖时间 (ms)
#define LCD_W lcddev.width      // LCD 宽度
#define LCD_H lcddev.height     // LCD 高度
#define ITEM_HEIGHT 40          // 菜单项高度
#define ITEM_WIDTH  140         // 菜单项宽度
#define ITEM_GAP    15          // 菜单项之间间隔
#define FONT_SIZE   24          // 字体大小

#define ClockMain_ITEM_HEIGHT  ITEM_HEIGHT
#define ClockMain_ITEM_WIDTH   ITEM_WIDTH + 40  // 时钟主菜单比普通菜单更宽


/* ===================== 按键消抖相关变量 ===================== */
static uint32_t lastTick_PA0 = 0;
static uint32_t lastTick_PE2 = 0;
static uint32_t lastTick_PE3 = 0;
static uint32_t lastTick_PE4 = 0;

/* ===================== 菜单选择状态变量 ===================== */
static int MENU_MAIN_selection = 0;          // 主菜单选项下标
static int MENU_CLOCK_SETTING_selection = 0; // 时钟设置菜单下标
static int MENU_CLOCK_MAIN_selection = 0;    // 时钟主菜单下标
static int MENU_CLOCK_ALARM_A_selection = 0; // 闹钟 A 设置菜单下标
static int MENU_CLOCK_ALARM_B_selection = 0; // 闹钟 B 设置菜单下标
static int MENU_ADC_selection = 0;
static int MENU_WIFI_selection = 0;
/* ===================== 菜单状态枚举 ===================== */
typedef enum {
    MENU_MAIN,           // 主菜单
    MENU_CLOCK,          // 时钟显示
    MENU_ADC,            // ADC 功能
    MENU_DAC,            // DAC 功能
    MENU_CLOCK_MAIN,     // 时钟主菜单 (选择进入设置/闹钟)
    MENU_CLOCK_SETTING,  // 时间日期设置
    MENU_CLOCK_ALARM_A,  // 闹钟 A 设置
    MENU_CLOCK_ALARM_B,  // 闹钟 B 设置
		MENU_WIFI
} MenuState;

/* ===================== 按键状态枚举 ===================== */
//typedef enum {
//    KEY_UP,       // 上键
//    KEY_DOWM,     // 下键
//    KEY_LEFT,     // 左键
//    KEY_RIGHT,    // 右键
//    KEY_DISABLE   // 无按键
//} KeyState;

/* ===================== 当前状态全局变量 ===================== */
MenuState menu_state = MENU_MAIN;   // 当前菜单状态
KeyState key_state = KEY_DISABLE;   // 当前按键状态

extern float voltage[SAMPLE_SIZE];
extern __IO uint16_t adc_value[SAMPLE_SIZE];
extern uint8_t first_draw;
extern uint8_t BRE;

/* ===================== 按键 GPIO 初始化 ===================== */
void Key_Init(void)
{
  GPIO_InitTypeDef GPIO_InitStruct = {0};

  /* 开启 GPIO 时钟 */
  __HAL_RCC_GPIOE_CLK_ENABLE();
  __HAL_RCC_GPIOC_CLK_ENABLE();
  __HAL_RCC_GPIOH_CLK_ENABLE();
  __HAL_RCC_GPIOA_CLK_ENABLE();

  /* 配置 GPIOE 2/3/4 为下降沿中断 (上拉) */
  GPIO_InitStruct.Pin = GPIO_PIN_2|GPIO_PIN_3|GPIO_PIN_4;
  GPIO_InitStruct.Mode = GPIO_MODE_IT_FALLING;
  GPIO_InitStruct.Pull = GPIO_PULLUP;
  HAL_GPIO_Init(GPIOE, &GPIO_InitStruct);

  /* 配置 GPIOA 0 为上升沿中断 (下拉) */
  GPIO_InitStruct.Pin = GPIO_PIN_0;
  GPIO_InitStruct.Mode = GPIO_MODE_IT_RISING;
  GPIO_InitStruct.Pull = GPIO_PULLDOWN;
  HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

  /* EXTI 中断优先级配置 */
  HAL_NVIC_SetPriority(EXTI0_IRQn, 0, 0);
  HAL_NVIC_EnableIRQ(EXTI0_IRQn);

  HAL_NVIC_SetPriority(EXTI2_IRQn, 0, 0);
  HAL_NVIC_EnableIRQ(EXTI2_IRQn);

  HAL_NVIC_SetPriority(EXTI3_IRQn, 0, 0);
  HAL_NVIC_EnableIRQ(EXTI3_IRQn);

  HAL_NVIC_SetPriority(EXTI4_IRQn, 0, 0);
  HAL_NVIC_EnableIRQ(EXTI4_IRQn);
}

/* ===================== 按键中断回调函数 ===================== */
void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin)
{
    uint32_t now = HAL_GetTick();  // 获取系统当前时间 (ms)

    switch(GPIO_Pin)
    {
        case GPIO_PIN_0: // PA0 -> KEY_UP
            if (now - lastTick_PA0 > DEBOUNCE_DELAY)
            {
                lastTick_PA0 = now;
                key_state = KEY_UP;
            }
            break;

        case GPIO_PIN_2: // PE2 -> KEY_LEFT
            if (now - lastTick_PE2 > DEBOUNCE_DELAY)
            {
                lastTick_PE2 = now;
                key_state = KEY_LEFT;
            }
            break;

        case GPIO_PIN_3: // PE3 -> KEY_DOWN
            if (now - lastTick_PE3 > DEBOUNCE_DELAY)
            {
              lastTick_PE3 = now;
              key_state = KEY_DOWM;
            }
            break;

        case GPIO_PIN_4: // PE4 -> KEY_RIGHT
            if (now - lastTick_PE4 > DEBOUNCE_DELAY)
            {
              lastTick_PE4 = now;
              key_state = KEY_RIGHT;
            }
            break;

        default:
            break;
    }
}


/* ===================== 菜单更新函数 ===================== */
/**
 * @brief 更新菜单状态，处理用户输入并绘制相应的菜单界面
 * 
 * 该函数是菜单系统的主循环，根据当前的菜单状态（menu_state）和用户输入（key_state），
 * 执行相应的操作，如切换菜单、进入子菜单、修改设置等。
 * 
 * 主要逻辑包括：
 * - 主菜单（MENU_MAIN）：用户可以通过左右键切换选项，按下键进入子菜单。
 * - 时钟显示（MENU_CLOCK）：显示当前时间，用户可以通过按键进入时钟设置或返回主菜单。
 * - 时钟设置（MENU_CLOCK_SETTING）：用户可以设置时间，通过左右键选择要设置的项，按下键修改值。
 * - 闹钟设置（MENU_CLOCK_ALARM_A 和 MENU_CLOCK_ALARM_B）：用户可以设置闹钟时间。
 * - ADC 菜单（MENU_ADC）：预留用于显示 ADC 采样结果。
 * - DAC 菜单（MENU_DAC）：显示 DAC 波形，并允许用户通过按键调整波形参数。
 * - WIFI 菜单（MENU_WIFI）：允许用户选择 WIFI 模式（AP 或 STA）并执行相应操作。
 * 
 * @note 该函数假设 key_state 和 menu_state 已经被正确初始化。
 */
void Menu_Update(void)
{
    switch(menu_state)
    {
        /* -------- 主菜单 -------- */
        /**
         * @brief 主菜单状态处理
         * 用户可以通过左右键切换选项，按下键进入子菜单。
         */
        case MENU_MAIN:
            if(key_state == KEY_LEFT) // 左键切换选项
            {
                MENU_MAIN_selection++;
                LCD_Clear(WHITE);
                if(MENU_MAIN_selection > 4) MENU_MAIN_selection = 0; // 循环
                key_state = KEY_DISABLE;
            }
            else if(key_state == KEY_RIGHT) // 右键切换选项
            {
                MENU_MAIN_selection--;
                LCD_Clear(WHITE);
                if(MENU_MAIN_selection < 0) MENU_MAIN_selection = 4;
                key_state = KEY_DISABLE;
            }
						
            Menu_DrawMain(MENU_MAIN_selection); // 绘制菜单

            if(MENU_MAIN_selection == 0 && key_state == KEY_DOWM) // 进入时钟
            {
                menu_state = MENU_CLOCK;
                key_state = KEY_DISABLE;
                LCD_Clear(WHITE);
            }
						else if(MENU_MAIN_selection == 1 && key_state == KEY_DOWM) // 进入 ADC 菜单
            {
                menu_state = MENU_ADC;
                key_state = KEY_DISABLE;
                LCD_Clear(WHITE);
            }
						else if(MENU_MAIN_selection == 2 && key_state == KEY_DOWM) // 进入 DAC 菜单
            {
                menu_state = MENU_DAC;
                key_state = KEY_DISABLE;
                LCD_Clear(WHITE);
            }
						else if(MENU_MAIN_selection == 3 && key_state == KEY_DOWM)
						{
								menu_state = MENU_WIFI;
                key_state = KEY_DISABLE;
                LCD_Clear(WHITE);
						}
						else if(MENU_MAIN_selection == 4 && key_state == KEY_DOWM)
						{
								tetrisGame();
                key_state = KEY_DISABLE;
                LCD_Clear(WHITE);
						}
            break;

        /* -------- 时钟显示 -------- */
        /**
         * @brief 时钟显示状态处理
         * 显示当前时间，用户可以通过按键进入时钟设置或返回主菜单。
         */
        case MENU_CLOCK:
            LCD_RCT_Show(); // 显示 RTC 时间
				
            if(key_state == KEY_UP) // 返回主菜单
            {
                menu_state = MENU_MAIN;
                key_state = KEY_DISABLE;
                LCD_Clear(WHITE);
            }
            else if(key_state == KEY_DOWM) // 进入时钟主菜单
            {
                menu_state = MENU_CLOCK_MAIN;
                key_state = KEY_DISABLE;
                LCD_Clear(WHITE);
            }
						
            break;

        /* -------- 时钟主菜单 (选择进入设置/闹钟) -------- */
        /**
         * @brief 时钟主菜单状态处理
         * 用户可以通过左右键切换选项，按下键进入子菜单。
         */
        case MENU_CLOCK_MAIN:
            if(key_state == KEY_LEFT)
            {
                MENU_CLOCK_MAIN_selection++;
                LCD_Clear(WHITE);
                if(MENU_CLOCK_MAIN_selection > 2) MENU_CLOCK_MAIN_selection = 0;
                key_state = KEY_DISABLE;
            }
            else if(key_state == KEY_RIGHT)
            {
                MENU_CLOCK_MAIN_selection--;
                LCD_Clear(WHITE);
                if(MENU_CLOCK_MAIN_selection < 0) MENU_CLOCK_MAIN_selection = 2;
                key_state = KEY_DISABLE;
            }
						
            Menu_DrawClockMain(MENU_CLOCK_MAIN_selection); // 绘制子菜单
						
            if(MENU_CLOCK_MAIN_selection == 0 && key_state == KEY_DOWM) // 进入时间设置
            {
                menu_state = MENU_CLOCK_SETTING;
                key_state = KEY_DISABLE;
                LCD_Clear(WHITE);
            }
            else if(MENU_CLOCK_MAIN_selection == 1 && key_state == KEY_DOWM) // 进入闹钟 A
            {
                menu_state = MENU_CLOCK_ALARM_A;
                key_state = KEY_DISABLE;
                LCD_Clear(WHITE);
            }
						else if(MENU_CLOCK_MAIN_selection == 2 && key_state == KEY_DOWM) // 进入闹钟 B
            {
                menu_state = MENU_CLOCK_ALARM_B;
                key_state = KEY_DISABLE;
                LCD_Clear(WHITE);
            }
            else if(key_state == KEY_UP) // 返回上一级
            {
                LCD_Clear(WHITE);
                key_state = KEY_DISABLE;
                menu_state = MENU_CLOCK;
            }
            break;

        /* -------- 时钟设置 -------- */
        /**
         * @brief 时钟设置状态处理
         * 用户可以通过左右键切换选项，按下键修改时间值。
         */
        case MENU_CLOCK_SETTING:
            LCD_RCT_Show(); // 显示当前时间
				
            if(key_state == KEY_LEFT)
            {
                MENU_CLOCK_SETTING_selection++;
                LCD_Clear(WHITE);
                if(MENU_CLOCK_SETTING_selection > 6) MENU_CLOCK_SETTING_selection = 0;
                key_state = KEY_DISABLE;
            }
            else if(key_state == KEY_RIGHT)
            {
                MENU_CLOCK_SETTING_selection--;
                LCD_Clear(WHITE);
                if(MENU_CLOCK_SETTING_selection < 0) MENU_CLOCK_SETTING_selection = 6;
                key_state = KEY_DISABLE;
            }
							
            Menu_DrawClockSetting(MENU_CLOCK_SETTING_selection); // 绘制当前选择项
            Menu_ChangClockSetting(MENU_CLOCK_SETTING_selection); // 修改 RTC 值
							
            if(key_state == KEY_UP && MENU_CLOCK_SETTING_selection == 6) // Exit 返回
            {
                menu_state = MENU_CLOCK;
                key_state = KEY_DISABLE;
                LCD_Clear(WHITE);
            }
            break;
				
        /* -------- 闹钟 A 设置 -------- */
        /**
         * @brief 闹钟 A 设置状态处理
         * 用户可以通过左右键切换选项，按下键修改闹钟时间。
         */
        case MENU_CLOCK_ALARM_A:
            if(key_state == KEY_LEFT)
            {
                MENU_CLOCK_ALARM_A_selection++;
                LCD_Clear(WHITE);
                if(MENU_CLOCK_ALARM_A_selection > 3) MENU_CLOCK_ALARM_A_selection = 0;
                key_state = KEY_DISABLE;
            }
            else if(key_state == KEY_RIGHT)
            {
                MENU_CLOCK_ALARM_A_selection--;
                LCD_Clear(WHITE);
                if(MENU_CLOCK_ALARM_A_selection < 0) MENU_CLOCK_ALARM_A_selection = 3;
                key_state = KEY_DISABLE;
            }
							
            Menu_DrawClock_Alarm_A_Main(MENU_CLOCK_ALARM_A_selection); // 绘制界面
            Menu_ChangClock_Alarm_A_Main(MENU_CLOCK_ALARM_A_selection); // 修改闹钟时间
							
            if(key_state == KEY_UP && MENU_CLOCK_ALARM_A_selection == 3) // Exit 返回
            {
                menu_state = MENU_CLOCK;
                key_state = KEY_DISABLE;
                LCD_Clear(WHITE);
            }
            break;
				
					 /* -------- 闹钟 B 设置 -------- */
					/**
					 * @brief 闹钟 B 设置状态处理
					 * 用户可以通过左右键切换选项，按下键修改闹钟时间。
					 */
					case MENU_CLOCK_ALARM_B:
							if(key_state == KEY_LEFT)
							{
									MENU_CLOCK_ALARM_B_selection++;
									LCD_Clear(WHITE);
									if(MENU_CLOCK_ALARM_B_selection > 3) MENU_CLOCK_ALARM_B_selection = 0;
									key_state = KEY_DISABLE;
							}
							else if(key_state == KEY_RIGHT)
							{
									MENU_CLOCK_ALARM_B_selection--;
									LCD_Clear(WHITE);
									if(MENU_CLOCK_ALARM_B_selection < 0) MENU_CLOCK_ALARM_B_selection = 3;
									key_state = KEY_DISABLE;
							}
								
							Menu_DrawClock_Alarm_B_Main(MENU_CLOCK_ALARM_B_selection);   // 绘制界面
							Menu_ChangClock_Alarm_B_Main(MENU_CLOCK_ALARM_B_selection); // 修改闹钟时间
								
							if(key_state == KEY_UP && MENU_CLOCK_ALARM_B_selection == 3) // Exit 返回
							{
									menu_state = MENU_CLOCK;
									key_state = KEY_DISABLE;
									LCD_Clear(WHITE);
							}
							break;
					/* -------- ADC 菜单 -------- */
					/**
					 * @brief ADC 菜单状态处理
					 * 预留用于显示 ADC 采样结果。
					 */
					case MENU_ADC: 
								Menu_DrawADC();
						
								if(key_state == KEY_UP) // Exit 返回
								{
											menu_state = MENU_MAIN;
											key_state = KEY_DISABLE;
											LCD_Clear(WHITE);
								}
							break;

					/* -------- DAC 菜单 -------- */
					/**
					 * @brief DAC 菜单状态处理
					 * 显示 DAC 波形，并允许用户通过按键调整波形参数。
					 */
					case MENU_DAC:
									if(first_draw)  // 第一次进入
									{
											first_draw = 0;
											LCD_Display_Dir(1);
											LCD_Clear(WHITE);

											DAC_Wave_Stop();
											DAC_Wave_Init();
											DAC_Wave_Start();
											DAC_Wave_UpdateDisplay();
									}

									// 按键处理
									if(key_state != KEY_DISABLE) 
									{
											DAC_Wave_ProcessKeyInput(key_state);
											DAC_Wave_UpdateDisplay();
											key_state = KEY_DISABLE;
									}

									// 定时刷新 DAC 显示
									{
											static uint32_t last_update = 0;
											uint32_t current_time = HAL_GetTick();
											uint32_t update_interval = 150; // 150ms刷新一次

											if(current_time - last_update > update_interval) {
													DAC_Wave_UpdateDisplay();
													last_update = current_time;
											}
									}

									// 退出条件：按返回键 / 或全局 BRE 信号
									if(BRE == 1) 
									{
											DAC_Wave_Stop();
											LCD_Display_Dir(0); // 方向恢复
											LCD_Clear(WHITE);

											menu_state = MENU_MAIN;   // 回到主菜单
											key_state = KEY_DISABLE;
											BRE = 0;
											first_draw = 1;           // 方便下次重新初始化
									}
							break;
						
						/* -------- WIFI 菜单 -------- */
						/**
						 * @brief WIFI 菜单状态处理
						 * 允许用户选择 WIFI 模式（AP 或 STA）并执行相应操作。
						 */
						case MENU_WIFI:
								
								if(key_state == KEY_LEFT)
							{
									MENU_WIFI_selection++;
									LCD_Clear(WHITE);
									if(MENU_WIFI_selection > 1) MENU_WIFI_selection = 0;
									key_state = KEY_DISABLE;
							}
							else if(key_state == KEY_RIGHT)
							{
									MENU_WIFI_selection--;
									LCD_Clear(WHITE);
									if(MENU_WIFI_selection < 0) MENU_WIFI_selection = 1;
									key_state = KEY_DISABLE;
							}
						
								Menu_DrawClock_WIFI(MENU_WIFI_selection);
								
								if(key_state == KEY_DOWM)
								{
									Menu_ChangClock_WIFI(MENU_WIFI_selection);
								}
						
								if(key_state == KEY_UP) // 返回主菜单
							{
									menu_state = MENU_MAIN;
									key_state = KEY_DISABLE;
									LCD_Clear(WHITE);
							}
								
								break;

					/* -------- 默认行为 -------- */
					default:
							menu_state = MENU_MAIN; // 默认回到主菜单
							break;
			}
}
/* ================= 主菜单 ================= */
void Menu_DrawMain(int selection)
{
    // 计算总高度 = 3 个菜单项的高度 + 2 个间隔高度
    uint16_t totalHeight = 3 * ITEM_HEIGHT + 2 * ITEM_GAP;

    // 起始 Y 坐标，使菜单垂直居中
    uint16_t startY = (LCD_H - totalHeight) / 2;

    // 起始 X 坐标，使菜单水平居中
    uint16_t startX = (LCD_W - ITEM_WIDTH) / 2;

    uint16_t y2 = 0, y3 = 0, y4 = 0, y5 = 0; // 存储第二、第三菜单项的 Y 坐标

    switch (selection)
    {
        case 0: // 绘制第一个菜单项：Clock
            LCD_DrawRectangle(startX, startY, startX + ITEM_WIDTH, startY + ITEM_HEIGHT); // 绘制边框
            LCD_ShowString(startX + 20, startY + 10, ITEM_WIDTH, ITEM_HEIGHT, FONT_SIZE, (uint8_t *)"Clock"); // 显示文字
            break;

        case 1: // 绘制第二个菜单项：ADC
            y2 = startY + ITEM_HEIGHT + ITEM_GAP; // 第二项 Y 坐标 = 第一项下方 + 间隔
            LCD_DrawRectangle(startX, y2, startX + ITEM_WIDTH, y2 + ITEM_HEIGHT);
            LCD_ShowString(startX + 20, y2 + 10, ITEM_WIDTH, ITEM_HEIGHT, FONT_SIZE, (uint8_t *)"ADC");
            break;

        case 2: // 绘制第三个菜单项：DAC
            y3 = startY + 2 * ITEM_HEIGHT + 2 * ITEM_GAP; // 第三项 Y 坐标
            LCD_DrawRectangle(startX, y3, startX + ITEM_WIDTH, y3 + ITEM_HEIGHT);
            LCD_ShowString(startX + 20, y3 + 10, ITEM_WIDTH, ITEM_HEIGHT, FONT_SIZE, (uint8_t *)"DAC");
            break;
				
				case 3: // 绘制第四个菜单项：WIFI
						y4 = startY + 3 * ITEM_HEIGHT + 3 * ITEM_GAP; // 第四项 Y 坐标
						LCD_DrawRectangle(startX, y4, startX + ITEM_WIDTH, y4 + ITEM_HEIGHT);
						LCD_ShowString(startX + 20, y4 + 10, ITEM_WIDTH, ITEM_HEIGHT, FONT_SIZE, (uint8_t *)"WIFI");
					break;
				
				case 4: // 绘制第五个菜单项：GAME
						y5 = startY + 4 * ITEM_HEIGHT + 4 * ITEM_GAP; // 第五项 Y 坐标
						LCD_DrawRectangle(startX, y5, startX + ITEM_WIDTH, y5 + ITEM_HEIGHT);
						LCD_ShowString(startX + 20, y5 + 10, ITEM_WIDTH, ITEM_HEIGHT, FONT_SIZE, (uint8_t *)"GAME");
					break;
    }
}

/* ================= 时钟主菜单 ================= */
void Menu_DrawClockMain(int selection)
{
    // 总高度 = 3 个菜单项 + 2 个间隔
    uint16_t totalHeight = 3 * ClockMain_ITEM_HEIGHT + 2 * ITEM_GAP;
    // 起始 Y 坐标，垂直居中
    uint16_t startY = (LCD_H - totalHeight) / 2;
    // 起始 X 坐标，水平居中
    uint16_t startX = (LCD_W - ClockMain_ITEM_WIDTH) / 2;

    uint16_t y2 = 0, y3 = 0;

    switch (selection)
    {
        case 0: // 绘制第一个菜单项：Clock_Setting
            LCD_DrawRectangle(startX, startY, startX + ClockMain_ITEM_WIDTH, startY + ClockMain_ITEM_HEIGHT);
            LCD_ShowString(startX + 20, startY + 10, ClockMain_ITEM_WIDTH, ClockMain_ITEM_HEIGHT, FONT_SIZE, (uint8_t *)"Clock_Setting");
            break;

        case 1: // 绘制第二个菜单项：Clock_Alarm_A
            y2 = startY + ClockMain_ITEM_HEIGHT + ITEM_GAP;
            LCD_DrawRectangle(startX, y2, startX + ClockMain_ITEM_WIDTH, y2 + ClockMain_ITEM_HEIGHT);
            LCD_ShowString(startX + 20, y2 + 10, ClockMain_ITEM_WIDTH, ClockMain_ITEM_HEIGHT, FONT_SIZE, (uint8_t *)"Clock_Alarm_A");
            break;

        case 2: // 绘制第三个菜单项：Clock_Alarm_B
            y3 = startY + 2 * ClockMain_ITEM_HEIGHT + 2 * ITEM_GAP;
            LCD_DrawRectangle(startX, y3, startX + ClockMain_ITEM_WIDTH, y3 + ClockMain_ITEM_HEIGHT);
            LCD_ShowString(startX + 20, y3 + 10, ClockMain_ITEM_WIDTH, ClockMain_ITEM_HEIGHT, FONT_SIZE, (uint8_t *)"Clock_Alarm_B");
            break;
    }
}

/* ================= 时钟设置菜单 ================= */
void Menu_DrawClockSetting(int selection)
{
    // 绘制右上角“Setting”按钮
    uint16_t startX = LCD_W - ITEM_WIDTH - 10; // X 坐标靠右
    uint16_t startY = ITEM_HEIGHT + 10;        // Y 坐标靠上
    LCD_DrawRectangle(startX, startY, startX + ITEM_WIDTH, startY + ITEM_HEIGHT);
    LCD_ShowString(startX + 20, startY + 10, ITEM_WIDTH, ITEM_HEIGHT, FONT_SIZE, (uint8_t *)"Setting");

    // 居中区域坐标
    uint16_t cx = (LCD_W - ITEM_WIDTH) / 2;
    uint16_t cy = (LCD_H - ITEM_HEIGHT) / 2;

    switch (selection)
    {
        case 0: // Hour 小时
            LCD_DrawRectangle(cx, cy, cx + ITEM_WIDTH, cy + ITEM_HEIGHT);
            LCD_ShowString(cx + 20, cy + 10, ITEM_WIDTH, ITEM_HEIGHT, FONT_SIZE, (uint8_t *)"Hour");
            break;

        case 1: // Minute 分钟
            LCD_DrawRectangle(cx, cy, cx + ITEM_WIDTH, cy + ITEM_HEIGHT);
            LCD_ShowString(cx + 20, cy + 10, ITEM_WIDTH, ITEM_HEIGHT, FONT_SIZE, (uint8_t *)"Minute");
            break;

        case 2: // Second 秒
            LCD_DrawRectangle(cx, cy, cx + ITEM_WIDTH, cy + ITEM_HEIGHT);
            LCD_ShowString(cx + 20, cy + 10, ITEM_WIDTH, ITEM_HEIGHT, FONT_SIZE, (uint8_t *)"Second");
            break;

        case 3: // Year 年
            LCD_DrawRectangle(cx, cy, cx + ITEM_WIDTH, cy + ITEM_HEIGHT);
            LCD_ShowString(cx + 20, cy + 10, ITEM_WIDTH, ITEM_HEIGHT, FONT_SIZE, (uint8_t *)"Year");
            break;

        case 4: // Month 月
            LCD_DrawRectangle(cx, cy, cx + ITEM_WIDTH, cy + ITEM_HEIGHT);
            LCD_ShowString(cx + 20, cy + 10, ITEM_WIDTH, ITEM_HEIGHT, FONT_SIZE, (uint8_t *)"Month");
            break;

        case 5: // Day 日
            LCD_DrawRectangle(cx, cy, cx + ITEM_WIDTH, cy + ITEM_HEIGHT);
            LCD_ShowString(cx + 20, cy + 10, ITEM_WIDTH, ITEM_HEIGHT, FONT_SIZE, (uint8_t *)"Day");
            break;

        case 6: // Exit 退出
            LCD_DrawRectangle(cx, cy, cx + ITEM_WIDTH, cy + ITEM_HEIGHT);
            LCD_ShowString(cx + 20, cy + 10, ITEM_WIDTH, ITEM_HEIGHT, FONT_SIZE, (uint8_t *)"Exit");
            break;
    }
}


// ===================== 辅助函数：判断闰年 =====================
static int Is_Leap_Year(int year)
{
    year += 2000; // RTC 返回年份是 0~99，补成完整年份 20xx
    // 闰年判断规则：
    // 能被400整除，或者能被4整除但不能被100整除
    if((year % 400 == 0) || ((year % 4 == 0) && (year % 100 != 0)))
        return 1;  // 是闰年，返回 1
    else
        return 0;  // 非闰年，返回 0
}

// ===================== 辅助函数：获取某年某月最大天数 =====================
static int Get_Month_Max_Day(int year, int month)
{
    switch(month)
    {
        case 1: case 3: case 5: case 7: case 8: case 10: case 12:
            return 31; // 大月
        case 4: case 6: case 9: case 11:
            return 30; // 小月
        case 2:
            return Is_Leap_Year(year) ? 29 : 28; // 二月，闰年 29 天，平年 28 天
        default:
            return 31; // 理论不会出现非法月份，防护返回 31
    }
}

// ===================== 修改 RTC 时间/日期的函数 =====================
void Menu_ChangClockSetting(int selection)
{
    extern RTC_HandleTypeDef RTC_Handler; 
    RTC_TimeTypeDef now_time;  // 当前时间结构体
    RTC_DateTypeDef now_date;  // 当前日期结构体

    switch(selection)
    {
        case 0: // 修改小时
        {
            HAL_RTC_GetTime(&RTC_Handler, &now_time, RTC_FORMAT_BIN); // 读取当前时间
            int Now_Hour = now_time.Hours;

            // 根据按键修改小时
            if(key_state == KEY_UP)
            {
                Now_Hour++;
                key_state = KEY_DISABLE; // 清空按键状态
            }                
            if(key_state == KEY_DOWM)
            {
                Now_Hour--;
                key_state = KEY_DISABLE;
            }                

            // 边界处理
            if(Now_Hour > 23) Now_Hour = 0;
            if(Now_Hour < 0) Now_Hour = 23;

            // 写回 RTC
            RTC_Set_Time(Now_Hour, now_time.Minutes, now_time.Seconds);
        }
        break;

        case 1: // 修改分钟
        {
            HAL_RTC_GetTime(&RTC_Handler, &now_time, RTC_FORMAT_BIN);
            int Now_Minute = now_time.Minutes;

            if(key_state == KEY_UP)
            {
                Now_Minute++;
                key_state = KEY_DISABLE;
            }                
            if(key_state == KEY_DOWM)
            {
                Now_Minute--;
                key_state = KEY_DISABLE;
            }                

            if(Now_Minute > 59) Now_Minute = 0;
            if(Now_Minute < 0) Now_Minute = 59;

            RTC_Set_Time(now_time.Hours, Now_Minute, now_time.Seconds);
        }
        break;

        case 2: // 修改秒
        {
            HAL_RTC_GetTime(&RTC_Handler, &now_time, RTC_FORMAT_BIN);
            int Now_Second = now_time.Seconds;

            if(key_state == KEY_UP)
            {
                Now_Second++;
                key_state = KEY_DISABLE;
            }                
            if(key_state == KEY_DOWM)
            {
                Now_Second--;
                key_state = KEY_DISABLE;
            }                

            if(Now_Second > 59) Now_Second = 0;
            if(Now_Second < 0) Now_Second = 59;

            RTC_Set_Time(now_time.Hours, now_time.Minutes, Now_Second);
        }
        break;

        case 3: // 修改年份
        {
            HAL_RTC_GetDate(&RTC_Handler, &now_date, RTC_FORMAT_BIN); // 读取当前日期
            int Now_Year = now_date.Year;

            if(key_state == KEY_UP)
            {
                Now_Year++;
                key_state = KEY_DISABLE;
            }                
            if(key_state == KEY_DOWM)
            {
                Now_Year--;
                key_state = KEY_DISABLE;
            }                

            if(Now_Year > 99) Now_Year = 0; // RTC 仅支持 0~99
            if(Now_Year < 0) Now_Year = 99;

            RTC_Set_Date(Now_Year, now_date.Month, now_date.Date);
        }
        break;

        case 4: // 修改月份
        {
            HAL_RTC_GetDate(&RTC_Handler, &now_date, RTC_FORMAT_BIN);
            int Now_Month = now_date.Month;

            if(key_state == KEY_UP)
            {
                Now_Month++;
                key_state = KEY_DISABLE;
            }                
            if(key_state == KEY_DOWM)
            {
                Now_Month--;
                key_state = KEY_DISABLE;
            }                

            if(Now_Month > 12) Now_Month = 1;
            if(Now_Month < 1) Now_Month = 12;

            RTC_Set_Date(now_date.Year, Now_Month, now_date.Date);
        }
        break;

        case 5: // 修改日期
        {
            HAL_RTC_GetDate(&RTC_Handler, &now_date, RTC_FORMAT_BIN);
            int Now_Date = now_date.Date;

            if(key_state == KEY_UP)
            {
                Now_Date++;
                key_state = KEY_DISABLE;
            }                
            if(key_state == KEY_DOWM)
            {
                Now_Date--;
                key_state = KEY_DISABLE;
            }                

            // 获取该月最大天数，防止日期溢出
            int max_day = Get_Month_Max_Day(now_date.Year, now_date.Month);

            if(Now_Date > max_day) Now_Date = 1;  // 循环到第一天
            if(Now_Date < 1) Now_Date = max_day;  // 循环到当月最后一天

            RTC_Set_Date(now_date.Year, now_date.Month, Now_Date);
        }
        break;
    }
}



/**
  * @brief  绘制闹钟 A 设置界面
  * @param  selection 当前选中的项目 (0=Hour, 1=Minute, 2=Second, 3=Exit)
  */
void Menu_DrawClock_Alarm_A_Main(int selection)
{
    uint8_t size = 32;  // 显示字体大小
    
    // 左上角的 "Alarm_A" 标题框位置
    uint16_t startX = LCD_W - ITEM_WIDTH - 10;
    uint16_t startY = ITEM_HEIGHT + 10;

    // 绘制标题矩形框
    LCD_DrawRectangle(startX, startY,
                      startX + ITEM_WIDTH,
                      startY + ITEM_HEIGHT);
    LCD_ShowString(startX + 20, startY + 10,
                   ITEM_WIDTH, ITEM_HEIGHT,
                   FONT_SIZE, (uint8_t *)"Alarm_A");

    /* ===== 显示闹钟当前时间（居中区域上半部分） ===== */
    uint16_t cx = (LCD_W - ITEM_WIDTH) / 2;   // 居中矩形 X 坐标
    uint16_t cy = (LCD_H - ITEM_HEIGHT) / 2;  // 居中矩形 Y 坐标
    
    extern RTC_HandleTypeDef RTC_Handler;
    RTC_AlarmTypeDef RTC_AlarmStruct;

    // 从 RTC 读取闹钟 A 的设置值
    HAL_RTC_GetAlarm(&RTC_Handler, &RTC_AlarmStruct, RTC_ALARM_A, RTC_FORMAT_BIN);
    
    // 格式化闹钟时间字符串
    char alarmBuffer[40];
    sprintf(alarmBuffer,
            "AlarmA: %02d:%02d:%02d",
            RTC_AlarmStruct.AlarmTime.Hours,
            RTC_AlarmStruct.AlarmTime.Minutes,
            RTC_AlarmStruct.AlarmTime.Seconds);
    
    // 在屏幕上半部分水平居中显示闹钟时间
    LCD_ShowString((LCD_W - strlen(alarmBuffer) * size/2) / 2,   // X 坐标
                   (LCD_H - size)/4,                            // Y 坐标
                   LCD_W, 32, size,
                   (uint8_t*)alarmBuffer);

    /* ===== 绘制功能菜单项，根据 selection 高亮不同选项 ===== */
    switch (selection)
    {
        case 0: /* Hour */
            LCD_DrawRectangle(cx, cy,
                              cx + ITEM_WIDTH,
                              cy + ITEM_HEIGHT);
            LCD_ShowString(cx + 20, cy + 10,
                           ITEM_WIDTH, ITEM_HEIGHT,
                           FONT_SIZE, (uint8_t *)"Hour");
            break;

        case 1: /* Minute */
            LCD_DrawRectangle(cx, cy,
                              cx + ITEM_WIDTH,
                              cy + ITEM_HEIGHT);
            LCD_ShowString(cx + 20, cy + 10,
                           ITEM_WIDTH, ITEM_HEIGHT,
                           FONT_SIZE, (uint8_t *)"Minute");
            break;

        case 2: /* Second */
            LCD_DrawRectangle(cx, cy,
                              cx + ITEM_WIDTH,
                              cy + ITEM_HEIGHT);
            LCD_ShowString(cx + 20, cy + 10,
                           ITEM_WIDTH, ITEM_HEIGHT,
                           FONT_SIZE, (uint8_t *)"Second");
            break;

        case 3: /* Exit */
            LCD_DrawRectangle(cx, cy,
                              cx + ITEM_WIDTH,
                              cy + ITEM_HEIGHT);
            LCD_ShowString(cx + 20, cy + 10,
                           ITEM_WIDTH, ITEM_HEIGHT,
                           FONT_SIZE, (uint8_t *)"Exit");
            break;
    }
}

/**
  * @brief  修改闹钟 A 的时分秒
  * @param  selection 当前修改的项目 (0=Hour, 1=Minute, 2=Second)
  */
void Menu_ChangClock_Alarm_A_Main(int selection)
{
    extern RTC_HandleTypeDef RTC_Handler;
    RTC_AlarmTypeDef RTC_AlarmStruct;
    
    // 获取当前闹钟 A 的设置
    HAL_RTC_GetAlarm(&RTC_Handler, &RTC_AlarmStruct, RTC_ALARM_A, RTC_FORMAT_BIN);
    
    // 拷贝原有时分秒
    int Hours   = RTC_AlarmStruct.AlarmTime.Hours;
    int Minutes = RTC_AlarmStruct.AlarmTime.Minutes;
    int Seconds = RTC_AlarmStruct.AlarmTime.Seconds;
            
    // 根据当前选中的选项修改对应数值
    switch(selection)
    {
        case 0: /* 修改小时 */
        {
            if(key_state == KEY_UP)      // 按上键 +1
            {
                Hours++;
                key_state = KEY_DISABLE;
            }                
            if(key_state == KEY_DOWM)    // 按下键 -1
            {
                Hours--;
                key_state = KEY_DISABLE;
            }                
            if(Hours > 23) Hours = 0;    // 循环处理
            if(Hours < 0)  Hours = 23;
        }
        break;
        
        case 1: /* 修改分钟 */
        {
            if(key_state == KEY_UP)
            {
                Minutes++;
                key_state = KEY_DISABLE;
            }                
            if(key_state == KEY_DOWM)
            {
                Minutes--;
                key_state = KEY_DISABLE;
            }                
            if(Minutes > 59) Minutes = 0;
            if(Minutes < 0)  Minutes = 59;
        }
        break;
        
        case 2: /* 修改秒 */
        {
            if(key_state == KEY_UP)
            {
                Seconds++;
                key_state = KEY_DISABLE;
            }                
            if(key_state == KEY_DOWM)
            {
                Seconds--;
                key_state = KEY_DISABLE;
            }                
            if(Seconds > 59) Seconds = 0;
            if(Seconds < 0)  Seconds = 59;
        }
        break;
    }

    // 将修改后的时分秒重新写入闹钟 A
    RTC_Set_AlarmA(Hours, Minutes, Seconds);
}


/**
  * @brief  绘制闹钟 B 设置界面
  * @param  selection 当前选中的项目 (0=Hour, 1=Minute, 2=Second, 3=Exit)
  */
void Menu_DrawClock_Alarm_B_Main(int selection)
{
    uint8_t size = 32;  // 显示字体大小
    
    // 左上角的 "Alarm_B" 标题框位置
    uint16_t startX = LCD_W - ITEM_WIDTH - 10;
    uint16_t startY = ITEM_HEIGHT + 10;

    // 绘制标题矩形框
    LCD_DrawRectangle(startX, startY,
                      startX + ITEM_WIDTH,
                      startY + ITEM_HEIGHT);
    LCD_ShowString(startX + 20, startY + 10,
                   ITEM_WIDTH, ITEM_HEIGHT,
                   FONT_SIZE, (uint8_t *)"Alarm_B");

    /* ===== 显示闹钟当前时间（居中区域上半部分） ===== */
    uint16_t cx = (LCD_W - ITEM_WIDTH) / 2;   // 居中矩形 X 坐标
    uint16_t cy = (LCD_H - ITEM_HEIGHT) / 2;  // 居中矩形 Y 坐标
    
    extern RTC_HandleTypeDef RTC_Handler;
    RTC_AlarmTypeDef RTC_AlarmStruct;

    // 从 RTC 读取闹钟 B 的设置值
    HAL_RTC_GetAlarm(&RTC_Handler, &RTC_AlarmStruct, RTC_ALARM_B, RTC_FORMAT_BIN);
    
    // 格式化闹钟时间字符串
    char alarmBuffer[40];
    sprintf(alarmBuffer,
            "AlarmB: %02d:%02d:%02d",
            RTC_AlarmStruct.AlarmTime.Hours,
            RTC_AlarmStruct.AlarmTime.Minutes,
            RTC_AlarmStruct.AlarmTime.Seconds);
    
    // 在屏幕上半部分水平居中显示闹钟时间
    LCD_ShowString((LCD_W - strlen(alarmBuffer) * size/2) / 2,   // X 坐标
                   (LCD_H - size)/4,                            // Y 坐标
                   LCD_W, 32, size,
                   (uint8_t*)alarmBuffer);

    /* ===== 绘制功能菜单项，根据 selection 高亮不同选项 ===== */
    switch (selection)
    {
        case 0: /* Hour */
            LCD_DrawRectangle(cx, cy,
                              cx + ITEM_WIDTH,
                              cy + ITEM_HEIGHT);
            LCD_ShowString(cx + 20, cy + 10,
                           ITEM_WIDTH, ITEM_HEIGHT,
                           FONT_SIZE, (uint8_t *)"Hour");
            break;

        case 1: /* Minute */
            LCD_DrawRectangle(cx, cy,
                              cx + ITEM_WIDTH,
                              cy + ITEM_HEIGHT);
            LCD_ShowString(cx + 20, cy + 10,
                           ITEM_WIDTH, ITEM_HEIGHT,
                           FONT_SIZE, (uint8_t *)"Minute");
            break;

        case 2: /* Second */
            LCD_DrawRectangle(cx, cy,
                              cx + ITEM_WIDTH,
                              cy + ITEM_HEIGHT);
            LCD_ShowString(cx + 20, cy + 10,
                           ITEM_WIDTH, ITEM_HEIGHT,
                           FONT_SIZE, (uint8_t *)"Second");
            break;

        case 3: /* Exit */
            LCD_DrawRectangle(cx, cy,
                              cx + ITEM_WIDTH,
                              cy + ITEM_HEIGHT);
            LCD_ShowString(cx + 20, cy + 10,
                           ITEM_WIDTH, ITEM_HEIGHT,
                           FONT_SIZE, (uint8_t *)"Exit");
            break;
    }
}


/**
  * @brief  修改闹钟 B 的时分秒
  * @param  selection 当前修改的项目 (0=Hour, 1=Minute, 2=Second)
  */
void Menu_ChangClock_Alarm_B_Main(int selection)
{
    extern RTC_HandleTypeDef RTC_Handler;
    RTC_AlarmTypeDef RTC_AlarmStruct;
    
    // 获取当前闹钟 B 的设置
    HAL_RTC_GetAlarm(&RTC_Handler, &RTC_AlarmStruct, RTC_ALARM_B, RTC_FORMAT_BIN);
    
    // 拷贝原有时分秒
    int Hours   = RTC_AlarmStruct.AlarmTime.Hours;
    int Minutes = RTC_AlarmStruct.AlarmTime.Minutes;
    int Seconds = RTC_AlarmStruct.AlarmTime.Seconds;
            
    // 根据当前选中的选项修改对应数值
    switch(selection)
    {
        case 0: /* 修改小时 */
        {
            if(key_state == KEY_UP)      // 按上键 +1
            {
                Hours++;
                key_state = KEY_DISABLE;
            }                
            if(key_state == KEY_DOWM)    // 按下键 -1
            {
                Hours--;
                key_state = KEY_DISABLE;
            }                
            if(Hours > 23) Hours = 0;    // 循环处理
            if(Hours < 0)  Hours = 23;
        }
        break;
        
        case 1: /* 修改分钟 */
        {
            if(key_state == KEY_UP)
            {
                Minutes++;
                key_state = KEY_DISABLE;
            }                
            if(key_state == KEY_DOWM)
            {
                Minutes--;
                key_state = KEY_DISABLE;
            }                
            if(Minutes > 59) Minutes = 0;
            if(Minutes < 0)  Minutes = 59;
        }
        break;
        
        case 2: /* 修改秒 */
        {
            if(key_state == KEY_UP)
            {
                Seconds++;
                key_state = KEY_DISABLE;
            }                
            if(key_state == KEY_DOWM)
            {
                Seconds--;
                key_state = KEY_DISABLE;
            }                
            if(Seconds > 59) Seconds = 0;
            if(Seconds < 0)  Seconds = 59;
        }
        break;
    }

    // 将修改后的时分秒重新写入闹钟 B
    RTC_Set_AlarmB(Hours, Minutes, Seconds);
}


/**
 * @brief 绘制 ADC 采样结果
 * 
 * 该函数负责执行 ADC 采样，处理采样数据，并将结果显示在屏幕上。
 * 主要步骤包括：
 * 1. 采样：通过 ADC 读取电压值。
 * 2. 信号处理：识别波形类型。
 * 3. 显示：将处理结果显示在屏幕上。
 * 4. 间隔：在下一次采样前等待 300ms。
 */
void Menu_DrawADC(void)
{
    int i;
    /* ---------- 采样 ---------- */
    for (i = 0; i < SAMPLE_SIZE; i++) {
        adc_value[i] = Read_ADC();  /**< 读取 ADC 值 */
        voltage[i]   = (float)adc_value[i] * 3.3f / 4096.0f;  /**< 将 ADC 值转换为电压值 */
        delay_us(1000000 / SAMPLE_RATE); /**< 采样间隔，确保采样率 */
    }

    /* ---------- 信号处理 ---------- */
    identify_waveform(); /**< 识别波形类型 */

    /* ---------- 显示 ---------- */
    display_results(); /**< 显示处理结果 */

    /* ---------- 下次采样间隔 ---------- */
    delay_ms(300); /**< 在下一次采样前等待 300ms */
}

/**
 * @brief 绘制 WIFI 选项菜单
 * 
 * 该函数根据用户选择绘制 WIFI 选项菜单。
 * 主要步骤包括：
 * 1. 计算居中矩形的坐标。
 * 2. 根据用户选择绘制不同的菜单选项。
 * 
 * @param selection 用户选择的菜单项（0: WIFI_AP, 1: WIFI_STA）
 */
void Menu_DrawClock_WIFI(int selection)
{
    uint8_t size = 32;  /**< 显示字体大小 */

    /* ===== 显示闹钟当前时间（居中区域上半部分） ===== */
    uint16_t cx = (LCD_W - ITEM_WIDTH) / 2;   /**< 居中矩形 X 坐标 */
    uint16_t cy = (LCD_H - ITEM_HEIGHT) / 2;  /**< 居中矩形 Y 坐标 */

    switch (selection)
    {
        case 0: 
            LCD_DrawRectangle(cx, cy, cx + ITEM_WIDTH, cy + ITEM_HEIGHT); /**< 绘制矩形框 */
            LCD_ShowString(cx + 20, cy + 10, ITEM_WIDTH, ITEM_HEIGHT, FONT_SIZE, (uint8_t *)"WIFI_AP"); /**< 显示文本 "WIFI_AP" */
            break;

        case 1: 
            LCD_DrawRectangle(cx, cy, cx + ITEM_WIDTH, cy + ITEM_HEIGHT); /**< 绘制矩形框 */
            LCD_ShowString(cx + 20, cy + 10, ITEM_WIDTH, ITEM_HEIGHT, FONT_SIZE, (uint8_t *)"WIFI_STA"); /**< 显示文本 "WIFI_STA" */
            break;
    }
}

/**
 * @brief 根据用户选择执行 WIFI 模式切换
 * 
 * 该函数调用 ATK-RM04 测试函数，根据用户的选择执行不同的操作。
 * 
 * @param selection 用户选择的菜单项（0: WIFI_AP, 1: WIFI_STA）
 */
void Menu_ChangClock_WIFI(int selection) 
{
    atk_rm04_test(selection); /**< 根据用户选择执行 WIFI 模式切换 */
}



















