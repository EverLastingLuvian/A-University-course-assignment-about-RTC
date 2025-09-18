#include "VOICE.h"
#include "RTC.h"
#include "audioplay.h"
#include <stdio.h>

extern int wav_active;  // 标志位，表示当前是否有音频正在播放

void Clock_Voice(void)
{
    // 如果已经有音频正在播放，则直接返回
    if (wav_active != 0) return;

    // 引用外部的 RTC 句柄
    extern RTC_HandleTypeDef RTC_Handler; 

    // 定义变量存储当前时间
    RTC_TimeTypeDef now_time;
    static uint8_t last_hour = 255;  // 上一次报时的小时数，初始值为 255（非法值）

    // 获取当前时间
    HAL_RTC_GetTime(&RTC_Handler, &now_time, RTC_FORMAT_BIN);

    // 提取小时、分钟和秒
    uint8_t hour   = now_time.Hours;
    uint8_t minute = now_time.Minutes;
    uint8_t second = now_time.Seconds;

    // 检查是否是整点（分钟和秒都为 0）
    if (minute == 0 && second == 0)
    {
        // 检查是否已经报过这个小时
        if (hour != last_hour)
        {
            // 更新上一次报时的小时数
            last_hour = hour;

            // 构造音频文件路径
            char path[32];
            if (hour == 0)
                sprintf(path, "0:/VOICE/24.wav");  // 24 小时报时
            else
                sprintf(path, "0:/VOICE/%02d.wav", hour);  // 格式化为两位数字，例如 "0:/VOICE/08.wav"

            // 播放整点报时音频
            audio_play_song((u8*)path);
        }
    }
}















