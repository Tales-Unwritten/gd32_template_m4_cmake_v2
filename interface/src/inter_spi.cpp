// ============================================================
// @platform GD32F4xx（当前平台）
//   移植到新 MCU 时，本文件需要完整重写，但公共接口签名不变。
//   需要替换的映射：
//     - 外设基址:   SPI0/SPI1/SPI2 → 目标平台外设基址
//     - 时钟使能:   rcu_periph_clock_enable(RCU_SPIx) → 目标时钟 API
//     - 外设初始化:  spi_init(spi_parameter_struct) → 目标 SPI 初始化 API
//     - 数据收发:   spi_i2s_data_transmit / spi_i2s_data_receive
//     - 标志轮询:   spi_i2s_flag_get(SPI_FLAG_TBE/RBNE)
//     - GPIO 复用:  AF5 → 目标平台 AF 编号
// ============================================================

#include "inter_spi.hpp"

#include "gd32f4xx.h"
#include "gd32f4xx_rcu.h"
#include "gd32f4xx_spi.h"
#include "gd32f4xx_gpio.h"

// ============================================================
//  构造 / 析构
// ============================================================

spi_port::spi_port(const SpiPortConfig &cfg)
    : _cfg(cfg)
    , _sck(cfg.sck_port, cfg.sck_pin)
    , _mosi(cfg.mosi_port, cfg.mosi_pin)
    , _miso(cfg.miso_port, cfg.miso_pin)
    , _cs(cfg.cs_port != port_none ? cfg.cs_port : portA, cfg.cs_pin != pin_none ? cfg.cs_pin : pin0)
    , _has_cs(cfg.cs_port != port_none && cfg.cs_pin != pin_none)
    , _initialized(false)
    , _periph(0)
{
}

spi_port::~spi_port()
{
    deinit();
}

// ============================================================
//  时钟使能
// ============================================================

void spi_port::_enable_clock()
{
    // [PORT] GD32 时钟映射
    switch (_cfg.periph) {
        case spi_id::spi0: rcu_periph_clock_enable(RCU_SPI0); break;
        case spi_id::spi1: rcu_periph_clock_enable(RCU_SPI1); break;
        case spi_id::spi2: rcu_periph_clock_enable(RCU_SPI2); break;
        case spi_id::spi3: rcu_periph_clock_enable(RCU_SPI3); break;
        case spi_id::spi4: rcu_periph_clock_enable(RCU_SPI4); break;
    }
}

// ============================================================
//  init
// ============================================================

void spi_port::init()
{
    if (_initialized) return;

    // 外设基址映射
    // [PORT] GD32 外设基址
    switch (_cfg.periph) {
        case spi_id::spi0: _periph = SPI0; break;
        case spi_id::spi1: _periph = SPI1; break;
        case spi_id::spi2: _periph = SPI2; break;
        case spi_id::spi3: _periph = SPI3; break;
        case spi_id::spi4: _periph = SPI4; break;
    }

    // 时钟
    _enable_clock();

    // GPIO: SCK, MOSI, MISO → AF 推挽输出
    // MISO 虽然是输入，但 AF 模式下 gpio_init 自动处理方向
    _sck.init(mode_af, nopull, otype_pp, speed_high);
    _sck.set_af(_cfg.af);

    _mosi.init(mode_af, nopull, otype_pp, speed_high);
    _mosi.set_af(_cfg.af);

    _miso.init(mode_af, nopull, otype_pp, speed_high);
    _miso.set_af(_cfg.af);

    // CS → 普通推挽输出（非 AF），初始释放
    if (_has_cs) {
        _cs.init(mode_output, nopull, otype_pp, speed_high);
        cs_deselect();
    }

    // SPI 外设初始化
    spi_parameter_struct spi_cfg;
    spi_struct_para_init(&spi_cfg);

    spi_cfg.device_mode          = SPI_MASTER;
    spi_cfg.trans_mode           = SPI_TRANSMODE_FULLDUPLEX;
    spi_cfg.frame_size           = _cfg.frame_size;
    spi_cfg.nss                  = SPI_NSS_SOFT;        // 软件 NSS（由 CS io_ctrl 管理）
    spi_cfg.endian               = _cfg.frame_format;
    spi_cfg.clock_polarity_phase = _cfg.clock_polarity_phase;
    spi_cfg.prescale             = _cfg.prescaler;

    spi_init(_periph, &spi_cfg);
    spi_enable(_periph);

    _initialized = true;
}

// ============================================================
//  deinit
// ============================================================

void spi_port::deinit()
{
    if (!_initialized) return;

    spi_disable(_periph);

    _sck.deinit();
    _mosi.deinit();
    _miso.deinit();

    if (_has_cs) {
        _cs.deinit();
    }

    _periph = 0;
    _initialized = false;
}

// ============================================================
//  全双工传输
// ============================================================

void spi_port::transfer(const uint8_t *tx, uint8_t *rx, uint16_t len)
{
    if (!_initialized || len == 0) return;

    for (uint16_t i = 0; i < len; i++) {
        // 等待发送缓冲区空
        while (RESET == spi_i2s_flag_get(_periph, SPI_FLAG_TBE));

        // 发送（nullptr = 发送 0xFF 占位）
        uint8_t tx_byte = tx ? tx[i] : 0xFF;
        spi_i2s_data_transmit(_periph, tx_byte);

        // 等待接收缓冲区非空
        while (RESET == spi_i2s_flag_get(_periph, SPI_FLAG_RBNE));

        // 接收（nullptr = 丢弃）
        uint8_t rx_byte = (uint8_t)spi_i2s_data_receive(_periph);
        if (rx) rx[i] = rx_byte;
    }
}

// ============================================================
//  单字节传输
// ============================================================

uint8_t spi_port::transfer_byte(uint8_t data)
{
    if (!_initialized) return 0;

    while (RESET == spi_i2s_flag_get(_periph, SPI_FLAG_TBE));
    spi_i2s_data_transmit(_periph, data);

    while (RESET == spi_i2s_flag_get(_periph, SPI_FLAG_RBNE));
    return (uint8_t)spi_i2s_data_receive(_periph);
}

// ============================================================
//  片选控制
// ============================================================

void spi_port::cs_select()
{
    if (!_has_cs) return;

    if (_cfg.cs_active_level == High) {
        _cs.high();
    } else {
        _cs.low();
    }
}

void spi_port::cs_deselect()
{
    if (!_has_cs) return;

    if (_cfg.cs_active_level == High) {
        _cs.low();
    } else {
        _cs.high();
    }
}
