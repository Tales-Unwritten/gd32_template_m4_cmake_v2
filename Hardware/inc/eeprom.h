#pragma once
#include "GD32F4xx.h"
#include "systick.h"
#include "../Hardware/inc/soft_i2c.h"
#include "stdint.h"
#include "string.h"

typedef enum
{
	EEPROM_ADDRESS_7bit_0 = 0x50, // 7-bit address 0x50
	EEPROM_ADDRESS_7bit_1 = 0x51, // 7-bit address 0x51
	EEPROM_ADDRESS_7bit_2 = 0x52, // 7-bit address 0x52
	EEPROM_ADDRESS_7bit_3 = 0x53, // 7-bit address 0x53
	EEPROM_ADDRESS_7bit_4 = 0x54, // 7-bit address 0x54
	EEPROM_ADDRESS_7bit_5 = 0x55, // 7-bit address 0x55
	EEPROM_ADDRESS_7bit_6 = 0x56, // 7-bit address 0x56
	EEPROM_ADDRESS_7bit_7 = 0x57, // 7-bit address 0x57
} eeprom_add_enum;


#define EEPROM_SIZE 256	// EEPROM size in bytes
#define EEPROM_PAGE_SIZE 8 // EEPROM page size in bytes

 void eeprom_config(uint32_t GPIOx, uint16_t SCL, uint16_t SDA);
uint8_t ee_Write_Byte(uint16_t addr, uint8_t data);
uint8_t ee_Read_Byte(uint16_t addr);

uint8_t ee_Write_Multi_Byte(uint16_t addr, uint8_t *data, uint16_t len);
uint8_t ee_Read_Multi_Byte(uint16_t addr, uint8_t *data, uint16_t len);

uint8_t ee_Write_Page_Byte(uint16_t page, uint8_t *data, uint16_t len);
uint8_t ee_Read_Page_Byte(uint16_t page, uint8_t *data, uint16_t len);

uint8_t ee_Read_Multi_Byte(uint16_t addr, uint8_t *data, uint16_t len);

void ee_erase_All(void);
void ee_erase_Byte(uint16_t addr);
void ee_erase_Page(uint16_t page);
void ee_Test_Read_Multi_Byte(void);
uint8_t ee_Check_Device(void);
