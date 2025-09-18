#include "common.h"
///////////////////////////////////////////////////////////////////////////////////////////////////////////
// 本文件为 ALIENTEK STM32F407 开发板——ATK-RM04 WiFi 模块串口测试程序
// 作者：ALIENTEK
// 更新日期：2014/10/28
// 版本：V1.0
// 版权所有 (C) 广州市星翼电子科技有限公司 2014-2024
///////////////////////////////////////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////////////////////////////////
// 用户配置区

// 本地端口号：8086（可修改，但需与远端保持一致）
const u8* portnum="8086";

// WiFi STA 模式：连接已有热点
const u8* wifista_ssid="ARCUEID";          // 目标路由器 SSID
const u8* wifista_encryption="wpawpa2_aes";// 加密方式：WPA/WPA2 AES
const u8* wifista_password="123456789";    // 路由器密码

// WiFi AP 模式：模块自身做热点
const u8* wifiap_ssid="ATK-RM04";          // 模块广播的 SSID
const u8* wifiap_encryption="wpawpa2_aes";// 加密方式：WPA/WPA2 AES
const u8* wifiap_password="12345678";      // 热点密码

///////////////////////////////////////////////////////////////////////////////////////////////////////////
// 4 种网络模式说明
const u8 *ATK_RM04_NETMODE_TBL[4] = {"ROUTER", "ETH-COM", "WIFI-STA", "WIFI-AP"};
// 4 种传输模式说明
const u8 *ATK_RM04_WORKMODE_TBL[4] = {"TCP Server", "TCP Client", "UDP Server", "UDP Client"};

///////////////////////////////////////////////////////////////////////////////////////////////////////////
// ATK-RM04 初始化
void atk_rm04_init(void)
{
#if AT_MODE_CONFIG == 0
    GPIO_InitTypeDef GPIO_InitStruct = {0};

    // 使能 GPIOF 时钟
    __HAL_RCC_GPIOF_CLK_ENABLE();

    // 配置 PF6 为推挽输出，上拉，100MHz
    GPIO_InitStruct.Pin = GPIO_PIN_6;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Pull = GPIO_PULLUP;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
    HAL_GPIO_Init(GPIOF, &GPIO_InitStruct);

    // 设置 PF6 高电平
    HAL_GPIO_WritePin(GPIOF, GPIO_PIN_6, GPIO_PIN_SET);

    // 控制端 ES_CTRL 高电平
    ES_CTRL = 1;
#endif

    MX_USART3_UART_Init();
}
// 把串口 3 收到的数据通过 usmart 打印出来
// mode: 0 不清零 USART3_RX_STA；1 清零
void atk_rm04_at_response(u8 mode)
{
    if(USART3_RX_STA&0X8000)            // 接收到一帧数据
    {
        USART3_RX_BUF[USART3_RX_STA&0X7FFF]=0;
        printf("%s",USART3_RX_BUF);     // 回显
        if(mode)USART3_RX_STA=0;
    }
}

// 在串口接收缓存中查找指定字符串
// str: 待查找关键字
// 返回：首次出现位置指针；未找到返回 0
u8* atk_rm04_check_cmd(u8 *str)
{
    char *strx=0;
    if(USART3_RX_STA&0X8000)
    {
        USART3_RX_BUF[USART3_RX_STA&0X7FFF]=0;
        strx=strstr((const char*)USART3_RX_BUF,(const char*)str);
    }
    return (u8*)strx;
}

// 发送 AT 指令并等待应答
// cmd: 待发送指令（无需追加 \r\n）
// ack: 期望收到的应答关键字；若为 NULL 则不等待
// waittime: 最大等待时间（单位 10 ms）
// 返回：0 成功；1 超时
u8 atk_rm04_send_cmd(u8 *cmd,u8 *ack,u16 waittime)
{
    u8 res=0;
    USART3_RX_STA=0;
    u3_printf("%s\r",cmd);              // 发送指令
    if(ack&&waittime)                   // 需要等待应答
    {
        while(--waittime)
        {
            delay_ms(10);
            if(USART3_RX_STA&0X8000)
            {
                if(atk_rm04_check_cmd(ack))break;
                USART3_RX_STA=0;
            }
        }
        if(waittime==0)res=1;
    }
    return res;
}

// 退出透明传输模式
// 返回：0 成功；1 失败
u8 atk_rm04_quit_trans(void)
{
#if AT_MODE_CONFIG==1                   // 使用串口序列退出
    delay_ms(15);
    while((USART3->SR&0X40)==0);USART3->DR='+';
    delay_ms(15);
    while((USART3->SR&0X40)==0);USART3->DR='+';
    delay_ms(15);
    while((USART3->SR&0X40)==0);USART3->DR='+';
    delay_ms(500);
    while((USART3->SR&0X40)==0);USART3->DR=0X1B;
    delay_ms(15);
    while((USART3->SR&0X40)==0);USART3->DR=0X1B;
    delay_ms(15);
    while((USART3->SR&0X40)==0);USART3->DR=0X1B;
    delay_ms(15);
    return atk_rm04_send_cmd("at","at",20);
#else                                   // 使用硬件复位
    ES_CTRL=0;
    delay_ms(120);
    ES_CTRL=1;
    return 0;
#endif
}

// 查询模块联网状态
// 返回：0 未连接；1 已连接
u8 atk_rm04_consta_check(void)
{
    u8 *p,res;
    if(atk_rm04_quit_trans())return 0;
    atk_rm04_send_cmd("at+S2N_Stat=?","?",50);
    p=atk_rm04_check_cmd("\r\n");
    res=*(p+2)-'0';                     // 提取状态字符
    atk_rm04_send_cmd("at+out_trans=0","ok",50);
    return res;
}

//// 软件键盘字符表
//const u8* kbd_tbl[13]={"1","2","3","4","5","6","7","8","9",".","0","#","DEL"};
//u8* kbd_fn_tbl[2];

//// 在 LCD 上绘制 3×5 键盘（240×140）
//// x,y: 左上角坐标
//void atk_rm04_load_keyboard(u16 x,u16 y)
//{
//    u16 i;
//    POINT_COLOR=RED;
//    LCD_Fill(x,y,x+240,y+140,WHITE);
//    LCD_DrawRectangle(x,y,x+240,y+140);
//    LCD_DrawRectangle(x+80,y,x+160,y+140);
//    LCD_DrawRectangle(x,y+28,x+240,y+56);
//    LCD_DrawRectangle(x,y+84,x+240,y+112);
//    POINT_COLOR=BLUE;
//    for(i=0;i<15;i++)
//    {
//        if(i<13)Show_Str_Mid(x+(i%3)*80,y+6+28*(i/3),(u8*)kbd_tbl[i],16,80);
//        else   Show_Str_Mid(x+(i%3)*80,y+6+28*(i/3),kbd_fn_tbl[i-13],16,80);
//    }
//}

// 更新按键按下/松开状态
// keyx: 按键序号 0~14
// sta: 0 松开（白底）；1 按下（绿底）
//void atk_rm04_key_staset(u16 x,u16 y,u8 keyx,u8 sta)
//{
//    u16 i=keyx/3,j=keyx%3;
//    if(keyx>15)return;
//    if(sta)LCD_Fill(x+j*80+1,y+i*28+1,x+j*80+78,y+i*28+26,GREEN);
//    else   LCD_Fill(x+j*80+1,y+i*28+1,x+j*80+78,y+i*28+26,WHITE);
//    if(j&&(i>3))Show_Str_Mid(x+j*80,y+6+28*i,kbd_fn_tbl[keyx-13],16,80);
//    else        Show_Str_Mid(x+j*80,y+6+28*i,(u8*)kbd_tbl[keyx],16,80);
//}

// 获取键盘当前按键值（依赖触摸屏）
// 返回：1~15 有效键值；0 无按键
//u8 atk_rm04_get_keynum(u16 x,u16 y)
//{
//    u16 i,j;
//    static u8 key_x=0;
//    u8 key=0;
//    tp_dev.scan(0);
//    if(tp_dev.sta&TP_PRES_DOWN)         // 触摸按下
//    {
//        for(i=0;i<5;i++)
//            for(j=0;j<3;j++)
//                if(tp_dev.x[0]<(x+j*80+80)&&tp_dev.x[0]>(x+j*80)&&
//                   tp_dev.y[0]<(y+i*28+28)&&tp_dev.y[0]>(y+i*28))
//                { key=i*3+j+1; break; }

//        if(key)
//        {
//            if(key_x==key)key=0;        // 消抖
//            else
//            {
//                atk_rm04_key_staset(x,y,key_x-1,0);
//                key_x=key;
//                atk_rm04_key_staset(x,y,key_x-1,1);
//            }
//        }
//    }
//    else if(key_x)                      // 触摸松开
//    {
//        atk_rm04_key_staset(x,y,key_x-1,0);
//        key_x=0;
//    }
//    return key;
//}

// 获取 WAN 口 IP
// ipbuf: 至少 16 字节缓存
void atk_rm04_get_wanip(u8* ipbuf)
{
	u8 *p,*p1;
	if(atk_rm04_send_cmd("at+net_wanip=?","?",50))//鳳WAN IP華硊囮啖
	{
		ipbuf[0]=0;
		return;
	}		
	p=atk_rm04_check_cmd("\r\n");
	p1=(u8*)strstr((const char*)p,",");
	*p1=0;
	sprintf((char*)ipbuf,"%s",p+2);
}



// 获取 STA 模式连接状态
// 返回：0 未连接；1 已连接
u8 atk_rm04_get_wifista_state(void)
{
    u8 *p;
    atk_rm04_send_cmd("at+wifi_ConState=?","?",20);
    p=atk_rm04_check_cmd("\r\n");
    return strstr((const char*)p,"Connected")?1:0;
}

// 在 LCD 上显示模块基本信息
// wanip: 0 不显示 WAN IP；1 显示
void atk_rm04_msg_show(u16 x, u16 y, u8 wanip)
{
    u8 *p, *p1;
    POINT_COLOR = BLUE;

    if (wanip == 0)
    {
        atk_rm04_send_cmd("at+ver=?", "?", 20);
        p = atk_rm04_check_cmd("\r\n");
        LCD_ShowString(x, y, 240, 16, 16, (u8 *)"Version:");
        LCD_ShowString(x + 40, y, 240, 16, 16, p + 2);

        atk_rm04_send_cmd("at+netmode=?", "?", 20);
        p = atk_rm04_check_cmd("\r\n");
        LCD_ShowString(x, y + 16, 240, 16, 16, (u8 *)"Net Mode:");
        LCD_ShowString(x + 72, y + 16, 240, 16, 16, (u8 *)ATK_RM04_NETMODE_TBL[*(p + 2) - '0']);

        atk_rm04_send_cmd("at+wifi_conf=?", "?", 20);
        p = atk_rm04_check_cmd("\r\n");
        p1 = (u8 *)strstr((const char *)p, ","); *p1 = 0; p1++;
        LCD_ShowString(x, y + 32, 240, 16, 16, (u8 *)"SSID:");
        LCD_ShowString(x + 56, y + 32, 240, 16, 16, p + 2);

        p = p1; p1 = (u8 *)strstr((const char *)p, ","); *p1 = 0; p1++;
        LCD_ShowString(x, y + 48, 240, 16, 16, (u8 *)"Encryption:");
        LCD_ShowString(x + 72, y + 48, 240, 16, 16, p);

        LCD_ShowString(x, y + 64, 240, 16, 16, (u8 *)"Password:");
        LCD_ShowString(x + 40, y + 64, 240, 16, 16, p1);
    }

    atk_rm04_send_cmd("at+net_wanip=?", "?", 20);
    p = atk_rm04_check_cmd("\r\n");
    p1 = (u8 *)strstr((const char *)p, ","); *p1 = 0;
    LCD_ShowString(x, y + 80, 240, 16, 16, (u8 *)"WAN IP:");
    LCD_ShowString(x + 56, y + 80, 240, 16, 16, p + 2);

    POINT_COLOR = RED;
}

// 显示 WiFi 配置界面（STA 或 AP）
void atk_rm04_wificonf_show(u16 x, u16 y, u8* rmd, u8* ssid, u8* encryption, u8* password)
{
    POINT_COLOR = RED;

    LCD_ShowString(x, y, 240, 12, 12, rmd);
    LCD_ShowString(x, y + 20, 240, 12, 12, (u8 *)"SSID:");
    LCD_ShowString(x, y + 36, 240, 12, 12, (u8 *)"Encryption:");
    LCD_ShowString(x, y + 52, 240, 12, 12, (u8 *)"Password:");

    POINT_COLOR = BLUE;
    LCD_ShowString(x + 30, y + 20, 240, 12, 12, ssid);
    LCD_ShowString(x + 54, y + 36, 240, 12, 12, encryption);
    LCD_ShowString(x + 30, y + 52, 240, 12, 12, password);
}


// 选择传输模式菜单
// 返回：0 TCP 服务器；1 TCP 客户端；2 UDP 服务器；3 UDP 客户端
//u8 atk_rm04_netpro_sel(u16 x,u16 y,u8* name)
//{
//    u8 key,t=0,*p;
//    u8 netpro=0;
//    LCD_Clear(WHITE);
//    POINT_COLOR=RED;
//    p=mymalloc(SRAMIN,50);
//    sprintf((char*)p,"%s 传输模式选择",name);
//    Show_Str_Mid(0,y,p,16,240);
//    Show_Str(x,y+30,200,16,"KEY0:下一项",16,0);
//    Show_Str(x,y+50,200,16,"KEY1:上一项",16,0);
//    Show_Str(x,y+70,200,16,"KEY_UP:确认",16,0);
//    Show_Str(x,y+100,200,16,"当前选择:",16,0);
//    POINT_COLOR=BLUE;
//    Show_Str(x+16,y+120,200,16,"TCP 服务器",16,0);
//    Show_Str(x+16,y+140,200,16,"TCP 客户端",16,0);
//    Show_Str(x+16,y+160,200,16,"UDP 服务器",16,0);
//    Show_Str(x+16,y+180,200,16,"UDP 客户端",16,0);
//    POINT_COLOR=RED;
//    Show_Str(x,y+120,200,16,"→",16,0);
//    while(1)
//    {
//        key=KEY_Scan(0);
//        if(key)
//        {
//            if(key==4)break;            // KEY_UP 确认
//            Show_Str(x,y+120+netpro*20,200,16,"  ",16,0);
//            if(key==1){ if(netpro<3)netpro++; else netpro=0; }
//            else if(key==2){ if(netpro>0)netpro--; else netpro=3; }
//            Show_Str(x,y+120+netpro*20,200,16,"→",16,0);
//        }
//        delay_ms(10);
//        atk_rm04_at_response(1);
//        if((t++)==20){ t=0; LED0=!LED0; }
//    }
//    myfree(SRAMIN,p);
//    return netpro;
//}

// IP/端口输入界面
// 返回：0 确认；1 取消
//u8 atk_rm04_ip_set(u8* title,u8* mode,u8* port,u8* ip)
//{
//    u8 res=0,key,timex=0,iplen=0;
//    LCD_Clear(WHITE);
//    POINT_COLOR=RED;
//    Show_Str_Mid(0,30,title,16,240);
//    Show_Str(30,90,200,16,"传输模式:",16,0);
//    Show_Str(30,110,200,16,"IP地址:",16,0);
//    Show_Str(30,130,200,16,"端口:",16,0);
//    kbd_fn_tbl[0]="连接";
//    kbd_fn_tbl[1]="取消";
//    atk_rm04_load_keyboard(0,180);
//    POINT_COLOR=BLUE;
//    Show_Str(30+72,90,200,16,mode,16,0);
//    Show_Str(30+40,130,200,16,port,16,0);
//    ip[0]=0;
//    while(1)
//    {
//        key=atk_rm04_get_keynum(0,180);
//        if(key)
//        {
//            if(key<12){ if(iplen<15) ip[iplen++]=kbd_tbl[key-1][0]; }
//            else
//            {
//                if(key==13&&iplen) iplen--;     // 退格
//                if(key==14&&iplen){ res=0; break; } // 连接
//                if(key==15)       { res=1; break; } // 取消
//            }
//            ip[iplen]=0;
//            LCD_Fill(30+56,110,239,110+16,WHITE);
//            Show_Str(30+56,110,200,16,ip,16,0);
//        }
//        timex++;
//        if(timex==20){ timex=0; LED0=!LED0; }
//        delay_ms(10);
//        atk_rm04_at_response(1);
//    }
//    return res;
//}

// 主测试界面
void atk_rm04_mtest_ui(u16 x, u16 y)
{
    LCD_Clear(WHITE);         // 清屏
    POINT_COLOR = RED;        // 设置字体颜色为红色

    // 标题显示
    int16_t xpos = (240 - 16*18)/2;
		if(xpos < 0) xpos = 0;
		LCD_ShowString(xpos, y, 240, 16, 16, (u8 *)"ATK-RM04 WiFi Module Test");


    // 选项显示
    LCD_ShowString(x, y+25, 200, 16, 16, (u8 *)"Please select:");  
    LCD_ShowString(x, y+45, 200, 16, 16, (u8 *)"KEY0: Serial Ethernet");  
    LCD_ShowString(x, y+65, 200, 16, 16, (u8 *)"KEY1: WiFi STA");  
    LCD_ShowString(x, y+85, 200, 16, 16, (u8 *)"KEY_UP: WiFi AP");  

    // 显示提示信息
    atk_rm04_msg_show(x, y+125, 0);
}

// 模块整体测试入口
void atk_rm04_test(int selection)
{
		POINT_COLOR=RED;
    LCD_ShowString(0,30,240,16,16,"ATK-RM04 WiFi Module Test");   
    atk_rm04_quit_trans();
    while(atk_rm04_send_cmd("at","\r\n",20))          
    {
        LCD_ShowString(40,55,160,16,16,"Module No Response!!!");
        delay_ms(800);
        LCD_Fill(40,55,200,71,WHITE);
        LCD_ShowString(40,55,160,16,16,"Restarting module...");
        atk_rm04_quit_trans();
    }

    atk_rm04_mtest_ui(32,30);

    atk_rm04_at_response(1);
	
    if(selection == 0) atk_rm04_wifiap_test();
		if(selection == 1) atk_rm04_wifista_test();
}







