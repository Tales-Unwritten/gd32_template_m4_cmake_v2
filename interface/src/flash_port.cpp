// ============================================================
// @platform GD32F4xx（当前平台）
// ============================================================

#include "flash_port.hpp"

#include "gd32f4xx.h"
#include "gd32f4xx_fmc.h"

void flash_port::init()
{
    fmc_unlock();
}

void flash_port::lock()
{
    fmc_lock();
}

bool flash_port::erase(uint32_t page_addr)
{
    fmc_flag_clear(FMC_FLAG_END | FMC_FLAG_OPERR | FMC_FLAG_WPERR
                  | FMC_FLAG_PGMERR | FMC_FLAG_PGSERR);

    if (fmc_page_erase(page_addr) != FMC_READY) return false;

    uint32_t timeout = 0xFFFF;
    while (fmc_flag_get(FMC_FLAG_BUSY)) {
        if (--timeout == 0) return false;
    }
    return true;
}

bool flash_port::write_word(uint32_t addr, uint32_t data)
{
    fmc_flag_clear(FMC_FLAG_END | FMC_FLAG_OPERR | FMC_FLAG_WPERR
                  | FMC_FLAG_PGMERR | FMC_FLAG_PGSERR);

    if (fmc_word_program(addr, data) != FMC_READY) return false;

    uint32_t timeout = 0xFFFF;
    while (fmc_flag_get(FMC_FLAG_BUSY)) {
        if (--timeout == 0) return false;
    }
    return true;
}

bool flash_port::write_words(uint32_t addr, const uint32_t *data, uint16_t count)
{
    for (uint16_t i = 0; i < count; i++) {
        if (!write_word(addr + i * 4, data[i])) return false;
    }
    return true;
}

uint32_t flash_port::read_word(uint32_t addr)
{
    return *(volatile uint32_t *)addr;
}

void flash_port::read_words(uint32_t addr, uint32_t *buf, uint16_t count)
{
    for (uint16_t i = 0; i < count; i++) {
        buf[i] = *(volatile uint32_t *)(addr + i * 4);
    }
}

uint32_t flash_port::page_size()
{
    // GD32F450/470: 返回第一个扇区大小 (16KB)
    return 16 * 1024;
}
