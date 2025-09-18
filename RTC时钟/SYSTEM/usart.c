#include "usart.h"
#include "delay.h"
#include "timer.h"
#include "usart3.h"

extern UART_HandleTypeDef UART1_Handler;
extern UART_HandleTypeDef huart3;

#if EN_USART1_RX
extern u8 USART_RX_BUF[USART_REC_LEN];
extern u16 USART_RX_STA;
extern u8 aRxBuffer[RXBUFFERSIZE];
#endif

extern uint8_t USART3_RX_BUF[USART3_MAX_RECV_LEN];
extern uint16_t USART3_RX_STA;
extern TIM_HandleTypeDef htim7;

////////////////////////////////////////////////////////////////////////////////// 	 
//Èç¹ûÊ¹ÓÃos,Ôò°üÀ¨ÏÂÃæµÄÍ·ÎÄ¼ş¼´¿É.
#if SYSTEM_SUPPORT_OS
#include "includes.h"					//os Ê¹ÓÃ	  
#endif
//////////////////////////////////////////////////////////////////////////////////	 
//±¾³ÌĞòÖ»¹©Ñ§Ï°Ê¹ÓÃ£¬Î´¾­×÷ÕßĞí¿É£¬²»µÃÓÃÓÚÆäËüÈÎºÎÓÃÍ¾
//ALIENTEK STM32F407¿ª·¢°å
//´®¿Ú1³õÊ¼»¯		   
//ÕıµãÔ­×Ó@ALIENTEK
//¼¼ÊõÂÛÌ³:www.openedv.com
//ĞŞ¸ÄÈÕÆÚ:2017/4/6
//°æ±¾£ºV1.5
//°æÈ¨ËùÓĞ£¬µÁ°æ±Ø¾¿¡£
//Copyright(C) ¹ãÖİÊĞĞÇÒíµç×Ó¿Æ¼¼ÓĞÏŞ¹«Ë¾ 2009-2019
//All rights reserved
//********************************************************************************
//V1.0ĞŞ¸ÄËµÃ÷ 
////////////////////////////////////////////////////////////////////////////////// 	  
//¼ÓÈëÒÔÏÂ´úÂë,Ö§³Öprintfº¯Êı,¶ø²»ĞèÒªÑ¡Ôñuse MicroLIB	  
//#define PUTCHAR_PROTOTYPE int fputc(int ch, FILE *f)	
#if 1
#pragma import(__use_no_semihosting)             
//±ê×¼¿âĞèÒªµÄÖ§³Öº¯Êı                 
struct __FILE 
{ 
	int handle; 
}; 

FILE __stdout;       
//¶¨Òå_sys_exit()ÒÔ±ÜÃâÊ¹ÓÃ°ëÖ÷»úÄ£Ê½    
void _sys_exit(int x) 
{ 
	x = x; 
} 
//ÖØ¶¨Òåfputcº¯Êı 
int fputc(int ch, FILE *f)
{ 	
	while((USART1->SR&0X40)==0);//Ñ­»··¢ËÍ,Ö±µ½·¢ËÍÍê±Ï   
	USART1->DR = (u8) ch;      
	return ch;
}
#endif 

#if EN_USART1_RX   //Èç¹ûÊ¹ÄÜÁË½ÓÊÕ
//´®¿Ú1ÖĞ¶Ï·şÎñ³ÌĞò
//×¢Òâ,¶ÁÈ¡USARTx->SRÄÜ±ÜÃâÄªÃûÆäÃîµÄ´íÎó   	
u8 USART_RX_BUF[USART_REC_LEN];     //½ÓÊÕ»º³å,×î´óUSART_REC_LEN¸ö×Ö½Ú.
//½ÓÊÕ×´Ì¬
//bit15£¬	½ÓÊÕÍê³É±êÖ¾
//bit14£¬	½ÓÊÕµ½0x0d
//bit13~0£¬	½ÓÊÕµ½µÄÓĞĞ§×Ö½ÚÊıÄ¿
u16 USART_RX_STA=0;       //½ÓÊÕ×´Ì¬±ê¼Ç	

u8 aRxBuffer[RXBUFFERSIZE];//HAL¿âÊ¹ÓÃµÄ´®¿Ú½ÓÊÕ»º³å
UART_HandleTypeDef UART1_Handler; //UART¾ä±ú

//³õÊ¼»¯IO ´®¿Ú1 
//bound:²¨ÌØÂÊ
void uart_init(u32 bound)
{	
	//UART ³õÊ¼»¯ÉèÖÃ
	UART1_Handler.Instance=USART1;					    //USART1
	UART1_Handler.Init.BaudRate=bound;				    //²¨ÌØÂÊ
	UART1_Handler.Init.WordLength=UART_WORDLENGTH_8B;   //×Ö³¤Îª8Î»Êı¾İ¸ñÊ½
	UART1_Handler.Init.StopBits=UART_STOPBITS_1;	    //Ò»¸öÍ£Ö¹Î»
	UART1_Handler.Init.Parity=UART_PARITY_NONE;		    //ÎŞÆæÅ¼Ğ£ÑéÎ»
	UART1_Handler.Init.HwFlowCtl=UART_HWCONTROL_NONE;   //ÎŞÓ²¼şÁ÷¿Ø
	UART1_Handler.Init.Mode=UART_MODE_TX_RX;		    //ÊÕ·¢Ä£Ê½
	HAL_UART_Init(&UART1_Handler);					    //HAL_UART_Init()»áÊ¹ÄÜUART1
	
	HAL_UART_Receive_IT(&UART1_Handler, (u8 *)aRxBuffer, RXBUFFERSIZE);//¸Ãº¯Êı»á¿ªÆô½ÓÊÕÖĞ¶Ï£º±êÖ¾Î»UART_IT_RXNE£¬²¢ÇÒÉèÖÃ½ÓÊÕ»º³åÒÔ¼°½ÓÊÕ»º³å½ÓÊÕ×î´óÊı¾İÁ¿
  
}

//UARTµ×²ã³õÊ¼»¯£¬Ê±ÖÓÊ¹ÄÜ£¬Òı½ÅÅäÖÃ£¬ÖĞ¶ÏÅäÖÃ
//´Ëº¯Êı»á±»HAL_UART_Init()µ÷ÓÃ
//huart:´®¿Ú¾ä±ú

void HAL_UART_MspInit(UART_HandleTypeDef *huart)
{
    //GPIO¶Ë¿ÚÉèÖÃ
	GPIO_InitTypeDef GPIO_Initure;
	
	if(huart->Instance==USART1)//Èç¹ûÊÇ´®¿Ú1£¬½øĞĞ´®¿Ú1 MSP³õÊ¼»¯
	{
		__HAL_RCC_GPIOA_CLK_ENABLE();			//Ê¹ÄÜGPIOAÊ±ÖÓ
		__HAL_RCC_USART1_CLK_ENABLE();			//Ê¹ÄÜUSART1Ê±ÖÓ
	
		GPIO_Initure.Pin=GPIO_PIN_9;			//PA9
		GPIO_Initure.Mode=GPIO_MODE_AF_PP;		//¸´ÓÃÍÆÍìÊä³ö
		GPIO_Initure.Pull=GPIO_PULLUP;			//ÉÏÀ­
		GPIO_Initure.Speed=GPIO_SPEED_FAST;		//¸ßËÙ
		GPIO_Initure.Alternate=GPIO_AF7_USART1;	//¸´ÓÃÎªUSART1
		HAL_GPIO_Init(GPIOA,&GPIO_Initure);	   	//³õÊ¼»¯PA9

		GPIO_Initure.Pin=GPIO_PIN_10;			//PA10
		HAL_GPIO_Init(GPIOA,&GPIO_Initure);	   	//³õÊ¼»¯PA10
		
#if EN_USART1_RX
		HAL_NVIC_EnableIRQ(USART1_IRQn);				//Ê¹ÄÜUSART1ÖĞ¶ÏÍ¨µÀ
		HAL_NVIC_SetPriority(USART1_IRQn,3,3);			//ÇÀÕ¼ÓÅÏÈ¼¶3£¬×ÓÓÅÏÈ¼¶3
#endif	
	}

}

 /**
 * @brief ç»Ÿä¸€UARTæ¥æ”¶å›è°ƒ
 * @param huart: å½“å‰è§¦å‘ä¸­æ–­çš„ä¸²å£å¥æŸ„
 */
void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart)
{
    if(huart->Instance == USART1)   // å¤„ç† USART1
    {
#if EN_USART1_RX
        if((USART_RX_STA & 0x8000) == 0)  // æ¥æ”¶æœªå®Œæˆ
        {
            if(USART_RX_STA & 0x4000)  // ä¸Šä¸€æ¬¡æ”¶åˆ° 0x0D
            {
                if(aRxBuffer[0] != 0x0A)
                    USART_RX_STA = 0;   // é”™è¯¯ï¼Œé‡ç½®
                else
                    USART_RX_STA |= 0x8000; // æ ‡è®°æ¥æ”¶å®Œæˆ
            }
            else
            {
                if(aRxBuffer[0] == 0x0D)
                    USART_RX_STA |= 0x4000;
                else
                {
                    USART_RX_BUF[USART_RX_STA & 0x3FFF] = aRxBuffer[0];
                    USART_RX_STA++;
                    if(USART_RX_STA > (USART_REC_LEN - 1))
                        USART_RX_STA = 0;  // è¶…é•¿ï¼Œé‡ç½®
                }
            }
        }
#endif
    }
}
 
//´®¿Ú1ÖĞ¶Ï·şÎñ³ÌĞò
void USART1_IRQHandler(void)                	
{ 
	u32 timeout=0;
#if SYSTEM_SUPPORT_OS	 	//Ê¹ÓÃOS
	OSIntEnter();    
#endif
	
	HAL_UART_IRQHandler(&UART1_Handler);	//µ÷ÓÃHAL¿âÖĞ¶Ï´¦Àí¹«ÓÃº¯Êı
	
	timeout=0;
    while (HAL_UART_GetState(&UART1_Handler) != HAL_UART_STATE_READY)//µÈ´ı¾ÍĞ÷
	{
	 timeout++;////³¬Ê±´¦Àí
     if(timeout>HAL_MAX_DELAY) break;		
	
	}
     
	timeout=0;
	while(HAL_UART_Receive_IT(&UART1_Handler, (u8 *)aRxBuffer, RXBUFFERSIZE) != HAL_OK)//Ò»´Î´¦ÀíÍê³ÉÖ®ºó£¬ÖØĞÂ¿ªÆôÖĞ¶Ï²¢ÉèÖÃRxXferCountÎª1
	{
	 timeout++; //³¬Ê±´¦Àí
	 if(timeout>HAL_MAX_DELAY) break;	
	}
#if SYSTEM_SUPPORT_OS	 	//Ê¹ÓÃOS
	OSIntExit();  											 
#endif
} 
#endif	


/*ÏÂÃæ´úÂëÎÒÃÇÖ±½Ó°ÑÖĞ¶Ï¿ØÖÆÂß¼­Ğ´ÔÚÖĞ¶Ï·şÎñº¯ÊıÄÚ²¿¡£
 ËµÃ÷£º²ÉÓÃHAL¿â´¦ÀíÂß¼­£¬Ğ§ÂÊ²»¸ß¡£*/
/*

//´®¿Ú1ÖĞ¶Ï·şÎñ³ÌĞò
void USART1_IRQHandler(void)                	
{ 
	u8 Res;
#if SYSTEM_SUPPORT_OS	 	//Ê¹ÓÃOS
	OSIntEnter();    
#endif
	if((__HAL_UART_GET_FLAG(&UART1_Handler,UART_FLAG_RXNE)!=RESET))  //½ÓÊÕÖĞ¶Ï(½ÓÊÕµ½µÄÊı¾İ±ØĞëÊÇ0x0d 0x0a½áÎ²)
	{
        HAL_UART_Receive(&UART1_Handler,&Res,1,1000); 
		if((USART_RX_STA&0x8000)==0)//½ÓÊÕÎ´Íê³É
		{
			if(USART_RX_STA&0x4000)//½ÓÊÕµ½ÁË0x0d
			{
				if(Res!=0x0a)USART_RX_STA=0;//½ÓÊÕ´íÎó,ÖØĞÂ¿ªÊ¼
				else USART_RX_STA|=0x8000;	//½ÓÊÕÍê³ÉÁË 
			}
			else //»¹Ã»ÊÕµ½0X0D
			{	
				if(Res==0x0d)USART_RX_STA|=0x4000;
				else
				{
					USART_RX_BUF[USART_RX_STA&0X3FFF]=Res ;
					USART_RX_STA++;
					if(USART_RX_STA>(USART_REC_LEN-1))USART_RX_STA=0;//½ÓÊÕÊı¾İ´íÎó,ÖØĞÂ¿ªÊ¼½ÓÊÕ	  
				}		 
			}
		}   		 
	}
	HAL_UART_IRQHandler(&UART1_Handler);	
#if SYSTEM_SUPPORT_OS	 	//Ê¹ÓÃOS
	OSIntExit();  											 
#endif
} 
#endif	
*/
 
 





