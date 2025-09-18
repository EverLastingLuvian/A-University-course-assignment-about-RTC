#ifndef __WAVPLAY_H
#define __WAVPLAY_H
#include "sys.h" 
#include <stdint.h>


#define WAV_I2S_TX_DMA_BUFSIZE    8192
 
/* RIFF 块 */
typedef struct __packed
{
    u32 ChunkID;
    u32 ChunkSize;
    u32 Format;
} ChunkRIFF;

/* fmt 块 */
typedef struct __packed
{
    u32 ChunkID;
    u32 ChunkSize;
    u16 AudioFormat;
    u16 NumOfChannels;
    u32 SampleRate;
    u32 ByteRate;
    u16 BlockAlign;
    u16 BitsPerSample;
} ChunkFMT;

/* fact 块 */
typedef struct __packed
{
    u32 ChunkID;
    u32 ChunkSize;
    u32 NumOfSamples;
} ChunkFACT;

/* LIST 块 */
typedef struct __packed
{
    u32 ChunkID;
    u32 ChunkSize;
} ChunkLIST;

/* data 块 */
typedef struct __packed
{
    u32 ChunkID;
    u32 ChunkSize;
} ChunkDATA;

/* wav 文件头 */
typedef struct __packed
{
    ChunkRIFF riff;
    ChunkFMT  fmt;
    /* ChunkFACT fact; // 可选 */
    ChunkDATA data;
} __WaveHeader;

/* wav 播放控制信息（注意保持原名 __wavctrl） */
typedef struct __packed
{
    u16 audioformat;
    u16 nchannels;
    u16 blockalign;
    u32 datasize;

    u32 totsec;
    u32 cursec;

    u32 bitrate;
    u32 samplerate;
    u16 bps;

    u32 datastart;
} __wavctrl;


u8 wav_decode_init(u8* fname,__wavctrl* wavx);
u32 wav_buffill(u8 *buf,u16 size,u8 bits);
void wav_i2s_dma_tx_callback(void); 
u8 wav_play_song(u8* fname);
void wav_play_poll(void);
#endif
















