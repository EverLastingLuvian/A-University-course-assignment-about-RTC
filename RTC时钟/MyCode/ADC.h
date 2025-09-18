#ifndef ADC_H
#define ADC_H
#include "main.h"
#include "sys.h"
#include "lcd.h"
#include "delay.h"
#include "math.h"
#include "stdio.h"

/* -------------------- 宏定义 -------------------- */
#define SAMPLE_SIZE   256     /* 一帧样本点数 */
#define SAMPLE_RATE   50000   /* 目标采样率 50 kHz，对应 20 µs 间隔 */

void LCD_Backlight_Init(void);
void Init_ADC(void);
uint16_t Read_ADC(void);
void identify_waveform(void);
void display_results(void);

#endif










