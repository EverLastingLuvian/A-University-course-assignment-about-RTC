#ifndef COMMON_H
#define COMMON_H

#include "sys.h"
#include "usart.h"		
#include "delay.h"	 	 	 	 	 
#include "lcd.h"  
#include "w25qxx.h" 	 
#include "malloc.h"
#include "string.h"    
#include "text.h"		
#include "usart3.h" 
#include "ff.h"

/////////////////////////////////////////////////////////////////////////////////////////////////////////// 
// 本头文件用于ALIENTEK STM32F407开发板
// ATK-RM04 WiFi模块 驱动及测试
// 作者：ALIENTEK
// 官网：www.openedv.com
// 创建日期：2014/10/28
// 版本：V1.0
// 说明：定义ATK-RM04模块相关函数、宏、变量
// Copyright(C) ALIENTEK 2014-2024
// All rights reserved									  
/////////////////////////////////////////////////////////////////////////////////////////////////////////// 

/////////////////////////////////////////////////////////////////////////////////////////////////////////// 
#define AT_MODE_CONFIG		0	// 0：使用ES/RST硬件复位退出AT模式
									// 1：使用软件退出透传模式 (+++ 1B1B1B) 并进入AT模式
// ES/RST控制引脚
#define ES_CTRL				PFout(6)	// ES/RST控制引脚，连接PF6
/////////////////////////////////////////////////////////////////////////////////////////////////////////// 

// 模块初始化
void atk_rm04_init(void);

// AT命令相关函数
void atk_rm04_at_response(u8 mode);               // 处理AT命令返回
u8* atk_rm04_check_cmd(u8 *str);                 // 检查返回字符串
u8 atk_rm04_send_cmd(u8 *cmd,u8 *ack,u16 waittime); // 发送AT命令
u8 atk_rm04_quit_trans(void);                    // 退出透传模式
u8 atk_rm04_consta_check(void);                  // 检测模块状态

// 虚拟键盘函数
//void atk_rm04_load_keyboard(u16 x,u16 y);       // 显示虚拟键盘
//void atk_rm04_key_staset(u16 x,u16 y,u8 keyx,u8 sta); // 设置按键状态
//u8 atk_rm04_get_keynum(u16 x,u16 y);            // 获取按键编号

// 网络相关函数
void atk_rm04_get_wanip(u8* ipbuf);             // 获取WAN IP
u8 atk_rm04_get_wifista_state(void);            // 获取WIFI STA连接状态
void atk_rm04_msg_show(u16 x,u16 y,u8 wanip);   // 显示信息
void atk_rm04_wificonf_show(u16 x,u16 y,u8* rmd,u8* ssid,u8* encryption,u8* password); // 显示WiFi配置
//u8 atk_rm04_netpro_sel(u16 x,u16 y,u8* name);   // 选择网络协议
void atk_rm04_mtest_ui(u16 x,u16 y);            // 显示测试界面

//u8 atk_rm04_ip_set(u8* title,u8* mode,u8* port,u8* ip); // IP设置
void atk_rm04_test(int selection);                        // 测试函数

// 功能测试函数
u8 atk_rm04_wifiap_test(void);	// WIFI AP测试
u8 atk_rm04_wifista_test(void);

// 全局变量
extern const u8* portnum;			// 端口号

extern const u8* wifista_ssid;		// WIFI STA SSID
extern const u8* wifista_encryption;// WIFI STA 加密方式
extern const u8* wifista_password; 	// WIFI STA 密码

extern const u8* wifiap_ssid;		// WIFI AP SSID
extern const u8* wifiap_encryption;	// WIFI AP 加密方式
extern const u8* wifiap_password; 	// WIFI AP 密码

extern u8* kbd_fn_tbl[2];                     // 虚拟键盘功能键表
extern const u8* ATK_RM04_NETMODE_TBL[4];     // 网络模式表
extern const u8* ATK_RM04_WORKMODE_TBL[4];    // 工作模式表

#endif
