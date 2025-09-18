#include "eeprom.h"
#include "string.h"

// FLASH页大小（根据具体STM32型号调整）
#define FLASH_PAGE_SIZE 0x4000  // 16KB for STM32F407

// EEPROM初始化
void EEPROM_Init(void) {
    // 不需要特殊初始化，使用内部Flash模拟EEPROM
}

// 擦除Flash页
static void FLASH_ErasePage(uint32_t page_address) {
    FLASH_EraseInitTypeDef EraseInitStruct;
    uint32_t PageError = 0;
    
    // 解锁Flash
    HAL_FLASH_Unlock();
    
    // 配置擦除参数
    EraseInitStruct.TypeErase = FLASH_TYPEERASE_SECTORS;
    EraseInitStruct.Sector = FLASH_SECTOR_11;  // 使用第11扇区（根据具体芯片调整）
    EraseInitStruct.NbSectors = 1;
    EraseInitStruct.VoltageRange = FLASH_VOLTAGE_RANGE_3;
    
    // 执行擦除
    HAL_FLASHEx_Erase(&EraseInitStruct, &PageError);
    
    // 锁定Flash
    HAL_FLASH_Lock();
}

// 写入数据
void EEPROM_WriteBytes(uint16_t addr, uint8_t *data, uint16_t len) {
    uint32_t flash_address = EEPROM_START_ADDR + addr;
    uint32_t *data_ptr = (uint32_t*)data;
    uint16_t words_to_write = (len + 3) / 4;  // 计算需要写入的字数（32位）
    
    // 解锁Flash
    HAL_FLASH_Unlock();
    
    // 如果需要，先擦除整个页
    if (addr == 0) {
        FLASH_ErasePage(EEPROM_START_ADDR);
    }
    
    // 写入数据
    for (uint16_t i = 0; i < words_to_write; i++) {
        HAL_FLASH_Program(FLASH_TYPEPROGRAM_WORD, flash_address + (i * 4), data_ptr[i]);
    }
    
    // 锁定Flash
    HAL_FLASH_Lock();
}

// 读取数据
void EEPROM_ReadBytes(uint16_t addr, uint8_t *data, uint16_t len) {
    uint32_t flash_address = EEPROM_START_ADDR + addr;
    
    // 直接从Flash读取数据
    memcpy(data, (void*)flash_address, len);
}

