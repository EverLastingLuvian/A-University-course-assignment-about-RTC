/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2025 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */
/* USER CODE END Header */

/* Includes ------------------------------------------------------------------*/
#include "main.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "sys.h"
#include "delay.h"
#include "usart.h"
#include "usmart.h"

#include "sram.h"
#include "malloc.h"
#include "sdio_sdcard.h"
#include "w25qxx.h"
#include "diskio.h"
#include "ff.h"          
#include "exfuns.h"     
#include "lcd.h"
#include "Menu.h"
#include "fontupd.h"

#include "wm8978.h"
#include "audioplay.h"

#include "RTC.h"
#include "string.h"	
#include "VOICE.h"
#include "beep.h"
#include "ADC.h"
#include "common.h"
#include "text.h"
#include "dac_wave.h"
#include "eeprom.h"

/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */
/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/

/* USER CODE BEGIN PV */

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
/* USER CODE BEGIN PFP */

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */

/**
 * @brief 在LCD指定位置显示一条信息（字符串+数值）
 * @param x 起始x坐标
 * @param y 起始y坐标
 * @param msg 要显示的字符串
 * @param val 要显示的数值
 */
void LCD_ShowInfo(uint16_t x, uint16_t y, char *msg, uint32_t val) {
    LCD_ShowString(x, y, 320, 16, 16, (uint8_t *)msg);
    LCD_ShowxNum(x + 8 * strlen(msg), y, val, 2, 16, 1);
}

/* USER CODE END 0 */

/**
  * @brief  The application entry point.
  * @retval int
  */
int main(void)
{
  /* USER CODE BEGIN 1 */
	
  /* USER CODE END 1 */

  /* MCU Configuration--------------------------------------------------------*/

  /* Reset of all peripherals, Initializes the Flash interface and the Systick. */
  HAL_Init();
  SystemClock_Config();
  /* USER CODE BEGIN Init */
	uart_init(9600); 			// 串口初始化，波特率9600
	delay_init(168);			// 延时函数初始化，系统主频168MHz
	usmart_dev.init(84);		// USMART调试组件初始化
	delay_ms(1000);				// 等待外设稳定
	LCD_Init();					// LCD初始化
	LCD_Init();					// 再次初始化，确保稳定（可保留或删除）
	LCD_Clear(WHITE);			// 清屏为白色
	Key_Init();					// 按键初始化
  /* USER CODE END Init */

  /* USER CODE BEGIN SysInit */       			
	SRAM_Init();				// 外部SRAM初始化
	W25QXX_Init();				// W25QXX SPI Flash初始化
	WM8978_Init();				// WM8978音频编解码芯片初始化
	WM8978_I2S_Cfg(2, 0);		// I2S接口配置：主模式，16位数据
	WM8978_HPvol_Set(50,50);	// 耳机音量设置：左右声道各50%
	WM8978_SPKvol_Set(50);		// 扬声器音量设置：50%

	// 为内部、外部及CCM内存初始化内存管理
	my_mem_init(SRAMIN);
	my_mem_init(SRAMEX);
	my_mem_init(SRAMCCM);

	exfuns_init();				// FatFS相关扩展函数初始化
	f_mount(fs[0],"0:",1);		// 挂载SD卡（盘符0:）
  /* USER CODE END SysInit */

  /* Initialize all configured peripherals */
  /* USER CODE BEGIN 2 */
	RTC_Init();					// 实时时钟初始化
	BEEP_Init();				// 蜂鸣器初始化
	Init_ADC();					// ADC初始化（用于电池电压检测等）
	LCD_Backlight_Init();		// LCD背光控制初始化
	
	EEPROM_Init();  			// 初始化EEPROM（24C02等）
	DAC_Wave_Init();			// DAC波形输出初始化（用于提示音等）
		
	atk_rm04_init();			// ATK-RM04 WiFi模块初始化
  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
		Alarm_BEEP_Task();		// 闹钟蜂鸣器任务（非阻塞）
		Menu_Update();			// 菜单界面刷新
		Clock_Voice();			// 整点报时/语音时钟
    /* USER CODE END WHILE */
		
		wav_play_poll();		// 音频播放轮询（播放SD卡WAV文件）
    /* USER CODE BEGIN 3 */
  }
  /* USER CODE END 3 */
}

/**
  * @brief System Clock Configuration
  * @retval None
  */
void SystemClock_Config(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

  __HAL_RCC_PWR_CLK_ENABLE();
  __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE1);

  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
  RCC_OscInitStruct.HSEState = RCC_HSE_ON;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;

  RCC_OscInitStruct.PLL.PLLM = 8;      	// PLLM: 外部8MHz晶振 / 8 = 1MHz
  RCC_OscInitStruct.PLL.PLLN = 336;   	// PLLN: 1MHz * 336 = 336MHz
  RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV2; // PLLP: 336MHz / 2 = 168MHz (SYSCLK)
  RCC_OscInitStruct.PLL.PLLQ = 7;     	// PLLQ: 336MHz / 7 = 48MHz (USB/SDIO等)

  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK
                              | RCC_CLOCKTYPE_SYSCLK
                              | RCC_CLOCKTYPE_PCLK1
                              | RCC_CLOCKTYPE_PCLK2;

  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK; // 选择PLL作为系统时钟
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;        // AHB  168MHz
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV4;         // APB1 42MHz (定时器时钟=84MHz)
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV2;         // APB2 84MHz (定时器时钟=168MHz)

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_5) != HAL_OK)
  {
    Error_Handler();
  }
}

/* USER CODE BEGIN 4 */

/* USER CODE END 4 */

/**
  * @brief  This function is executed in case of error occurrence.
  * @retval None
  */
void Error_Handler(void)
{
  /* USER CODE BEGIN Error_Handler_Debug */
  /* User can add his own implementation to report the HAL error return state */
  __disable_irq();
  while (1)
  {
    // 死循环：错误发生后的最终兜底
  }
  /* USER CODE END Error_Handler_Debug */
}

#ifdef  USE_FULL_ASSERT
/**
  * @brief  Reports the name of the source file and the source line number
  *         where the assert_param error has occurred.
  * @param  file: pointer to the source file name
  * @param  line: assert_param error line source number
  * @retval None
  */
void assert_failed(uint8_t *file, uint32_t line)
{
  /* USER CODE BEGIN 6 */
  /* User can add his own implementation to report the file name and line number,
     ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
  /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */











