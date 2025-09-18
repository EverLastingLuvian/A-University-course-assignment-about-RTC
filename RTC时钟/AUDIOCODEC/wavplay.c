#include "wavplay.h" 
#include "audioplay.h"
#include "usart.h" 
#include "delay.h" 
#include "malloc.h"
#include "ff.h"
#include "i2s.h"
#include "wm8978.h"


__wavctrl wavctrl;		
vu8 wavtransferend=0;
vu8 wavwitchbuf=0;		

int wav_active = 0;

u8 wav_decode_init(u8* fname,__wavctrl* wavx)
{
    FIL *ftemp;
    u8 *buf; 
    u32 br=0;
    u8 res=0;
    
    ChunkRIFF *riff;
    ChunkFMT *fmt;
    ChunkFACT *fact;
    ChunkDATA *data;

    ftemp=(FIL*)mymalloc(SRAMIN,sizeof(FIL));
    buf=mymalloc(SRAMIN,512);

    if(ftemp && buf)
    {
        res=f_open(ftemp,(TCHAR*)fname,FA_READ);
        if(res==FR_OK)
        {
            f_read(ftemp,buf,512,&br);	
            riff=(ChunkRIFF *)buf;
            if(riff->Format==0X45564157) 
            {
                fmt=(ChunkFMT *)(buf+12);
                fact=(ChunkFACT *)(buf+12+8+fmt->ChunkSize);
                if(fact->ChunkID==0X74636166||fact->ChunkID==0X5453494C)
                    wavx->datastart=12+8+fmt->ChunkSize+8+fact->ChunkSize;
                else 
                    wavx->datastart=12+8+fmt->ChunkSize;

                data=(ChunkDATA *)(buf+wavx->datastart);
                if(data->ChunkID==0X61746164)
                {
                    wavx->audioformat=fmt->AudioFormat;
                    wavx->nchannels=fmt->NumOfChannels;
                    wavx->samplerate=fmt->SampleRate;
                    wavx->bitrate=fmt->ByteRate*8;
                    wavx->blockalign=fmt->BlockAlign;
                    wavx->bps=fmt->BitsPerSample;
                    
                    wavx->datasize=data->ChunkSize;
                    wavx->datastart=wavx->datastart+8; 

                    printf("WAV Info: %dHz, %dbit, %dch, size=%d\r\n",
                        wavx->samplerate, wavx->bps, wavx->nchannels, wavx->datasize);
                }
                else res=3;
            }else res=2;
        }else res=1;
    }
    f_close(ftemp);
    myfree(SRAMIN,ftemp);
    myfree(SRAMIN,buf); 
    return res;
}



void wav_i2s_dma_tx_callback(void) 
{   
    if(DMA1_Stream4->CR & (1<<19)) 
        wavwitchbuf=0;
    else 
        wavwitchbuf=1;

    wavtransferend=1;
} 



u32 wav_buffill(u8 *buf,u16 size,u8 bits)
{
    u16 readlen=0;
    u32 bread;
    u16 i;
    u8 *p;

    if(bits==24)
    {
        readlen=(size/4)*3;
        f_read(audiodev.file,audiodev.tbuf,readlen,(UINT*)&bread);
        p=audiodev.tbuf;
        for(i=0;i<size;)
        {
            buf[i++]=p[1];
            buf[i]=p[2]; 
            i+=2;
            buf[i++]=p[0];
            p+=3;
        } 
        bread=(bread*4)/3;
    }else 
    {
        f_read(audiodev.file,buf,size,(UINT*)&bread);
        if(bread<size)
        {
            for(i=bread;i<size;i++) buf[i]=0; 
        }
    }
    return bread;
}  



u8 wav_play_song(u8* fname)
{
    u8 res;  

    audiodev.file=(FIL*)mymalloc(SRAMIN,sizeof(FIL));
    audiodev.i2sbuf1=mymalloc(SRAMIN,WAV_I2S_TX_DMA_BUFSIZE);
    audiodev.i2sbuf2=mymalloc(SRAMIN,WAV_I2S_TX_DMA_BUFSIZE);
    audiodev.tbuf=mymalloc(SRAMIN,WAV_I2S_TX_DMA_BUFSIZE);

    if(audiodev.file&&audiodev.i2sbuf1&&audiodev.i2sbuf2&&audiodev.tbuf)
    { 
        res=wav_decode_init(fname,&wavctrl);
        if(res==0)
        {
            // °t¸m I2S
            if(wavctrl.bps==16)
            {
                WM8978_I2S_Cfg(2,0);
                I2S2_Init(I2S_STANDARD_PHILIPS,I2S_MODE_MASTER_TX,
                          I2S_CPOL_LOW,I2S_DATAFORMAT_16B_EXTENDED);
            }else if(wavctrl.bps==24)
            {
                WM8978_I2S_Cfg(2,2);
                I2S2_Init(I2S_STANDARD_PHILIPS,I2S_MODE_MASTER_TX,
                          I2S_CPOL_LOW,I2S_DATAFORMAT_24B);
            }
            I2S2_SampleRate_Set(wavctrl.samplerate / 2); 
            I2S2_TX_DMA_Init(audiodev.i2sbuf1,audiodev.i2sbuf2,
                             WAV_I2S_TX_DMA_BUFSIZE/2);
            i2s_tx_callback=wav_i2s_dma_tx_callback;

            audio_stop();
            res=f_open(audiodev.file,(TCHAR*)fname,FA_READ);
            if(res==0)
            {
                f_lseek(audiodev.file, wavctrl.datastart);
                wav_buffill(audiodev.i2sbuf1,WAV_I2S_TX_DMA_BUFSIZE,wavctrl.bps);
                wav_buffill(audiodev.i2sbuf2,WAV_I2S_TX_DMA_BUFSIZE,wavctrl.bps);
								
								wav_active = 1;
                audio_start();  
            }
        }
    } 
    return res;
}

void wav_play_poll(void)
{
    u32 fillnum;

    if(!wav_active) return;

    if(wavtransferend) 
    {
        wavtransferend = 0;

        if(wavwitchbuf)
						fillnum=wav_buffill(audiodev.i2sbuf2,WAV_I2S_TX_DMA_BUFSIZE,wavctrl.bps);
				else 
						fillnum=wav_buffill(audiodev.i2sbuf1,WAV_I2S_TX_DMA_BUFSIZE,wavctrl.bps);

        if(fillnum==0)
        {
						wav_active = 0;
						audio_stop();
						myfree(SRAMIN,audiodev.tbuf);
						myfree(SRAMIN,audiodev.i2sbuf1);
						myfree(SRAMIN,audiodev.i2sbuf2);
						myfree(SRAMIN,audiodev.file);
        }
    }
}










