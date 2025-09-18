#ifndef __LCD_H
#define __LCD_H

#include "stm32f4xx_hal.h"
#include "stdlib.h"
#include "inttypes.h"

//********************************************************************************
// 本驱动适用于 ALIENTEK 2.8"/3.5"/4.3"/7" TFT 液晶模块
// 支持驱动 IC：ILI9341 / NT35310 / NT35510 / SSD1963 等
// 版权归 ALIENTEK 所有，仅供学习参考
//********************************************************************************

/*--------------------  外部变量声明  --------------------*/
extern SRAM_HandleTypeDef TFTSRAM_Handler;   // FSMC-SRAM 句柄（用于并口 LCD）

/* LCD 重要参数集合 */
typedef struct
{
    uint16_t width;      // 屏幕宽度（像素）
    uint16_t height;     // 屏幕高度（像素）
    uint16_t id;         // 驱动 IC 的 ID
    uint8_t  dir;        // 扫描方向：0-竖屏，1-横屏
    uint16_t wramcmd;    // 开始写 GRAM 指令
    uint16_t setxcmd;    // 设置 X 坐标指令
    uint16_t setycmd;    // 设置 Y 坐标指令
}_lcd_dev;

extern _lcd_dev lcddev;      // 在 .c 里定义的全局参数结构体
extern uint32_t POINT_COLOR; // 当前画笔颜色
extern uint32_t BACK_COLOR;  // 当前背景颜色

/*--------------------  位带操作定义  --------------------*/
#define BITBAND(addr, bitnum) ((addr & 0xF0000000)+0x2000000+((addr &0xFFFFF)<<5)+(bitnum<<2))
#define MEM_ADDR(addr)        *((volatile unsigned long *)(addr))
#define BIT_ADDR(addr, bitnum) MEM_ADDR(BITBAND(addr, bitnum))

#define GPIOB_ODR_Addr    (GPIOB_BASE + 20)     // GPIOB ODR 寄存器地址
#define LCD_LED           BIT_ADDR(GPIOB_ODR_Addr, 15)  // PB15 控制背光（1-亮，0-灭）

/* 并口 FMC 映射结构 */
typedef struct
{
    volatile uint16_t LCD_REG;  // 寄存器地址（RS=0）
    volatile uint16_t LCD_RAM;  // GRAM 地址（RS=1）
} LCD_TypeDef;

/* FMC Bank1-NOR/SRAM4 基址 + 偏移 = 0x6C00 007E */
#define LCD_BASE      ((uint32_t)(0x6C000000 | 0x0000007E))
#define LCD           ((LCD_TypeDef *) LCD_BASE)  // 强制转换为结构体指针，方便读写

/*--------------------  扫描方向宏  --------------------*/
#define L2R_U2D  0      // 从左到右，从上到下（默认）
#define L2R_D2U  1      // 从左到右，从下到上
#define R2L_U2D  2      // 从右到左，从上到下
#define R2L_D2U  3      // 从右到左，从下到上
#define U2D_L2R  4      // 从上到下，从左到右
#define U2D_R2L  5      // 从上到下，从右到左
#define D2U_L2R  6      // 从下到上，从左到右
#define D2U_R2L  7      // 从下到上，从右到左

#define DFT_SCAN_DIR  L2R_U2D  // 默认扫描方向

/*--------------------  常用颜色宏  --------------------*/
#define WHITE      0xFFFF
#define BLACK      0x0000
#define BLUE       0x001F
#define RED        0xF800
#define GREEN      0x07E0
#define CYAN       0x7FFF
#define YELLOW     0xFFE0
#define MAGENTA    0xF81F
#define GRAY       0x8430
#define BROWN      0xBC40
#define BRRED      0xFC07
#define DARKBLUE   0x01CF
#define LIGHTBLUE  0x7D7C
#define GRAYBLUE   0x5458
#define LIGHTGREEN 0x841F
#define LGRAY      0xC618   // 浅灰色（用作背景）
#define LGRAYBLUE  0xA651   // 浅灰蓝色
#define LBBLUE     0x2B12   // 浅棕蓝色（选中高亮）

/*--------------------  函数声明  --------------------*/
void LCD_Init(void);                // LCD 初始化
void LCD_DisplayOn(void);           // 打开显示
void LCD_DisplayOff(void);          // 关闭显示
void LCD_Clear(uint32_t Color);     // 清屏（填充颜色）
void LCD_SetCursor(uint16_t Xpos, uint16_t Ypos);  // 设置光标
void LCD_DrawPoint(uint16_t x, uint16_t y);        // 画点
void LCD_Fast_DrawPoint(uint16_t x, uint16_t y, uint32_t color); // 快速画点
uint32_t LCD_ReadPoint(uint16_t x, uint16_t y);    // 读点颜色
void LCD_Draw_Circle(uint16_t x0, uint16_t y0, uint8_t r);      // 画圆
void LCD_DrawLine(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2);      // 画线
void LCD_DrawRectangle(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2); // 画矩形
void LCD_Fill(uint16_t sx, uint16_t sy, uint16_t ex, uint16_t ey, uint32_t color);        // 填充矩形
void LCD_Color_Fill(uint16_t sx, uint16_t sy, uint16_t ex, uint16_t ey, uint16_t *color); // 填充颜色数组
void LCD_ShowChar(uint16_t x, uint16_t y, uint8_t num, uint8_t size, uint8_t mode);       // 显示一个字符
void LCD_ShowNum(uint16_t x, uint16_t y, uint32_t num, uint8_t len, uint8_t size);        // 显示数字
void LCD_ShowxNum(uint16_t x, uint16_t y, uint32_t num, uint8_t len, uint8_t size, uint8_t mode); // 显示任意进制数字
void LCD_ShowString(uint16_t x, uint16_t y, uint16_t width, uint16_t height, uint8_t size, uint8_t *p); // 显示字符串
void LCD_Show_Chinese(uint16_t x,uint16_t y,uint8_t num,uint8_t size);

/* 底层寄存器操作 */
void LCD_WriteReg(uint16_t LCD_Reg, uint16_t LCD_RegValue);
uint16_t LCD_ReadReg(uint16_t LCD_Reg);
void LCD_WriteRAM_Prepare(void);
void LCD_WriteRAM(uint16_t RGB_Code);

/* SSD1963 专用 */
void LCD_SSD_BackLightSet(uint8_t pwm);   // 背光调节（0-100）

/* 扫描/显示方向 */
void LCD_Scan_Dir(uint8_t dir);           // 设置 GRAM 扫描方向
void LCD_Display_Dir(uint8_t dir);        // 设置屏幕横竖屏
void LCD_Set_Window(uint16_t sx, uint16_t sy, uint16_t width, uint16_t height); // 设置开窗区域

/*--------------------  SSD1963 时序参数  --------------------*/
#define SSD_HOR_RESOLUTION   800   // 水平分辨率
#define SSD_VER_RESOLUTION   480   // 垂直分辨率

/* 行/场同步脉冲、前后肩参数 */
#define SSD_HOR_PULSE_WIDTH  1
#define SSD_HOR_BACK_PORCH   46
#define SSD_HOR_FRONT_PORCH  210

#define SSD_VER_PULSE_WIDTH  1
#define SSD_VER_BACK_PORCH   23
#define SSD_VER_FRONT_PORCH  22

/* 自动计算总宽度/高度 */
#define SSD_HT (SSD_HOR_RESOLUTION + SSD_HOR_BACK_PORCH + SSD_HOR_FRONT_PORCH)
#define SSD_HPS SSD_HOR_BACK_PORCH
#define SSD_VT (SSD_VER_RESOLUTION + SSD_VER_BACK_PORCH + SSD_VER_FRONT_PORCH)
#define SSD_VPS SSD_VER_BACK_PORCH

#endif /* __LCD_H */


