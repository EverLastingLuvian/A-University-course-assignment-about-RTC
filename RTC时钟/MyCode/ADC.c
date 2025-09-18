#include "ADC.h"

/* -------------------- 全局变量 -------------------- */
__IO uint16_t adc_value[SAMPLE_SIZE];   /* 原始 ADC 码值 */
float       voltage[SAMPLE_SIZE];       /* 换算后的电压值 */
char        wave_type[20] = "Unknown";  /* 波形种类字符串 */
float       frequency = 0;              /* 计算得到的频率 */
float       vpp       = 0;              /* 峰峰值 */

/* HAL 句柄 */
ADC_HandleTypeDef hadc1;


/* 点亮 LCD 背光：PF10 接 N-MOS 或 P-MOS 控制背光 LED
 * 高电平点亮，低电平关闭。 */
void LCD_Backlight_Init(void)
{
    GPIO_InitTypeDef GPIO_InitStruct;
    __HAL_RCC_GPIOF_CLK_ENABLE();
    GPIO_InitStruct.Pin  = GPIO_PIN_10;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(GPIOF, &GPIO_InitStruct);
    HAL_GPIO_WritePin(GPIOF, GPIO_PIN_10, GPIO_PIN_SET); /* 背光开 */
}


/* ADC1 初始化：
 *  - 12 bit 分辨率，右对齐
 *  - 连续转换模式（方便后面循环采样）
 *  - 采样时间 15 个周期，@84 MHz ADCCLK 实际 15*4/84M≈0.7 µs
 *    对于 50 kHz 采样来说足够快，但注意总转换时间还要加采样保持。
 * 隐患：
 *  - 没开 DMA，主循环里靠 delay_us 硬等待 20 µs， jitter 很大。
 *  - 连续转换模式下每次 HAL_ADC_Stop 再 Start 会浪费 >5 µs。
 * 改进：
 *  - 用 TIM2 触发 ADC，DMA 双缓冲，采样间隔由定时器保证。
 *  - 采样时间可拉长到 84 周期，提高输入阻抗容忍度。*/
void Init_ADC(void)
{
    GPIO_InitTypeDef GPIO_InitStruct;
    ADC_ChannelConfTypeDef sConfig;

    __HAL_RCC_ADC1_CLK_ENABLE();
    __HAL_RCC_GPIOA_CLK_ENABLE();

    /* PA5 -> ADC1_IN0 */
    GPIO_InitStruct.Pin  = GPIO_PIN_5;
    GPIO_InitStruct.Mode = GPIO_MODE_ANALOG;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

    hadc1.Instance                   = ADC1;
    hadc1.Init.ClockPrescaler        = ADC_CLOCK_SYNC_PCLK_DIV4; /* ADCCLK=84 MHz/4=21 MHz */
    hadc1.Init.Resolution            = ADC_RESOLUTION_12B;
    hadc1.Init.ScanConvMode          = DISABLE;
    hadc1.Init.ContinuousConvMode    = ENABLE;
    hadc1.Init.DiscontinuousConvMode = DISABLE;
    hadc1.Init.ExternalTrigConvEdge  = ADC_EXTERNALTRIGCONVEDGE_NONE;
    hadc1.Init.ExternalTrigConv      = ADC_SOFTWARE_START;
    hadc1.Init.DataAlign             = ADC_DATAALIGN_RIGHT;
    hadc1.Init.NbrOfConversion       = 1;
    hadc1.Init.DMAContinuousRequests = DISABLE;
    hadc1.Init.EOCSelection          = ADC_EOC_SINGLE_CONV;
    if (HAL_ADC_Init(&hadc1) != HAL_OK) { Error_Handler(); }

    sConfig.Channel      = ADC_CHANNEL_5;
    sConfig.Rank         = 1;
    sConfig.SamplingTime = ADC_SAMPLETIME_15CYCLES;
    if (HAL_ADC_ConfigChannel(&hadc1, &sConfig) != HAL_OK) { Error_Handler(); }
}


/* 读取一次 ADC 结果，阻塞式
 * 注意：连续转换模式已经打开，这里 Start/Stop 纯粹是为了取一次值，
 * 频繁调用会降低效率；如果改成 DMA 循环缓冲区就不需要这一步。 */
uint16_t Read_ADC(void)
{
    uint16_t value;
    if (HAL_ADC_Start(&hadc1) != HAL_OK)      { Error_Handler(); }
    if (HAL_ADC_PollForConversion(&hadc1, 100)!= HAL_OK){ Error_Handler(); }
    value = HAL_ADC_GetValue(&hadc1);
    HAL_ADC_Stop(&hadc1);
    return value;
}

/* ******************************************************************
 * 波形识别算法（极简版）
 * ******************************************************************/
void identify_waveform(void)
{
    int   i;
    float max_v = 0.0f;
    float min_v = 3.3f;
    float sum   = 0.0f;
    int   zero_crossings = 0;
    int   rising_edges   = 0;
    float threshold;
    float avg;
    float rise_time_ratio;

    /* 扫描最大、最小、平均 */
    for (i = 0; i < SAMPLE_SIZE; i++) {
        if (voltage[i] > max_v) max_v = voltage[i];
        if (voltage[i] < min_v) min_v = voltage[i];
        sum += voltage[i];
    }
    avg = sum / SAMPLE_SIZE;
    vpp = (max_v - min_v)*2;

    threshold = vpp * 0.1f;          /* 经验阈值，可改自适应 */
    for (i = 1; i < SAMPLE_SIZE; i++) {
        /* 过零：穿过平均值就算 */
        if ((voltage[i-1] < avg && voltage[i] >= avg) ||
            (voltage[i-1] > avg && voltage[i] <= avg))
            zero_crossings++;

        /* 上升沿：斜率 > 阈值 且高于平均 */
        if ((voltage[i] - voltage[i-1] > threshold) &&
            (voltage[i] > avg + threshold/2))
            rising_edges++;
    }

    /* 频率 = 过零次数/2 * 每周期样本数 */
    if (zero_crossings >= 4)
        frequency = ((zero_crossings ) * (SAMPLE_RATE / (float)SAMPLE_SIZE))*0.76923;
    else
        frequency = 0;

    /* 陡度判型 */
    if (frequency > 0) {
        rise_time_ratio = (float)rising_edges / (SAMPLE_SIZE / 2.0f);
        if (rise_time_ratio < 0.2f)
            sprintf(wave_type, "Sine Wave");
        else if (rise_time_ratio > 0.8f)
            sprintf(wave_type, "Square Wave");
        else
            sprintf(wave_type, "Triangle Wave");
    } else {
        sprintf(wave_type, "No Signal");
    }
}

/* ******************************************************************
 * LCD 呈现结果：文字 + 简易波形图
 * ******************************************************************/
void display_results(void)
{
    int  i;
    int  x1, y1, x2, y2;
    char buf[64];

    LCD_Clear(WHITE);
    POINT_COLOR = BLUE;
    LCD_ShowString(30, 10, 200, 16, 16, (uint8_t *)"Waveform Analyzer");

    POINT_COLOR = RED;
    LCD_ShowString(30, 30, 200, 16, 16, (uint8_t *)"Type:");
    LCD_ShowString(100,30, 200, 16, 16, (uint8_t *)wave_type);

    LCD_ShowString(30, 50, 200, 16, 16, (uint8_t *)"Freq:");
    if (frequency < 1000.0f && frequency > 0.1f) {
        sprintf(buf, "%.2f Hz", frequency);
    } else if (frequency >= 1000.0f) {
        sprintf(buf, "%.2f kHz", frequency/10000.0f);
    } else {
        sprintf(buf, "N/A");
    }
    LCD_ShowString(100, 50, 200, 16, 16, (uint8_t *)buf);

    sprintf(buf, "Voltage: %.2f V", voltage[SAMPLE_SIZE-1]);
    LCD_ShowString(30, 70, 200, 16, 16, (uint8_t *)buf);

    sprintf(buf, "Vpp: %.2f V", vpp);
    LCD_ShowString(30, 90, 200, 16, 16, (uint8_t *)buf);

    /* 画坐标轴 */
    POINT_COLOR = BLACK;
    LCD_DrawLine(20, 200, 300, 200);  /* 时间轴 */
    LCD_DrawLine(20, 150,  20, 300);  /* 电压轴 */
    LCD_ShowString(305, 145, 200, 12, 12, (uint8_t *)"t");
    LCD_ShowString(5,  130, 200, 12, 12, (uint8_t *)"V");

    /* 画波形 */
    POINT_COLOR = RED;
    for (i = 1; i < SAMPLE_SIZE; i++) {
        x1 = 20 + (i-1) * 280 / (SAMPLE_SIZE-1);
        y1 = 200 - (int)((voltage[i-1] - 1.65f) * 70 / 1.65f);
        x2 = 20 +  i   * 280 / (SAMPLE_SIZE-1);
        y2 = 200 - (int)((voltage[i]   - 1.65f) * 70 / 1.65f);

        /*  Clamp 到屏幕区域  */
        if (y1 < 80) y1 = 80; if (y1 > 400) y1 = 400;
        if (y2 < 80) y2 = 80; if (y2 > 400) y2 = 400;
        LCD_DrawLine(x1, y1, x2, y2);
    }
}






















