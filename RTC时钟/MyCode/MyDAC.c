#include "MyDAC.h"

/* --------------------- 全局变量 --------------------- */
u16 cnt = 0;              // 触摸按键计数/防抖计数 (用于短按/长按判定)

double THD;               // 总谐波失真（未在此片段中计算，但保留用于后续扩展）
double thd1, thd2, thd3, thd4, thd5 = 0; // 分频段或分次谐波值（占位）

u16 display_f_a_delay_cnt = 0;  // 控制频率/幅值显示刷新速率的计数器（UI 节流）
u16 wave_jugde = 0;           // 当前波形类别标识（例如枚举：SIN, SQU, TRI）
long long Recent_A = 0;         // 最近一次的振幅（原始 ADC 码值，范围通常 0~4095）
long long Recent_Freq = 0;      // 最近一次的频率（单位：Hz）

u32 adc_get[1024];              // ADC 采样缓冲区，保存从 ADC 获取的原始样本
                                // 注意：数组长度 1024 应与 FFT_LENGTH 保持一致，
                                //       或在拷贝时保证不会越界。

// FFT 数据缓冲区：CMSIS DSP 的复数 FFT 要求交织存放 (real, imag, real, imag, ...)
float fft_inputbuf[FFT_LENGTH * 2]; // 交织格式的输入缓冲（实部/虚部交织）
float fft_outputbuf[FFT_LENGTH];    // FFT 变换后计算得到的幅值（实数）

u16 tim_1024_judge = 0;   // 标志位：当采集到 1024 个样本（或 FFT_LENGTH 个样本）时 ISR 设置为 1

// 用于记录 FFT 峰值及其索引（峰值：幅值）
float32_t max = 0;
u32 max_index = 0;
float32_t second = 0;
u32 second_index = 0;
float32_t third = 0;
u32 third_index = 0;

u16 Freq_FFT;            // 通过 FFT 计算得到的频率（Hz）

// 记录不同波形对应的峰峰值（单位在代码中有换算，这里用 mV 或根据工程另行定义）
float Vpp_Sin = 0, Vpp_tri = 0, Vpp_Squa = 0, VPP_ALL = 0;

float32_t output_tmp[1024]; // 临时数组：用于处理 FFT 结果（注意长度应 >= FFT_LENGTH）


void MyDAC_Init(void)
{
	u16 i;

	/* ------------------ 初始化阶段 ------------------ */
	// 初始化 TIM4 中断（示例参数：预分频 420-1，自动重装载 1000-1）
	// 具体用途：代码中使用 TIM4 来控制某些定时任务（如扫频或界面动画）
	TIM4_Int_Init_2(420 - 1, 1000 - 1);
	TIM_Cmd(TIM4, DISABLE);        // 先关闭 TIM4，后续根据 sweep_judge 控制启停

	Recent_A = 4095;               // 初始振幅（ADC 码值），取最大值作为默认
	Recent_Freq = 1000;            // 初始频率（Hz），默认 1000 Hz

	Adc_Init();                    // 初始化 ADC（配置通道、中断/触发等）

	// 初始化 TIM3，用作 ADC 触发定时器。注：此处注释中写到 Fs: 20kHz，
	//      说明期望 TIM3 触发频率对应 ADC 采样率 Fs（具体计算由定时器时钟决定）。
	TIM3_Int_Init_2(42 - 1, 100 - 1);    // 期望采样率 Fs = 20 kHz（示例）

	// CMSIS-DSP FFT 结构体实例化并初始化：基于 radix-4 的复数 FFT
	arm_cfft_radix4_instance_f32 scfft;  // 注意：实例需要在使用 FFT 的作用域中保留
	arm_cfft_radix4_init_f32(&scfft, FFT_LENGTH, 0, 1);

	TFT_LCD_Init();    // LCD 硬件初始化
	bsp_GUI_Init();    // GUI 初始化（界面元素、字体、图标等）
}