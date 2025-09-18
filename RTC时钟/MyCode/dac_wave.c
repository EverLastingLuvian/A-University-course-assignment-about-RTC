#include "dac_wave.h"
#include "math.h"
#include "stdlib.h"
#include "string.h"
#include "dackey.h"
#include "stdio.h"  
#include "lcd.h"
#include "eeprom.h"
#include "Menu.h"
#include <math.h>


extern void Error_Handler(void);
void DAC_Wave_Generate(void);
uint16_t wave_buffer[400] = {0}; // 波形显示缓冲区
uint8_t first_draw = 1;

// 在dac_wave.h中定义双缓冲
uint16_t wave_buffer1[1024];
uint16_t wave_buffer2[1024];
uint8_t current_buffer = 0;
uint16_t *act_buffer; // 当前DMA正在使用的缓冲区（供显示用）
uint16_t *backup_buffer;  // 备用缓冲区（供新数据生成用）

uint8_t BRE = 0;
// DAC配置实例
DAC_Config dac_config = {
    .type = WAVE_SINE,
    .frequency = 1.0f,  // 默认1kHz
    .amplitude = 1.0f,  // 默认1Vpp
    .wave_data = NULL,
    .data_size = 256    // 波形数据点数
};

// 全局句柄
DAC_HandleTypeDef hdac;
DMA_HandleTypeDef hdma_dac;
TIM_HandleTypeDef htim6;

// 操作模式
OperationMode current_mode = MODE_WAVE_SELECT;

#define DAC_MAX_SIZE 2048   // 誹惠璶эゲ? ? dac_config.data_size

// ??だ皌 DMA ノ猧???磷 malloc
static uint16_t dac_wave_data[DAC_MAX_SIZE];

void DAC_Wave_Init(void) {
    DAC_ChannelConfTypeDef sConfig = {0};
    GPIO_InitTypeDef GPIO_InitStruct = {0};
    
    act_buffer = wave_buffer1;       // DMA ???1
    backup_buffer = wave_buffer2;    // DMA ????2
    dac_config.wave_data = act_buffer; // 纐? act_buffer
        
    // ㄏ GPIOA / DAC / DMA1 / TIM6 ??
    __HAL_RCC_GPIOA_CLK_ENABLE();
    __HAL_RCC_DAC_CLK_ENABLE();
    __HAL_RCC_DMA1_CLK_ENABLE();
    __HAL_RCC_TIM6_CLK_ENABLE();
    
    // 皌竚 PA4 ? DAC ?
    GPIO_InitStruct.Pin = GPIO_PIN_4;
    GPIO_InitStruct.Mode = GPIO_MODE_ANALOG;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);
    
    // ﹍て DAC
    hdac.Instance = DAC;
    if (HAL_DAC_Init(&hdac) != HAL_OK) {
        Error_Handler();
    }
    
    // 皌竚 DAC1 ㄏノ TIM6 郉?
    sConfig.DAC_Trigger = DAC_TRIGGER_T6_TRGO;
    sConfig.DAC_OutputBuffer = DAC_OUTPUTBUFFER_ENABLE;
    if (HAL_DAC_ConfigChannel(&hdac, &sConfig, DAC_CHANNEL_1) != HAL_OK) {
        Error_Handler();
    }
    
    // ﹍て DMA
    hdma_dac.Instance = DMA1_Stream5;
    hdma_dac.Init.Channel = DMA_CHANNEL_7;
    hdma_dac.Init.Direction = DMA_MEMORY_TO_PERIPH;
    hdma_dac.Init.PeriphInc = DMA_PINC_DISABLE;
    hdma_dac.Init.MemInc = DMA_MINC_ENABLE;
    hdma_dac.Init.PeriphDataAlignment = DMA_PDATAALIGN_HALFWORD;
    hdma_dac.Init.MemDataAlignment = DMA_MDATAALIGN_HALFWORD;
    hdma_dac.Init.Mode = DMA_CIRCULAR;
    hdma_dac.Init.Priority = DMA_PRIORITY_HIGH;
    hdma_dac.Init.FIFOMode = DMA_FIFOMODE_DISABLE;
    if (HAL_DMA_Init(&hdma_dac) != HAL_OK) {
        Error_Handler();
    }  
    
    __HAL_LINKDMA(&hdac, DMA_Handle1, hdma_dac);
    
    // ﹍て TIM6
    htim6.Instance = TIM6;
    htim6.Init.Prescaler = 0;
    htim6.Init.CounterMode = TIM_COUNTERMODE_UP;
    htim6.Init.Period = 839;  // 84MHz/840 = 100kHz
    htim6.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_ENABLE;
    
    HAL_NVIC_SetPriority(TIM6_DAC_IRQn, 0, 0);
    HAL_NVIC_EnableIRQ(TIM6_DAC_IRQn);
    
    if (HAL_TIM_Base_Init(&htim6) != HAL_OK) {
        Error_Handler();
    }
    TIM_MasterConfigTypeDef sMasterConfig = {0};
    sMasterConfig.MasterOutputTrigger = TIM_TRGO_UPDATE;
    sMasterConfig.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE;
    if (HAL_TIMEx_MasterConfigSynchronization(&htim6, &sMasterConfig) != HAL_OK) {
        Error_Handler();
    }

    if (dac_config.data_size > DAC_MAX_SIZE) {
        Error_Handler();
    }
    dac_config.wave_data = dac_wave_data;

    for (int i = 0; i < dac_config.data_size; i++) {
        dac_config.wave_data[i] = 300;  
    }

    DAC_Wave_LoadFromEEPROM();

    DAC_Wave_Generate();
}




// 生成波形数据
void DAC_Wave_Generate(void) {
    // ?琩猧???
    if (dac_config.wave_data == NULL) return;

    // 谔玂Τ??
    if (dac_config.data_size == 0) dac_config.data_size = 1;
    if (dac_config.frequency <= 0) dac_config.frequency = 0.1f;
    if (dac_config.amplitude < 0) dac_config.amplitude = 0;

    // ??ヘ????
    uint16_t* target_buffer = current_buffer ? wave_buffer1 : wave_buffer2;

    // ?衡碩?
    float amplitude_scale = (dac_config.amplitude / 3.3f) * 4095.0f;
    if (amplitude_scale > 4095) amplitude_scale = 4095.0f;

    uint16_t mid_value = 2048;  // 1.65V ?? DAC 12bit

    // 誹猧?ネΘ?誹
    switch (dac_config.type) {
        case WAVE_SINE: {
            float inc = (2 * 3.1415926535f) / dac_config.data_size;
            for (int i = 0; i < dac_config.data_size; i++) {
                float outdata = mid_value + (amplitude_scale / 2) * sinf(inc * i);
                if (outdata > 4095) outdata = 4095;
                if (outdata < 0) outdata = 0;
                target_buffer[i] = (uint16_t)outdata;
            }
            break;
        }

        case WAVE_SQUARE: {
            for (int i = 0; i < dac_config.data_size; i++) {
                target_buffer[i] = (i < dac_config.data_size / 2) ?
                                   mid_value + amplitude_scale / 2 :
                                   mid_value - amplitude_scale / 2;
                if (target_buffer[i] > 4095) target_buffer[i] = 4095;
                if (target_buffer[i] < 0) target_buffer[i] = 0;
            }
            break;
        }

        case WAVE_TRIANGLE: {
            int half_size = dac_config.data_size / 2;
            if (half_size == 0) half_size = 1;
            for (int i = 0; i < dac_config.data_size; i++) {
                if (i < half_size) {
                    target_buffer[i] = mid_value - amplitude_scale / 2 +
                                       (amplitude_scale * i) / half_size;
                } else {
                    target_buffer[i] = mid_value + amplitude_scale / 2 -
                                       (amplitude_scale * (i - half_size)) / half_size;
                }
                if (target_buffer[i] > 4095) target_buffer[i] = 4095;
                if (target_buffer[i] < 0) target_buffer[i] = 0;
            }
            break;
        }

        case WAVE_OFF:
        default:
            for (int i = 0; i < dac_config.data_size; i++) {
                target_buffer[i] = mid_value;
            }
            break;
    }

    // ?衡﹚?竟㏄戳
    uint32_t timer_freq = (uint32_t)(dac_config.frequency * 1000.0f * dac_config.data_size);
    if (timer_freq == 0) timer_freq = 1;  // ňゎ埃箂

    uint32_t timer_period = (uint32_t)(84000000.0f / timer_freq);
    if (timer_period < 2) timer_period = 2;
    if (timer_period > 0xFFFF) timer_period = 0xFFFF;

    __HAL_TIM_SET_PRESCALER(&htim6, 0);
    __HAL_TIM_SET_AUTORELOAD(&htim6, timer_period - 1);

    // ち? DMA ???
    HAL_DAC_Stop_DMA(&hdac, DAC_CHANNEL_1);
    dac_config.wave_data = target_buffer;
    current_buffer = !current_buffer;

    if (HAL_DAC_Start_DMA(&hdac, DAC_CHANNEL_1,
                          (uint32_t*)dac_config.wave_data,
                          dac_config.data_size, DAC_ALIGN_12B_R) != HAL_OK) {
        Error_Handler();
    }
}

// 启动波形输出
void DAC_Wave_Start(void) {
    // 先停止之前的传输
    HAL_DAC_Stop_DMA(&hdac, DAC_CHANNEL_1);
    HAL_TIM_Base_Stop(&htim6);
    
    DAC_Wave_Generate();
    
    // 启动TIM7
	  if (HAL_TIM_Base_Start(&htim6) != HAL_OK) {
        Error_Handler();
    }
    
}

// 停止波形输出
void DAC_Wave_Stop(void) {
    HAL_TIM_Base_Stop(&htim6);
    HAL_DAC_Stop_DMA(&hdac, DAC_CHANNEL_1);
}

// 设置波形类型
void DAC_Wave_SetType(WaveType type) {
    dac_config.type = type;
    DAC_Wave_Generate();
    DAC_Wave_SaveToEEPROM();
}

// 设置频率 (0.1-2.0kHz)
void DAC_Wave_SetFrequency(float freq) {
    if (freq < 0.1f) freq = 0.1f;
    if (freq > 2.0f) freq = 2.0f;
    
    dac_config.frequency = freq;
    
    // 更新定时器频率
    uint32_t timer_freq = (uint32_t)(freq * 1000 * dac_config.data_size);
    uint32_t timer_period = (uint32_t)(84000000.0f / timer_freq);
    
    if (timer_period < 2) timer_period = 2;
    if (timer_period > 0xFFFF) timer_period = 0xFFFF;
    
    __HAL_TIM_SET_AUTORELOAD(&htim6, timer_period - 1);
    
    DAC_Wave_SaveToEEPROM();
}

// 设置幅度 (0.5-2.0V)
void DAC_Wave_SetAmplitude(float amp) {
    if (amp < 0.5f) amp = 0.5f;
    if (amp > 2.0f) amp = 2.0f;
    
    dac_config.amplitude = amp;
    DAC_Wave_Generate();
    DAC_Wave_SaveToEEPROM();
}

// 获取当前操作模式
OperationMode DAC_Wave_GetCurrentMode(void) {
    return current_mode;
}

// 保存设置到EEPROM
void DAC_Wave_SaveToEEPROM(void) {
    uint8_t data[sizeof(dac_config.type) + sizeof(dac_config.frequency) + sizeof(dac_config.amplitude)];
    memcpy(data, &dac_config.type, sizeof(dac_config.type));
    memcpy(data + sizeof(dac_config.type), &dac_config.frequency, sizeof(dac_config.frequency));
    memcpy(data + sizeof(dac_config.type) + sizeof(dac_config.frequency), 
           &dac_config.amplitude, sizeof(dac_config.amplitude));
    
    // 使用EEPROM_WriteBytes函数
    EEPROM_WriteBytes(0, data, sizeof(data));
}

// 从EEPROM加载设置
void DAC_Wave_LoadFromEEPROM(void) {
    uint8_t data[sizeof(dac_config.type) + sizeof(dac_config.frequency) + sizeof(dac_config.amplitude)];
    
    // ? EEPROM
    EEPROM_ReadBytes(0, data, sizeof(data));
    
    // memcpy ?疼蔨
    memcpy(&dac_config.type, data, sizeof(dac_config.type));
    memcpy(&dac_config.frequency, data + sizeof(dac_config.type), sizeof(dac_config.frequency));
    memcpy(&dac_config.amplitude, 
           data + sizeof(dac_config.type) + sizeof(dac_config.frequency), 
           sizeof(dac_config.amplitude));
    
    // ?琩谔玂猧?猭
    if (dac_config.type > WAVE_TRIANGLE) {
        dac_config.type = WAVE_SINE;
    }
    
    // ?琩?瞯
    if (isnan(dac_config.frequency) || dac_config.frequency < 0.1f || dac_config.frequency > 2.0f) {
        dac_config.frequency = 1.0f;  // 纐? 1 kHz
    }
    
    // ?琩碩
    if (isnan(dac_config.amplitude) || dac_config.amplitude < 0.5f || dac_config.amplitude > 2.0f) {
        dac_config.amplitude = 1.0f;  // 纐? 1 Vpp
    }
}
// 在LCD上显示波形信息
void DAC_Wave_UpdateDisplay(void) {
    uint8_t buffer[50];
    static uint8_t animation_frame = 0;
    
		// 定义最大文本长度（根据最长可能的文本确定）
#define MAX_WAVE_TEXT_LEN 8  // "TRIANGLE" 是最长的，8个字符
#define MAX_VALUE_TEXT_LEN 8 // "2.0 kHz" 或 "2.0 V" 是7个字符，加1个缓冲
		
    // 保存当前颜色
    uint16_t old_color = POINT_COLOR;
    
    // 首次绘制时绘制所有静态内容
    if (first_draw) {
        // 清除整个屏幕
        LCD_Clear(WHITE);
        
        // 显示标题 - 顶部居中
        POINT_COLOR = BLUE;
        LCD_ShowString(300, 10, 200, 32, 32, (uint8_t*)"DAC WAVE GEN");
        
        // 绘制分隔线
        LCD_DrawLine(10, 50, 790, 50);  // 标题下方分隔线
        LCD_DrawLine(400, 50, 400, 430); // 垂直分隔线，分离参数和波形
        LCD_DrawLine(10, 430, 790, 430); // 水平分隔线，分离控制说明
        
        // 底部控制说明区域
        POINT_COLOR = BLACK;
        LCD_ShowString(30, 450, 740, 24, 24, (uint8_t*)"UP:Mode  KEY0:Inc  KEY1:Dec  ");
        
        first_draw = 0;
    }
    
    // 只更新需要变化的区域，而不是整个屏幕
    
    // 更新当前模式显示 - 顶部右侧，使用红色突出
    POINT_COLOR = BLACK;
    LCD_Fill(600, 15, 780, 39, WHITE); // 清除旧模式显示区域
    POINT_COLOR = RED;
    switch (current_mode) {
        case MODE_WAVE_SELECT:
            LCD_ShowString(600, 15, 180, 24, 24, (uint8_t*)"[WAVE SELECT]");
            break;
        case MODE_FREQ_ADJUST:
            LCD_ShowString(600, 15, 180, 24, 24, (uint8_t*)"[FREQ ADJUST]");
            break;
        case MODE_VPP_ADJUST:
            LCD_ShowString(600, 15, 180, 24, 24, (uint8_t*)"[VPP ADJUST]");
            break;
    }
    
    // 参数显示区域 - 左侧 (10-400像素宽度)
    POINT_COLOR = BLACK;
    LCD_ShowString(30, 70, 120, 24, 24, (uint8_t*)"Wave Type:");
    // 波形类型显示 - 突出显示当前选择
    LCD_Fill(40, 110, 40 + (MAX_WAVE_TEXT_LEN * 24), 110 + 24, WHITE); // 根据最大长度清除
    if (current_mode == MODE_WAVE_SELECT) {
        LCD_Fill(25, 100, 370, 40, LGRAY);
        POINT_COLOR = RED;
    } else {
        POINT_COLOR = BLACK;
    }
    
    
    switch (dac_config.type) {
        case WAVE_SINE: 
            LCD_ShowString(40, 110, 120, 24, 24, (uint8_t*)"SINE"); 
            break;
        case WAVE_SQUARE: 
            LCD_ShowString(40, 110, 120, 24, 24, (uint8_t*)"SQUARE"); 
            break;
        case WAVE_TRIANGLE: 
            LCD_ShowString(40, 110, 120, 24, 24, (uint8_t*)"TRIANGLE"); 
            break;
        default: 
            LCD_ShowString(40, 110, 120, 24, 24, (uint8_t*)"OFF"); 
            break;
    }
    
    // 频率显示 - 突出显示当前选择
   LCD_Fill(40, 190, 40 + (MAX_VALUE_TEXT_LEN * 24), 190 + 24, WHITE); // 根据最大长度清除
    POINT_COLOR = BLACK;
    LCD_ShowString(30, 150, 120, 24, 24, (uint8_t*)"Freq:");
    
    if (current_mode == MODE_FREQ_ADJUST) {
        LCD_Fill(25, 180, 370, 40, LGRAY);
        POINT_COLOR = RED;
    } else {
        POINT_COLOR = BLACK;
    }
    
    sprintf((char*)buffer, "%.1f kHz", dac_config.frequency);
    LCD_ShowString(40, 190, 120, 24, 24, buffer);
    
    // 幅度显示 - 突出显示当前选择
   LCD_Fill(40, 270, 40 + (MAX_VALUE_TEXT_LEN * 24), 270 + 24, WHITE); // 根据最大长度清除
    POINT_COLOR = BLACK;
    LCD_ShowString(30, 230, 120, 24, 24, (uint8_t*)"Vpp:");
    
    if (current_mode == MODE_VPP_ADJUST) {
        LCD_Fill(25, 260, 370, 40, LGRAY);
        POINT_COLOR = RED;
    } else {
        POINT_COLOR = BLACK;
    }
    
    sprintf((char*)buffer, "%.1f V", dac_config.amplitude);
    LCD_ShowString(40, 270, 120, 24, 24, buffer);
    
    // 波形预览区域 - 右侧 (400-790像素宽度, 50-430像素高度)
    if (dac_config.type != WAVE_OFF) {
        int start_x = 450;
        int start_y = 240;
        int height = 150;
        int width = 300;
        
        // 只清除波形区域，而不是整个区域
        LCD_Fill(start_x + 1, start_y - height/2 + 1, start_x + width - 1, start_y + height/2 - 1, WHITE);
        
        // 绘制坐标轴
        POINT_COLOR = BLACK;
        LCD_DrawLine(start_x, start_y, start_x + width, start_y); // X轴
        LCD_DrawLine(start_x, start_y - height/2, start_x, start_y + height/2); // Y轴
        
        // 添加坐标标签
        LCD_ShowString(start_x - 30, start_y - height/2 - 15, 60, 16, 16, (uint8_t*)"+V");
        LCD_ShowString(start_x - 30, start_y + height/2 - 15, 60, 16, 16, (uint8_t*)"-V");
        LCD_ShowString(start_x + width/2 - 30, start_y + height/2 + 10, 60, 16, 16, (uint8_t*)"Time");
        
        // 动态流动波形显示
        POINT_COLOR = BLUE;
        
        // 使用动画帧来创建动态效果
        animation_frame = (animation_frame + 2) % dac_config.data_size;
        
        // 更新波形缓冲区 - 实现从左向右流动效果
        for (int i = 0; i < width - 1; i++) {
            int index = (i + animation_frame) % dac_config.data_size;
            int value = start_y - ((dac_config.wave_data[index] * height) / 4096) + height/2;
            
            // 限制在预览区域内
            if (value < start_y - height/2) value = start_y - height/2;
            if (value > start_y + height/2) value = start_y + height/2;
            
            // 保存到波形缓冲区
            wave_buffer[i] = value;
        }
        
        // 绘制新波形线
for (int i = 0; i < width - 2; i++) {  // 改为width-2，避免超出边界
    int index1 = (i + animation_frame) % dac_config.data_size;
    int index2 = (i + 1 + animation_frame) % dac_config.data_size;
    int value1 = start_y - ((dac_config.wave_data[index1] * height) / 4096) + height/2;
    int value2 = start_y - ((dac_config.wave_data[index2] * height) / 4096) + height/2;
    
    // 限制在预览区域内
    if (value1 < start_y - height/2) value1 = start_y - height/2;
    if (value1 > start_y + height/2) value1 = start_y + height/2;
    if (value2 < start_y - height/2) value2 = start_y - height/2;
    if (value2 > start_y + height/2) value2 = start_y + height/2;
    
    // 确保不超出右侧边界
    if (start_x + i + 1 < start_x + width) {
        LCD_DrawLine(start_x + i, value1, start_x + i + 1, value2);
    }
}
        
        // 添加网格线
        POINT_COLOR = LGRAY;
        // 水平网格线
        LCD_DrawLine(start_x, start_y - height/4, start_x + width, start_y - height/4);
        LCD_DrawLine(start_x, start_y + height/4, start_x + width, start_y + height/4);
        // 垂直网格线
        LCD_DrawLine(start_x + width/4, start_y - height/2, start_x + width/4, start_y + height/2);
        LCD_DrawLine(start_x + width/2, start_y - height/2, start_x + width/2, start_y + height/2);
        LCD_DrawLine(start_x + 3*width/4, start_y - height/2, start_x + 3*width/4, start_y + height/2);
    } else {
        // 如果波形关闭，清除波形区域
        LCD_Fill(401, 51, 789, 429, WHITE);
    }
    
    // 恢复原颜色
    POINT_COLOR = old_color;
}

// 处理按键输入
void DAC_Wave_ProcessKeyInput(uint8_t key) {
    switch (key) {
        case KEY_UP: 
            current_mode = (OperationMode)((current_mode + 1) % 3);
            break;
            
        case KEY_RIGHT: 
            switch (current_mode) {
                case MODE_WAVE_SELECT:
                    dac_config.type = (WaveType)((dac_config.type + 1) % 4);
                    break;
                case MODE_FREQ_ADJUST:
                    if (dac_config.frequency < 2.0f) {
                        dac_config.frequency += 0.1f;
                    }
                    break;
                case MODE_VPP_ADJUST:
                    if (dac_config.amplitude < 2.0f) {
                        dac_config.amplitude += 0.1f;
                    }
                    break;
            }
            DAC_Wave_Generate();
            break;
            
        case KEY_DOWM: 
            switch (current_mode) {
                case MODE_WAVE_SELECT:
                    dac_config.type = (WaveType)((dac_config.type + 3) % 4); 
                    break;
                case MODE_FREQ_ADJUST:
                    if (dac_config.frequency > 0.1f) {
                        dac_config.frequency -= 0.1f;
                    }
                    break;
                case MODE_VPP_ADJUST:
                    if (dac_config.amplitude > 0.5f) {
                        dac_config.amplitude -= 0.1f;
                    }
                    break;
            }
            DAC_Wave_Generate();
            break;

        case KEY_LEFT: // 癶
            BRE = 1;
            break;
    }
}


void DAC_Wave_ResetDisplay(void) {
    first_draw = 1;
    DAC_Wave_UpdateDisplay();
}
