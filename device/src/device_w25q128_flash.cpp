#include "device_w25q128_flash.hpp"

// ── W25Q128 命令 ──────────────────────────────────────────
#define W25Q_CMD_JEDEC_ID  0x9F

// ── SPI 配置（来自 hardware/inc/hard_w25q128.h 的引脚定义） ───────
// SPI4, GPIOF: CS=PF6, CLK=PF7, MISO=PF8, MOSI=PF9
// Mode 3 (CPOL=1, CPHA=1), prescaler=2, MSB, 8-bit

w25q128_flash::w25q128_flash()
    : _spi(SpiPortConfig{
          spi_id::spi4,
          GPIOF, GPIO_PIN_7,   // SCK
          GPIOF, GPIO_PIN_9,   // MOSI
          GPIOF, GPIO_PIN_8,   // MISO
          GPIOF, GPIO_PIN_6,   // CS
          RESET,               // CS 低有效
          GPIO_AF_5,
          SPI_PSC_2,
          SPI_CK_PL_HIGH_PH_2EDGE,  // Mode 3
          SPI_ENDIAN_MSB,
          SPI_FRAMESIZE_8BIT
      })
    , _initialized(false)
{
}

void w25q128_flash::init()
{
    _spi.init();
    _initialized = true;
}

uint32_t w25q128_flash::read_jedec_id()
{
    uint32_t id = 0;

    _spi.cs_select();
    _spi.transfer_byte(W25Q_CMD_JEDEC_ID);
    id  = (uint32_t)_spi.transfer_byte(0xFF) << 16;
    id |= (uint32_t)_spi.transfer_byte(0xFF) << 8;
    id |= (uint32_t)_spi.transfer_byte(0xFF);
    _spi.cs_deselect();

    return id;
}
