#ifndef __EEPROM_H__
#define __EEPROM_H__

#include "stm32f4xx_hal.h"

// 函数声明
void EEPROM_Init(void);
void EEPROM_WriteBytes(uint16_t addr, uint8_t *data, uint16_t len);
void EEPROM_ReadBytes(uint16_t addr, uint8_t *data, uint16_t len);

// 使用内部Flash模拟EEPROM的起始地址
#define EEPROM_START_ADDR 0x08080000  // 使用Flash的最后一页（根据具体芯片调整）

#endif




