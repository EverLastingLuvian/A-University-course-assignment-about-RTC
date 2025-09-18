#include "delay.h" 
#include "usart3.h" 
#include "stdarg.h" 
#include "stdio.h" 
#include "string.h" 
#include "timer.h"

extern TIM_HandleTypeDef htim7;

/* 定义缓冲区 */
__ALIGN_BEGIN uint8_t USART3_TX_BUF[USART3_MAX_SEND_LEN] __ALIGN_END;
uint8_t USART3_RX_BUF[USART3_MAX_RECV_LEN];

/* 接收状态：
 * [15]：接收完成标志
 * [14:0]：接收数据长度
 */
uint16_t USART3_RX_STA = 0;

/* USART3 句柄 */
UART_HandleTypeDef huart3;

/* ---------------- USART3 初始化 ---------------- */
void MX_USART3_UART_Init()
{
    /* 打开时钟 */
    __HAL_RCC_USART3_CLK_ENABLE();
    __HAL_RCC_GPIOB_CLK_ENABLE();

    GPIO_InitTypeDef GPIO_InitStruct = {0};

    /* PB10 -> TX, PB11 -> RX */
    GPIO_InitStruct.Pin = GPIO_PIN_10 | GPIO_PIN_11;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Pull = GPIO_PULLUP;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
    GPIO_InitStruct.Alternate = GPIO_AF7_USART3;
    HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

    /* USART3 参数配置 */
    huart3.Instance = USART3;
    huart3.Init.BaudRate = 115200*3;
    huart3.Init.WordLength = UART_WORDLENGTH_8B;
    huart3.Init.StopBits = UART_STOPBITS_1;
    huart3.Init.Parity = UART_PARITY_NONE;
    huart3.Init.Mode = UART_MODE_TX_RX;
    huart3.Init.HwFlowCtl = UART_HWCONTROL_NONE;
    huart3.Init.OverSampling = UART_OVERSAMPLING_16;

    if (HAL_UART_Init(&huart3) != HAL_OK)
    {
        Error_Handler();
    }

    /* 开启接收中断 */
    __HAL_UART_ENABLE_IT(&huart3, UART_IT_RXNE);

    /* NVIC 配置 */
    HAL_NVIC_SetPriority(USART3_IRQn, 0, 0); // 提高中断优先级
    HAL_NVIC_EnableIRQ(USART3_IRQn);

    /* 初始化 TIM7，配置 100ms 超时 */
    MX_TIM7_Init(1000 - 1, 8400 - 1);  // 100ms
    USART3_RX_STA = 0;
    HAL_TIM_Base_Stop_IT(&htim7); // 默认先关闭
    
    // 不要调用 HAL_UART_Receive_IT，因为我们直接处理中断
}

/* ---------------- 串口接收中断 (HAL库写法) ---------------- */
void USART3_IRQHandler(void)
{
    uint8_t res;

    if(__HAL_UART_GET_FLAG(&huart3, UART_FLAG_RXNE) != RESET) // 收到数据
    {
        res = (uint8_t)(USART3->DR & 0xFF);  // 读数据寄存器，清除RXNE标志

        if((USART3_RX_STA & 0x8000) == 0) // 接收未完成
        {
            if(USART3_RX_STA < USART3_MAX_RECV_LEN) // 还能接收
            {
                if(USART3_RX_STA == 0)
                {
                    __HAL_TIM_SET_COUNTER(&htim7, 0);  // 清零计数
                    HAL_TIM_Base_Start_IT(&htim7);     // 开启定时器
                }
                else
                {
                    __HAL_TIM_SET_COUNTER(&htim7, 0);  // 期间不断清零计数器
                }

                USART3_RX_BUF[USART3_RX_STA++] = res; // 存数据
            }
            else
            {
                USART3_RX_STA |= 0x8000; // 超出长度，强制结束
            }
        }
    }
}



/* HAL 库 UART 接收中断回调 */
//void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart)
//{
//    if (huart->Instance == USART3)
//    {
//        uint8_t res;
//        res = (uint8_t)(huart->Instance->DR & 0xFF);

//        if ((USART3_RX_STA & (1 << 15)) == 0) // 接收未完成
//        {
//            if (USART3_RX_STA < USART3_MAX_RECV_LEN)
//            {
//                __HAL_TIM_SET_COUNTER(&htim7, 0); // 清零定时器
//                if (USART3_RX_STA == 0)
//                {
//                    HAL_TIM_Base_Start_IT(&htim7); // 启动超时定时器
//                }
//                USART3_RX_BUF[USART3_RX_STA++] = res; // 保存数据
//            }
//            else
//            {
//                USART3_RX_STA |= 1 << 15; // 接收完成
//            }
//        }
//    }
//}

/* ---------------- TIM7 超时回调 ---------------- */
//void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim)
//{
//    if (htim->Instance == TIM7)
//    {
//        USART3_RX_STA |= 1 << 15; // 标记接收完成
//        HAL_TIM_Base_Stop_IT(&htim7); // 关闭定时器
//    }
//}

/* ---------------- USART3 打印函数 ---------------- */
void u3_printf(const char *fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);
    int len = vsnprintf((char *)USART3_TX_BUF, USART3_MAX_SEND_LEN, fmt, ap);
    va_end(ap);

    if (len > 0 && len < USART3_MAX_SEND_LEN)
    {
        for (int j = 0; j < len; j++)
        {
            // 等待上一个字节发送完成
            while (__HAL_UART_GET_FLAG(&huart3, UART_FLAG_TC) == RESET);

            // 发送一个字节
            HAL_UART_Transmit(&huart3, (uint8_t *)&USART3_TX_BUF[j], 1, HAL_MAX_DELAY);
        }
    }
//	char test[] = "AT\r\n";
//	HAL_UART_Transmit(&huart3, (uint8_t*)test, strlen(test), HAL_MAX_DELAY);

}


