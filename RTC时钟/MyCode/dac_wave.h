#ifndef __DAC_WAVE_H__
#define __DAC_WAVE_H__

#include "stm32f4xx_hal.h"
#include "lcd.h"
#include "eeprom.h"

// 波形类型定义
typedef enum {
    WAVE_OFF = 0,
    WAVE_SINE,
    WAVE_SQUARE,
    WAVE_TRIANGLE
} WaveType;

// DAC配置结构体
typedef struct {
    WaveType type;
    float frequency;    // 频率 (kHz)
    float amplitude;    // 峰峰值 (V)
    uint16_t *wave_data;
    uint16_t data_size;
	

} DAC_Config;


// 操作模式
typedef enum {
    MODE_WAVE_SELECT = 0,
    MODE_FREQ_ADJUST,
    MODE_VPP_ADJUST
} OperationMode;



// 函数声明
void DAC_Wave_Init(void);
void DAC_Wave_Start(void);
void DAC_Wave_Stop(void);
void DAC_Wave_Generate(void);
void DAC_Wave_SetType(WaveType type);
void DAC_Wave_SetFrequency(float freq);
void DAC_Wave_SetAmplitude(float amp);
void DAC_Wave_UpdateDisplay(void);
void DAC_Wave_ProcessKeyInput(uint8_t key);
void DAC_Wave_SaveToEEPROM(void);
void DAC_Wave_LoadFromEEPROM(void);
void DAC_Wave_ResetDisplay(void);
OperationMode DAC_Wave_GetCurrentMode(void);

extern DAC_Config dac_config;
extern OperationMode current_mode;

#endif

