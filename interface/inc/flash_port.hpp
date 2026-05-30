#pragma once
// ============================================================
// @platform GD32F4xx
//   移植到新 MCU 时替换 FMC API：
//     fmc_unlock/lock → 目标 Flash 控制器 API
//     fmc_page_erase / fmc_word_program
//     FMU_FLASH_SIZE etc.
// ============================================================

#ifdef __cplusplus

#include <cstdint>
#include "gd32f4xx.h"

/**
 * @brief 内部 Flash 读写（存储校准参数/序列号，替代 EEPROM）
 *
 * 写入前必须先擦除所在扇区（扇区擦除粒度为 16KB~128KB）。
 *
 *   flash_port::init();
 *   flash_port::erase(0x08010000);                      // 擦除扇区
 *   flash_port::write_word(0x08010000, 0x12345678);     // 写 32-bit
 *   uint32_t val = flash_port::read_word(0x08010000);   // 读 32-bit
 */
class flash_port {
public:
    static void init();        ///< 解锁 Flash（写入前必须调用）
    static void lock();        ///< 锁定 Flash

    static bool erase(uint32_t page_addr);  ///< 扇区擦除
    static bool write_word(uint32_t addr, uint32_t data);
    static bool write_words(uint32_t addr, const uint32_t *data, uint16_t count);

    static uint32_t read_word(uint32_t addr);
    static void read_words(uint32_t addr, uint32_t *buf, uint16_t count);

    static uint32_t page_size(); ///< 扇区大小（字节）
};

#endif
