#include "timer.h"
#include "usart.h"
#include "lcd.h"
#include "usart3.h"

/* 外部变量 */
extern uint16_t USART3_RX_STA;

/* 定义 TIM7 句柄 */
TIM_HandleTypeDef htim7;

/* TIM7 初始化函数 */
void MX_TIM7_Init(uint16_t arr, uint16_t psc)
{
    __HAL_RCC_TIM7_CLK_ENABLE();   // 开启 TIM7 时钟

    htim7.Instance = TIM7;
    htim7.Init.Prescaler = psc;
    htim7.Init.CounterMode = TIM_COUNTERMODE_UP;
    htim7.Init.Period = arr;
    htim7.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
    htim7.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;

    if (HAL_TIM_Base_Init(&htim7) != HAL_OK)
    {
        Error_Handler();
    }

    /* 使能中断 */
    HAL_NVIC_SetPriority(TIM7_IRQn, 0, 1); // 与标准库相同的优先级
    HAL_NVIC_EnableIRQ(TIM7_IRQn);

    // 不要启动定时器，只在需要时启动
    // HAL_TIM_Base_Start_IT(&htim7);
}


/* ---------------- TIM7 中断服务函数 (HAL库写法) ---------------- */
void TIM7_IRQHandler(void)
{
    if(__HAL_TIM_GET_FLAG(&htim7, TIM_FLAG_UPDATE) != RESET)   // 更新中断触发
    {
        if(__HAL_TIM_GET_IT_SOURCE(&htim7, TIM_IT_UPDATE) != RESET) // 确认开启了更新中断
        {
            __HAL_TIM_CLEAR_IT(&htim7, TIM_IT_UPDATE);  // 清除中断标志位

            USART3_RX_STA |= 1 << 15;   // 标记接收完成
            HAL_TIM_Base_Stop_IT(&htim7); // 停止定时器（等价于 TIM_Cmd(TIM7, DISABLE)）
        }
    }
}
