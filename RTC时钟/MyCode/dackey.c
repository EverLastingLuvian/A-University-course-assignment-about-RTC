//#include "dackey.h"

//// 按键初始化
//void KEY_Init(void) {
//    GPIO_InitTypeDef GPIO_InitStruct = {0};
//    
//    // 使能GPIO时钟
//    __HAL_RCC_GPIOA_CLK_ENABLE();
//    __HAL_RCC_GPIOE_CLK_ENABLE();
//    
//    // KEY0(PE4), KEY1(PE3), KEY2(PE2) 配置为上拉输入
//    GPIO_InitStruct.Pin = KEY0_PIN | KEY1_PIN | KEY2_PIN;
//    GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
//    GPIO_InitStruct.Pull = GPIO_PULLUP;
//    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
//    HAL_GPIO_Init(KEY0_PORT, &GPIO_InitStruct);
//    
//    // KEY_UP(PA0) 配置为下拉输入
//    GPIO_InitStruct.Pin = KEY_UP_PIN;
//    GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
//    GPIO_InitStruct.Pull = GPIO_PULLDOWN;
//    HAL_GPIO_Init(KEY_UP_PORT, &GPIO_InitStruct);
//}

//// 按键扫描函数
//uint8_t KEY_Scan(void) {
//    static uint8_t key_up = 1; // 按键松开标志
//    
//    // 检测是否有按键按下
//    if (key_up && (HAL_GPIO_ReadPin(KEY_UP_PORT, KEY_UP_PIN) == GPIO_PIN_SET || 
//                   HAL_GPIO_ReadPin(KEY0_PORT, KEY0_PIN) == GPIO_PIN_RESET ||
//                   HAL_GPIO_ReadPin(KEY1_PORT, KEY1_PIN) == GPIO_PIN_RESET ||
//                   HAL_GPIO_ReadPin(KEY2_PORT, KEY2_PIN) == GPIO_PIN_RESET)) {
//        HAL_Delay(10); // 去抖动
//        key_up = 0;
//        
//        // 确定具体是哪个按键按下
//        if (HAL_GPIO_ReadPin(KEY_UP_PORT, KEY_UP_PIN) == GPIO_PIN_SET) {
//            return KEY_UP;
//        } else if (HAL_GPIO_ReadPin(KEY0_PORT, KEY0_PIN) == GPIO_PIN_RESET) {
//            return KEY0;
//        } else if (HAL_GPIO_ReadPin(KEY1_PORT, KEY1_PIN) == GPIO_PIN_RESET) {
//            return KEY1;
//        } else if (HAL_GPIO_ReadPin(KEY2_PORT, KEY2_PIN) == GPIO_PIN_RESET) {
//            return KEY2;
//        }
//    } 
//    // 检测按键是否松开
//    else if (HAL_GPIO_ReadPin(KEY_UP_PORT, KEY_UP_PIN) == GPIO_PIN_RESET && 
//             HAL_GPIO_ReadPin(KEY0_PORT, KEY0_PIN) == GPIO_PIN_SET &&
//             HAL_GPIO_ReadPin(KEY1_PORT, KEY1_PIN) == GPIO_PIN_SET &&
//             HAL_GPIO_ReadPin(KEY2_PORT, KEY2_PIN) == GPIO_PIN_SET) {
//        key_up = 1;
//    }
//    
//    return KEY_NONE; // 无按键按下
//}
//// 确保末尾有空行
