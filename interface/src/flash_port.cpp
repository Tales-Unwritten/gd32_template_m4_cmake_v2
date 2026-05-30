// ============================================================
// @platform GD32F4xx（当前平台）
// ============================================================

#include "flash_port.hpp"

#include "gd32f4xx.h"
#include "gd32f4xx_fmc.h"

uint32_t flash_port::_safe_start = 0;
uint32_t flash_port::_safe_end   = 0;

void flash_port::init(uint32_t safe_start, uint32_t safe_size)
{
    _safe_start = safe_start;
    _safe_end   = safe_start + safe_size;
    fmc_unlock();
}

void flash_port::lock()
{
    fmc_lock();
}

bool flash_port::_in_range(uint32_t addr, uint32_t len)
{
    // 防整数溢出：先检查 len 是否超出剩余空间
    if (len > (_safe_end - _safe_start)) return false;
    return (addr >= _safe_start) && ((addr + len) <= _safe_end);
}

bool flash_port::_wait_busy()
{
    uint32_t timeout = 0xFFFF;
    while (fmc_flag_get(FMC_FLAG_BUSY)) {
        if (--timeout == 0) {
            fmc_flag_clear(FMC_FLAG_END | FMC_FLAG_OPERR | FMC_FLAG_WPERR
                          | FMC_FLAG_PGMERR | FMC_FLAG_PGSERR);
            return false;
        }
    }
    return true;
}

// ════════════════════════════════════════════════════════════
//  擦除
// ════════════════════════════════════════════════════════════

bool flash_port::erase(uint32_t addr)
{
    // 扇区擦除会清除整个扇区（最小 16KB），需要完整覆盖在安全区内
    if (!_in_range(addr, 1)) return false;

    fmc_flag_clear(FMC_FLAG_END | FMC_FLAG_OPERR | FMC_FLAG_WPERR
                  | FMC_FLAG_PGMERR | FMC_FLAG_PGSERR);

    if (fmc_page_erase(addr) != FMC_READY) return false;
    return _wait_busy();
}

// ════════════════════════════════════════════════════════════
//  写入
// ════════════════════════════════════════════════════════════

bool flash_port::write_word(uint32_t addr, uint32_t data)
{
    if (!_in_range(addr, 4)) return false;  // 安全检查

    fmc_flag_clear(FMC_FLAG_END | FMC_FLAG_OPERR | FMC_FLAG_WPERR
                  | FMC_FLAG_PGMERR | FMC_FLAG_PGSERR);

    if (fmc_word_program(addr, data) != FMC_READY) return false;
    return _wait_busy();
}

bool flash_port::write_byte(uint32_t addr, uint8_t data)
{
    if (!_in_range(addr, 1)) return false;

    // Flash 只支持 32-bit 写入，需要 read-modify-write
    uint32_t word_addr = addr & ~0x3;          // 对齐到 4 字节
    uint32_t shift     = (addr & 0x3) * 8;     // 字节偏移
    uint32_t old_word  = *(volatile uint32_t *)word_addr;  // 读当前值
    uint32_t new_word  = (old_word & ~(0xFFUL << shift)) | ((uint32_t)data << shift);

    return write_word(word_addr, new_word);
}

bool flash_port::write_bytes(uint32_t addr, const uint8_t *data, uint16_t len)
{
    if (!_in_range(addr, len)) return false;

    for (uint16_t i = 0; i < len; i++) {
        if (!write_byte(addr + i, data[i])) return false;
    }
    return true;
}

// ════════════════════════════════════════════════════════════
//  读取（直接内存访问）
// ════════════════════════════════════════════════════════════

uint8_t flash_port::read_byte(uint32_t addr)
{
    return *(volatile uint8_t *)addr;
}

uint32_t flash_port::read_word(uint32_t addr)
{
    return *(volatile uint32_t *)addr;
}

void flash_port::read_bytes(uint32_t addr, uint8_t *buf, uint16_t len)
{
    for (uint16_t i = 0; i < len; i++) {
        buf[i] = *(volatile uint8_t *)(addr + i);
    }
}
