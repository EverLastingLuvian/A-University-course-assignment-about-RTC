#ifndef __USART3_H
#define __USART3_H

#include "main.h"
#include "usart.h"
#include "timer.h"
#include <stdarg.h>
#include <stdio.h>
#include <string.h>

/* 缓冲区大小配置 */
#define USART3_MAX_RECV_LEN     400     // 最大接收字节数
#define USART3_MAX_SEND_LEN     400     // 最大发送字节数
#define USART3_RX_EN            1       // 是否启用接收

/* 全局变量声明 */
extern uint8_t  USART3_RX_BUF[USART3_MAX_RECV_LEN]; // 接收缓冲区
extern uint8_t  USART3_TX_BUF[USART3_MAX_SEND_LEN]; // 发送缓冲区
extern uint16_t USART3_RX_STA;             // 接收状态标志

/* 函数声明 */
void MX_USART3_UART_Init(void);   // USART3 初始化
void MX_TIM7_Init(uint16_t arr, uint16_t psc); // TIM7 初始化
void u3_printf(const char *fmt, ...);      // printf 重定向

#endif /* __USART3_H */
