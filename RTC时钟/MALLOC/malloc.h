#ifndef __MALLOC_H
#define __MALLOC_H
#include "sys.h"
#include "stm32f4xx.h"
//////////////////////////////////////////////////////////////////////////////////	 
// ALIENTEK STM32F407开发板专用动态内存管理头文件
// 功能：提供多块 SRAM/CCM 的动态内存分配功能
// 作者：ALIENTEK 开发团队
// 官网：www.openedv.com
// 初版：2014/5/15
// 版本：V1.2
// 版权声明：版权所有，盗版必究，仅供学习和科研使用
//////////////////////////////////////////////////////////////////////////////////	 

#ifndef NULL
#define NULL 0
#endif

// 内存类型定义
#define SRAMIN	 0		// 内部 SRAM
#define SRAMEX   1		// 外部 SRAM
#define SRAMCCM  2		// CCM 内存（仅 CPU 可直接访问）

#define SRAMBANK 	3	// 支持的内存池数量

// 内存池1: 内部 SRAM
#define MEM1_BLOCK_SIZE			32  	  						
#define MEM1_MAX_SIZE			100*1024  						
#define MEM1_ALLOC_TABLE_SIZE	MEM1_MAX_SIZE/MEM1_BLOCK_SIZE  

// 内存池2: 外部 SRAM
#define MEM2_BLOCK_SIZE			32  	  						
#define MEM2_MAX_SIZE			960 *1024  						
#define MEM2_ALLOC_TABLE_SIZE	MEM2_MAX_SIZE/MEM2_BLOCK_SIZE
		 
// 内存池3: CCM 内存
#define MEM3_BLOCK_SIZE			32  	  						
#define MEM3_MAX_SIZE			60 *1024  						
#define MEM3_ALLOC_TABLE_SIZE	MEM3_MAX_SIZE/MEM3_BLOCK_SIZE

// 内存管理结构体
struct _m_mallco_dev
{
	void (*init)(u8);					// 初始化内存池
	u8 (*perused)(u8);		  	    	// 获取内存使用率
	u8 	*membase[SRAMBANK];				// 内存池起始地址
	u16 *memmap[SRAMBANK]; 				// 内存状态映射表
	u8  memrdy[SRAMBANK]; 				// 内存池是否就绪
};
extern struct _m_mallco_dev mallco_dev;	 // 全局内存管理对象

// 内存操作函数声明
void mymemset(void *s,u8 c,u32 count);	
void mymemcpy(void *des,void *src,u32 n);     
void my_mem_init(u8 memx);				
u32 my_mem_malloc(u8 memx,u32 size);	
u8 my_mem_free(u8 memx,u32 offset);		
u8 my_mem_perused(u8 memx);				

// 指针方式内存操作函数声明
void myfree(u8 memx,void *ptr);  			
void *mymalloc(u8 memx,u32 size);			
void *myrealloc(u8 memx,void *ptr,u32 size);

#endif














