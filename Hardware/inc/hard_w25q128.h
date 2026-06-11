#pragma once
#include <stdint.h>
#include "gd32f4xx.h"
#include "systick.h"
#include <string.h>
#include <stdio.h>

// W25Q128 命令定义
#define Write_Enable             0x06
#define Write_Disable            0x04
#define Read_Data                0x03
#define Fast_Read_Data           0x0B
#define Page_Program             0x02
#define Sector_Erase             0x20
#define Chip_Erase               0xC7
#define Read_ID                  0x90
#define Read_JEDEC_ID            0x9F
#define Release_Power_Down       0xAB
#define Deep_Power_Down          0xB9
#define Volatile_SR_Write_Enable 0x50
#define Read_Status_Register1    0x05
#define Read_Status_Register2    0x35
#define Read_Status_Register3    0x15
#define Write_Status_Register1   0x01
#define Write_Status_Register2   0x31
#define Write_Status_Register3   0x11
#define Enable_Reset             0x66
#define Reset_Device             0x99
#define Release_From_Hibernate   0x66

#define Read_SFDP_Register       0x5A
#define Erase_Secure_Register    0x44
#define Program_Secure_Register  0x42
#define Read_Secure_Register     0x48

// GPIO 引脚定义
#define CSS_PIN                  GPIO_PIN_6
#define CLOCK_PIN                GPIO_PIN_7
#define MISO_PIN                 GPIO_PIN_8
#define MOSI_PIN                 GPIO_PIN_9

// 时钟使能定义
#define W25Qxx_RCU_SPI           RCU_SPI4
#define W25Qxx_RCU_GPIO          RCU_GPIOF
#define W25Qxx_RCU_GPIO_CSS      RCU_GPIOF

// SPI 和 GPIO 端口定义
#define W25Qxx_SPI               SPI4
#define W25Qxx_PORT              GPIOF
#define W25Qxx_CSS_PORT          GPIOF
#define W25Qxx_AF_SPI            GPIO_AF_5

// 超时定义
#define SPI_TIMEOUT              0xFFFF

// 函数声明
void w25qxx_css_enable(void);
void w25qxx_css_disable(void);
void w25qxx_wait_busy(void);
void w25qxx_erase_sector(uint32_t sector_num);
void w25qxx_write_enable(void);
uint8_t spi_read_write_byte(uint8_t dat);
void w25qxx_write(uint8_t *buffer, uint32_t addr, uint16_t numbyte);
void w25qxx_read(uint8_t *buffer, uint32_t addr, uint16_t numbyte);
uint32_t w25qxx_read_id(void);
void My_W25Q28_TEXT(void);

void w25qxx_spi_config(void);
