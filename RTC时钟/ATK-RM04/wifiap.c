#include "common.h"
#include "Menu.h"
#include "RTC.h"

/////////////////////////////////////////////////////////////////////////////////////////////////////////// 
// 本文件用于ALIENTEK STM32F407开发板
// ATK-RM04 WiFi模块 COM-WIFI AP模式 测试
// 作者：ALIENTEK
// 官网：www.openedv.com
// 创建日期：2014/10/28
// 版本：V1.0
// 说明：用于测试WiFi AP模式下的TCP/UDP通信
// Copyright(C) ALIENTEK 2014-2024
// All rights reserved									  
/////////////////////////////////////////////////////////////////////////////////////////////////////////// 

// ATK-RM04 WIFI AP测试函数
// 功能：测试模块在AP模式下的TCP/UDP通信
// 返回值：0 表示测试失败，1 表示测试成功

u8 atk_rm04_wifiap_test(void)
{
			u8 netpro=0;	// 网络协议选择
			u8 ipbuf[16]; 	// IP缓存
			u8 *p; 
			u16 t=999;		// 定时计数
			u8 res=0;
			u16 rlen=0;
			u8 constate=0;	// 连接状态
			u8 timex=0;
			
			extern KeyState key_state;
	
			p = mymalloc(SRAMIN, 32); // 分配32字节内存
			// 配置模块为WIFI AP模式
			atk_rm04_send_cmd("at+netmode=3","ok",500);		
			atk_rm04_send_cmd("at+dhcpd=1","ok",500);	// 启用DHCP服务器
			atk_rm04_send_cmd("at+dhcpc=0","ok",500);	// 关闭DHCP客户端

			// 设置DHCP分配的IP范围和网关
			// 分配IP范围: 192.168.16.100~192.168.16.200, 网关: 192.168.16.1
			atk_rm04_send_cmd("at+dhcpd_ip=192.168.16.100,192.168.16.200,255.255.255.0,192.168.16.1","ok",500);
			atk_rm04_send_cmd("at+dhcpd_dns=192.168.16.1,0.0.0.0","ok",500);// 设置DHCP DNS

			// 设置AP模式下的模块IP和DNS
			atk_rm04_send_cmd("at+net_ip=192.168.16.254,255.255.255.0,192.168.16.1","ok",500);
			atk_rm04_send_cmd("at+net_dns=192.168.16.1,0.0.0.0","ok",500);

			// 配置WiFi AP的SSID、加密方式和密码
			sprintf((char*)p,"at+wifi_conf=%s,%s,%s",wifiap_ssid,wifiap_encryption,wifiap_password);
			atk_rm04_send_cmd(p,"ok",500);	// 设置AP参数
		//PRESTA:
		//	netpro = atk_rm04_netpro_sel(50,30,(u8*)ATK_RM04_NETMODE_TBL[3]); // 选择网络协议
			netpro = 0;
			if(netpro & 0x02)
				atk_rm04_send_cmd("at+remotepro=udp","ok",500);	// 使用UDP协议
			else 
				atk_rm04_send_cmd("at+remotepro=tcp","ok",500);	// 使用TCP协议

			sprintf((char*)p,"at+remoteport=%s",portnum);
			atk_rm04_send_cmd(p,"ok",500);	// 设置远程端口

		//	if(netpro & 0x01)	// 如果是客户端模式
		//	{
		//		//if(atk_rm04_ip_set("WIFI-AP 客户端IP设置",(u8*)ATK_RM04_WORKMODE_TBL[netpro],(u8*)portnum,ipbuf))
		//			goto PRESTA; // 如果IP设置失败，重新设置
		//		sprintf((char*)p,"at+remoteip=%s",ipbuf);
		//		atk_rm04_send_cmd(p,"ok",500);			// 设置远程服务器IP
		//		atk_rm04_send_cmd("at+mode=client","ok",500);	// 设置模块为客户端
		//	}
		//	else 
			atk_rm04_send_cmd("at+mode=server","ok",500);	// 设置模块为服务器

			LCD_Clear(WHITE);
			POINT_COLOR = RED;
			LCD_ShowString(0, 30, 240, 16, 16, "ATK-RM04 WIFI-AP Test");
			LCD_ShowString(30, 50, 200, 16, 12, "Init ATK-RM04 module, wait...");	

			if(atk_rm04_send_cmd("at+net_commit=1","\r\n",4000)) // 提交配置，等待最多4秒
			{ 
				LCD_Fill(30, 50, 239, 62, WHITE);
				LCD_ShowString(30, 50, 200, 16, 12, "Module init OK!"); 
				delay_ms(800);        
				res = 1; 
			}
			else
			{	
				atk_rm04_send_cmd("at+reconn=1","ok",500);	// 重新连接模块
				LCD_Fill(30, 50, 239, 62, WHITE);
				LCD_ShowString(30, 50, 200, 16, 12, "Module init failed!");
				LCD_ShowString(30, 80, 210, 16, 12, "KEY_UP:Exit  KEY0:Send");
				// 服务器模式获取 IP
				
				u8 ipbuf[16];
				sprintf((char*)ipbuf, "192.168.16.254"); // 固定 IP

				sprintf((char*)p, "IP:%s  Port:%s", ipbuf, (u8*)portnum);
				LCD_ShowString(30, 65, 200, 12, 12, p);	
				LCD_ShowString(30, 80, 200, 12, 12, "State:");
				LCD_ShowString(120, 80, 200, 12, 12, "Module:");
				LCD_ShowString(30, 100, 200, 12, 12, "TxCnt:");
				LCD_ShowString(30, 115, 200, 12, 12, "RxCnt:");
				
				atk_rm04_wificonf_show(30,180,"模块参数:",(u8*)wifiap_ssid,(u8*)wifiap_encryption,(u8*)wifiap_password);
				POINT_COLOR = BLUE;
				LCD_ShowString(150, 80, 200, 12, 12, (uint8_t*)ATK_RM04_WORKMODE_TBL[netpro]); // 显示模块当前工作模式
				Show_Str(120+30, 80, 200, 12, (u8*)ATK_RM04_WORKMODE_TBL[netpro], 12, 0);      // 模式说明文字

				USART3_RX_STA = 0;  // 清空接收完成标志
			}
		while(1)
		{
			
			if(key_state == KEY_UP) break;
			
			t++;                // 10 ms计数器
			delay_ms(10);

			if (USART3_RX_STA & 0X8000)    // 接收到一帧完整数据
			{
					rlen = USART3_RX_STA & 0X7FFF;      // 获取本次接收到的数据长度
					USART3_RX_BUF[rlen] = 0;            // 添加字符串结束符
					printf("%s", USART3_RX_BUF);        // 串口打印收到的数据

					sprintf((char*)p, "Recv %d bytes", rlen);     // 构造提示信息
					LCD_Fill(30 + 54, 115, 239, 130, WHITE);         // 清除旧提示区
					POINT_COLOR = RED;
					Show_Str(30 + 54, 115, 156, 12, p, 12, 0);       // 显示“收到N字节”

					POINT_COLOR = BLUE;
					LCD_Fill(30, 130, 239, 319, WHITE);              // 清空数据显示区
					Show_Str(30, 130, 180, 190, USART3_RX_BUF, 12, 0); // 显示收到的具体内容
					
					int year;
					int month, day, hour, min, sec;
					if (sscanf((char*)USART3_RX_BUF, "%d,%d,%d,%d,%d,%d",
										 &year, &month, &day, &hour, &min, &sec) == 6)
					{
							// 年份取后两位
							int rtc_year = (year % 100);

							// 写入RTC
							RTC_Set_Time(hour, min, sec);
							RTC_Set_Date(rtc_year, month, day);
					}
				
					USART3_RX_STA = 0;  // 准备下一次接收

					if (constate == 0)
							t = 1000;       // 若之前为未连接状态，立即检查连接
					else
							t = 0;          // 已连接，则10秒后再次检查
			}

			if (t == 1000)          // 约10秒无新数据，检查连接状态
			{
					constate = atk_rm04_consta_check() - '0';   // 获取连接状态（0/1）
					if (constate)
							Show_Str(30 + 30, 80, 200, 12, "OK", 12, 0);
					else
							Show_Str(30 + 30, 80, 200, 12, "Flase", 12, 0);
					t = 0;
			}

			atk_rm04_at_response(1);    // 处理AT指令响应（心跳/保活）
		}
			
			myfree(SRAMIN, p);          // 释放临时缓冲区
			atk_rm04_quit_trans();      // 退出透传模式
			return res;
	} 










	