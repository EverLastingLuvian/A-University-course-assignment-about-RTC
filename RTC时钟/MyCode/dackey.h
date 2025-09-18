#ifndef __KEY_H__
#define __KEY_H__

#include "stm32f4xx_hal.h"

// 添加双缓冲定义
#define WAVE_BUFFER_SIZE 256
extern uint16_t dma_buffer1[WAVE_BUFFER_SIZE];
extern uint16_t dma_buffer2[WAVE_BUFFER_SIZE];
extern volatile uint8_t active_buffer;

//// 按键引脚定义
//#define KEY_UP_PIN          GPIO_PIN_0
//#define KEY_UP_PORT         GPIOA
//#define KEY0_PIN            GPIO_PIN_4
//#define KEY0_PORT           GPIOE
//#define KEY1_PIN            GPIO_PIN_3
//#define KEY1_PORT           GPIOE
//#define KEY2_PIN            GPIO_PIN_2
//#define KEY2_PORT           GPIOE

//// 按键值定义
//#define KEY_UP              0
//#define KEY0                1
//#define KEY1                2
//#define KEY2                3
//#define KEY_NONE            255

//// 函数声明
//void KEY_Init(void);
//uint8_t KEY_Scan(void);

#endif  // 确保末尾有空行


