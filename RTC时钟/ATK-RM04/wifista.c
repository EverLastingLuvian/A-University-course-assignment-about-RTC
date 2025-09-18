#include "common.h"
#include "RTC.h"
#include "Menu.h"

/////////////////////////////////////////////////////////////////////////////////////////////////////////// 
// 本实验主要学习使用ATK-RM04 WIFI模块的STA模式实现数据传输
// 硬件平台：ALIENTEK STM32F407开发板  
// 功能描述：ATK-RM04 WIFI模块测试程序，实现COM转WIFI STA功能	   
// 设计作者：ALIENTEK
// 论坛地址：www.openedv.com
// 创建日期：2014/10/28
// 软件版本：V1.0
// 版权声明：本软件仅供学习使用，未经许可不得用于商业用途
// Copyright(C) 广州市星翼电子科技有限公司 2014-2024
// All rights reserved									  
/////////////////////////////////////////////////////////////////////////////////////////////////////////// 

// ATK-RM04 WIFI STA模式测试
// 实现TCP/UDP数据传输
// 返回值：0，失败
//         1，成功，模块进入透传模式
u8 atk_rm04_wifista_test(void)
{
	u8 netpro=0;	//网络模式
	u8 key;
	u8 timex=0; 
	u8 ipbuf[16]; 	//IP地址缓冲区
	u8 *p; 
	u16 t=999;		//用于定时检查连接状态
	u8 res=0;
	u16 rlen=0;
	u8 constate=0;	//连接状态
	
	extern KeyState key_state;
	
	p=mymalloc(SRAMIN,100);							//申请内存空间
	atk_rm04_send_cmd("at+netmode=2","ok",500);		//设置为WIFI STA(工作站)模式
//	atk_rm04_send_cmd("at+dhcpd=0","ok",500);		//关闭DHCP服务器(仅在AP模式有效) 
	atk_rm04_send_cmd("at+dhcpc=1","ok",500);		//启用DHCP客户端(net_ip参数无效)
	//设置连接参数：WIFI网络名称/加密方式/密码，这些参数必须提前设置好!!
	sprintf((char*)p,"at+wifi_conf=%s,%s,%s",wifista_ssid,wifista_encryption,wifista_password);//设置连接参数:ssid,加密方式,密码
	atk_rm04_send_cmd(p,"ok",500);					//发送连接参数
//PRESTA:
//	netpro=atk_rm04_netpro_sel(50,30,(u8*)ATK_RM04_NETMODE_TBL[2]);	//选择网络模式
	netpro = 1;
//	if(netpro&0X02)atk_rm04_send_cmd("at+remotepro=udp","ok",500);	//UDP协议
//	else 
	atk_rm04_send_cmd("at+remotepro=tcp","ok",500);			//TCP协议
	sprintf((char*)p, "at+remoteip=%s", "192.168.137.1");//目标IP地址 
		
	atk_rm04_send_cmd(p,"ok",500);					//设置远程(连接)IP地址
	
	sprintf((char*)p,"at+remoteport=%s",(uint8_t*)"8086");
	atk_rm04_send_cmd(p,"ok",500);					//设置端口号
	if(netpro&0X01)	//客户端模式
	{
//		if(atk_rm04_ip_set("WIFI-STA 远程IP设置",(u8*)ATK_RM04_WORKMODE_TBL[netpro],(u8*)portnum,ipbuf))goto PRESTA;	
		
		atk_rm04_send_cmd("at+mode=client","ok",500);	//设置为客户端		
		}
//	else atk_rm04_send_cmd("at+mode=server","ok",500);	//设置为服务器				 
	LCD_Clear(WHITE);
	POINT_COLOR=RED;
	LCD_ShowString(0,30,240,16,16,(u8*)"ATK-RM04 WIFI-STA TEST");
	LCD_ShowString(30,50,200,16,12,(u8*)"Initializing ATK-RM04 module...");  	
	if(atk_rm04_send_cmd("at+net_commit=1","\r\n",4000))//提交网络设置，等待模块连接约40s
 	{ 
		LCD_Fill(30,50,239,50+12,WHITE);	//清除之前显示
		LCD_ShowString(30,50,200,12,12,(u8*)"Configuration successful!"); 
		delay_ms(800);        
		res=1; 
	}else
	{	
		atk_rm04_send_cmd("at+reconn=1","ok",500);	//开启自动重连功能
		LCD_Fill(30,50,239,50+12,WHITE);			//清除之前显示
		LCD_ShowString(30,50,200,12,12,(u8*)"Configuration failed!");
		delay_ms(600);
		LCD_ShowString(30,50,210,16,12,(u8*)"KEY_UP: Continue  KEY0: Exit");
		atk_rm04_quit_trans();						//退出透传模式
		while(atk_rm04_get_wifista_state()==0)		//等待ATK-RM04连接成功 
		{ 	
			LCD_ShowString(30,80,200,12,12,(u8*)"ATK-RM04 connecting...");
			delay_ms(800);     
			LCD_ShowString(30,80,200,12,12,(u8*)"ATK-RM04 connecting...."); 
			delay_ms(800); 
		}
		LCD_Fill(30,80,239,80+12,WHITE);
		if((netpro&0X01)==0)atk_rm04_get_wanip(ipbuf);//服务器模式，获取WAN IP
 		sprintf((char*)p,"IP:%s Port:%s",ipbuf,(u8*)portnum);
		LCD_ShowString(30,65,200,12,12,p);				//显示IP和端口	
		LCD_ShowString(30,80,200,12,12,(u8*)"State:"); 
		LCD_ShowString(120,80,200,12,12,(u8*)"Mode:"); 
		LCD_ShowString(30,100,200,12,12,(u8*)"TX:"); 
		LCD_ShowString(30,115,200,12,12,(u8*)"RX:");
		atk_rm04_wificonf_show(30,180,(u8*)"SSID:",(u8*)wifista_ssid,(u8*)wifista_encryption,(u8*)wifista_password);
		POINT_COLOR=BLUE;
		LCD_ShowString(120+30,80,200,12,12,(u8*)ATK_RM04_WORKMODE_TBL[netpro]); 		//连接状态
		USART3_RX_STA=0;
		
		atk_rm04_send_cmd("at+timeout=0","ok",500);
		atk_rm04_send_cmd("at+reconn=1","ok",500);
		
	while(1)
	{
			if(key_state == KEY_UP) break;
		
			t++;
			delay_ms(10);
			if(USART3_RX_STA & 0X8000)
			{ 
					rlen = USART3_RX_STA & 0X7FFF;
					USART3_RX_BUF[rlen] = 0;       
					printf("%s", USART3_RX_BUF);    
					
					sprintf((char*)p, "Received %d bytes", rlen);
					LCD_Fill(30+54, 115, 239, 130, WHITE);
					POINT_COLOR = RED;
					Show_Str(30+54, 115, 156, 12, p, 12, 0);
					POINT_COLOR = BLUE;
					
					
					LCD_Fill(30, 130, 239, 319, WHITE);
					Show_Str(30, 130, 180, 190, USART3_RX_BUF, 12, 0);
					
					if(rlen >= 19 && 
						 USART3_RX_BUF[4] == '-' && 
						 USART3_RX_BUF[7] == '-' && 
						 USART3_RX_BUF[10] == 'T' && 
						 USART3_RX_BUF[13] == ':' && 
						 USART3_RX_BUF[16] == ':')
					{
							int year, month, day, hour, min, sec;
							if(sscanf((char*)USART3_RX_BUF, "%d-%d-%dT%d:%d:%d", 
												&year, &month, &day, &hour, &min, &sec) == 6)
							{
								
									RTC_Set_Time(hour, min, sec);
									RTC_Set_Date(year % 100, month, day);
									
									
									LCD_Fill(30, 150, 239, 165, WHITE);
									LCD_ShowString(30, 150, 200, 12, 12, (u8*)"Time Updated!");
							}
					}
					
					USART3_RX_STA = 0;
					if(constate == 0) t = 1000; 
					else t = 0;                
			}  
			
			if(t == 1000)  
			{
					constate = atk_rm04_consta_check() - '0'; 
					if(constate) 
							LCD_ShowString(30+30, 80, 200, 12, 12, (u8*)"Connected");  
					else 
							LCD_ShowString(30+30, 80, 200, 12, 12, (u8*)"Disconnected");  
					t = 0;
			}
			
			atk_rm04_at_response(1);
	}
		myfree(SRAMIN,p);		//释放内存 
		atk_rm04_quit_trans();	//退出透传模式
		return res;
	}
}









