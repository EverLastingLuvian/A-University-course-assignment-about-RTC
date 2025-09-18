#include "audioplay.h"
#include "ff.h"
#include "malloc.h"
#include "usart.h"
#include "wm8978.h"
#include "i2s.h"
#include "delay.h"
#include "exfuns.h"  
#include "string.h"  

__audiodev audiodev;	  


void audio_start(void)
{
	audiodev.status = 3 << 0;
	I2S_Play_Start();
} 


void audio_stop(void)
{
	audiodev.status = 0;
	I2S_Play_Stop();
}  


u16 audio_get_tnum(u8 *path)
{	  
	u8 res;
	u16 rval=0;
 	DIR tdir;	 		
	FILINFO* tfileinfo; 	
	tfileinfo=(FILINFO*)mymalloc(SRAMIN,sizeof(FILINFO));
    res=f_opendir(&tdir,(const TCHAR*)path); 
	if(res==FR_OK&&tfileinfo)
	{
		while(1)
		{
	        res=f_readdir(&tdir,tfileinfo);       
	        if(res!=FR_OK||tfileinfo->fname[0]==0)break;		 
			res=f_typetell((u8*)tfileinfo->fname);	
			if((res&0XF0)==0X40) 
			{
				rval++;
			}	    
		}  
	}  
	myfree(SRAMIN,tfileinfo);
	return rval;
}


void audio_play(void)
{
	u8 res;
 	DIR wavdir;	 			
	FILINFO *wavfileinfo;	
	u8 *pname;				
	u16 totwavnum; 			
	u16 curindex;			
 	u32 temp;
	u32 *wavoffsettbl;		
	
	WM8978_ADDA_Cfg(1,0);	
	WM8978_Input_Cfg(0,0,0);
	WM8978_Output_Cfg(1,1);	
	
 	while(f_opendir(&wavdir,"0:/VOICE")) 
 	{	    
		delay_ms(200);				  
	} 									  
	totwavnum=audio_get_tnum("0:/VOICE"); 
  	while(totwavnum==0) 
 	{	    
		delay_ms(200);				  
	}										   
	wavfileinfo=(FILINFO*)mymalloc(SRAMIN,sizeof(FILINFO));
  	pname=mymalloc(SRAMIN,_MAX_LFN*2+1);					
 	wavoffsettbl=mymalloc(SRAMIN,4*totwavnum);				
 	while(!wavfileinfo||!pname||!wavoffsettbl) 
 	{	    
		delay_ms(200);				  
	}  	 

    res=f_opendir(&wavdir,"0:/VOICE"); 
	if(res==FR_OK)
	{
		curindex=0;
		while(1)
		{
			temp=wavdir.dptr;								
	        res=f_readdir(&wavdir,wavfileinfo);       		
	        if(res!=FR_OK||wavfileinfo->fname[0]==0)break;	 
			res=f_typetell((u8*)wavfileinfo->fname);	
			if((res&0XF0)==0X40)	
			{
				wavoffsettbl[curindex]=temp;
				curindex++;
			}    
		} 
	}   
   	curindex=0;										
   	res=f_opendir(&wavdir,(const TCHAR*)"0:/VOICE"); 	
	while(res==FR_OK) 
	{	
		dir_sdi(&wavdir,wavoffsettbl[curindex]);			   
        res=f_readdir(&wavdir,wavfileinfo);       				
        if(res!=FR_OK||wavfileinfo->fname[0]==0)break;		 
		strcpy((char*)pname,"0:/VOICE/");						
		strcat((char*)pname,(const char*)wavfileinfo->fname);	
		

		audio_play_song(pname); 			 		

		curindex++;		   	
		if(curindex>=totwavnum)curindex=0; 
	} 											  
	myfree(SRAMIN,wavfileinfo);			    
	myfree(SRAMIN,pname);			    
	myfree(SRAMIN,wavoffsettbl);	 
} 


u8 audio_play_song(u8* fname)
{

	WM8978_ADDA_Cfg(1,0);	
	WM8978_Input_Cfg(0,0,0);
	WM8978_Output_Cfg(1,1);	
	WM8978_HPvol_Set(40,40);   
	WM8978_SPKvol_Set(30);
	
	u8 res;  
	res=f_typetell(fname); 
	switch(res)
	{
		case T_WAV:
			res=wav_play_song(fname);
			break;
		default: 
			printf("can't play:%s\r\n",fname);
			res=0;
			break;
	}
	return res;
}




























