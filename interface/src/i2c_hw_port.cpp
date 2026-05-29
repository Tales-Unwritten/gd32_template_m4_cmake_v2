// ============================================================
// @platform GD32F4xx（当前平台）
//   移植到新 MCU 时，本文件需要完整重写，但公共接口签名不变。
//   需要替换的映射：
//     - 外设基址:   I2C0/I2C1/I2C2 → 目标平台外设基址
//     - 时钟使能:   rcu_periph_clock_enable(RCU_I2Cx)
//     - 外设初始化:  i2c_clock_config / i2c_init
//     - 数据传输:   i2c_master_transmit_by_address / i2c_master_receive_by_address
//     - GPIO 复用:  AF4 → 目标平台 AF 编号
// ============================================================

#include "i2c_hw_port.hpp"

#include "gd32f4xx.h"
#include "gd32f4xx_rcu.h"
#include "gd32f4xx_i2c.h"

// ============================================================
//  构造 / 析构
// ============================================================

i2c_hw_port::i2c_hw_port(const I2cHwConfig &cfg)
    : _cfg(cfg)
    , _scl(cfg.scl_port, cfg.scl_pin)
    , _sda(cfg.sda_port, cfg.sda_pin)
    , _initialized(false)
    , _periph(0)
{
}

i2c_hw_port::~i2c_hw_port()
{
    deinit();
}

// ============================================================
//  时钟使能
// ============================================================

void i2c_hw_port::_enable_clock()
{
    // [PORT] GD32 时钟映射
    switch (_cfg.periph) {
        case i2c_hw_id::i2c0: rcu_periph_clock_enable(RCU_I2C0); break;
        case i2c_hw_id::i2c1: rcu_periph_clock_enable(RCU_I2C1); break;
        case i2c_hw_id::i2c2: rcu_periph_clock_enable(RCU_I2C2); break;
    }
}

// ============================================================
//  init
// ============================================================

void i2c_hw_port::init()
{
    if (_initialized) return;

    // 外设基址
    // [PORT] GD32 外设基址映射
    switch (_cfg.periph) {
        case i2c_hw_id::i2c0: _periph = I2C0; break;
        case i2c_hw_id::i2c1: _periph = I2C1; break;
        case i2c_hw_id::i2c2: _periph = I2C2; break;
    }

    _enable_clock();

    // GPIO: 开漏输出 + 上拉，AF 模式
    _scl.init(GPIO_MODE_AF, GPIO_PUPD_NONE, GPIO_OTYPE_OD, GPIO_OSPEED_50MHZ);
    _scl.set_af(_cfg.af);
    _sda.init(GPIO_MODE_AF, GPIO_PUPD_NONE, GPIO_OTYPE_OD, GPIO_OSPEED_50MHZ);
    _sda.set_af(_cfg.af);

    // I2C 时钟配置
    i2c_clock_config(_periph, _cfg.clock_speed, I2C_DTCY_2);

    // I2C 外设初始化（主机模式，7-bit 地址）
    i2c_mode_addr_config(_periph, I2C_I2CMODE_ENABLE,
                         I2C_ADDFORMAT_7BITS, 0);

    // 使能 I2C + 应答
    i2c_enable(_periph);
    i2c_ack_config(_periph, I2C_ACK_ENABLE);

    _initialized = true;
}

// ============================================================
//  deinit
// ============================================================

void i2c_hw_port::deinit()
{
    if (!_initialized) return;

    i2c_ack_config(_periph, I2C_ACK_DISABLE);
    i2c_disable(_periph);

    _scl.deinit();
    _sda.deinit();

    _periph = 0;
    _initialized = false;
}

// ============================================================
//  主机阻塞写入
// ============================================================

bool i2c_hw_port::write(uint8_t dev_addr, const uint8_t *data, uint16_t len)
{
    if (!_initialized || !data || len == 0) return false;

    // 等待总线空闲
    uint32_t timeout = 0xFFFF;
    while (i2c_flag_get(_periph, I2C_FLAG_I2CBSY)) {
        if (--timeout == 0) return false;
    }

    // 发送起始条件
    i2c_start_on_bus(_periph);

    // 等待 SBSEND（起始条件已发送）
    timeout = 0xFFFF;
    while (!i2c_flag_get(_periph, I2C_FLAG_SBSEND)) {
        if (--timeout == 0) return false;
    }

    // 发送设备地址（写方向）
    i2c_master_addressing(_periph, dev_addr, I2C_TRANSMITTER);

    // 等待 ADDSEND（地址已发送）
    timeout = 0xFFFF;
    while (!i2c_flag_get(_periph, I2C_FLAG_ADDSEND)) {
        if (--timeout == 0) return false;
    }
    i2c_flag_clear(_periph, I2C_FLAG_ADDSEND);

    // 发送数据
    for (uint16_t i = 0; i < len; i++) {
        timeout = 0xFFFF;
        while (!i2c_flag_get(_periph, I2C_FLAG_TBE)) {
            if (--timeout == 0) break;
        }
        i2c_data_transmit(_periph, data[i]);
    }

    // 等待传输完成
    timeout = 0xFFFF;
    while (!i2c_flag_get(_periph, I2C_FLAG_BTC)) {
        if (--timeout == 0) break;
    }

    // 发送停止条件
    i2c_stop_on_bus(_periph);
    while (i2c_flag_get(_periph, I2C_FLAG_STPDET)) {
        // 等待停止条件检测
    }
    i2c_flag_clear(_periph, I2C_FLAG_STPDET);

    return true;
}

// ============================================================
//  主机阻塞读取
// ============================================================

bool i2c_hw_port::read(uint8_t dev_addr, uint8_t *data, uint16_t len)
{
    if (!_initialized || !data || len == 0) return false;

    uint32_t timeout;

    // 等待总线空闲
    timeout = 0xFFFF;
    while (i2c_flag_get(_periph, I2C_FLAG_I2CBSY)) {
        if (--timeout == 0) return false;
    }

    // 发送起始条件
    i2c_start_on_bus(_periph);

    // 等待 SBSEND
    timeout = 0xFFFF;
    while (!i2c_flag_get(_periph, I2C_FLAG_SBSEND)) {
        if (--timeout == 0) return false;
    }

    // 发送设备地址（读方向）
    i2c_master_addressing(_periph, dev_addr, I2C_RECEIVER);

    // 等待 ADDSEND
    timeout = 0xFFFF;
    while (!i2c_flag_get(_periph, I2C_FLAG_ADDSEND)) {
        if (--timeout == 0) return false;
    }
    i2c_flag_clear(_periph, I2C_FLAG_ADDSEND);

    // 对于单字节读取，提前禁用 ACK（在最后一个字节之前）
    if (len == 1) {
        i2c_ack_config(_periph, I2C_ACK_DISABLE);
    }

    // 读取数据
    for (uint16_t i = 0; i < len; i++) {
        // 倒数第二个字节时，准备 NACK
        if (i == len - 2) {
            i2c_ack_config(_periph, I2C_ACK_DISABLE);
        }

        timeout = 0xFFFF;
        while (!i2c_flag_get(_periph, I2C_FLAG_RBNE)) {
            if (--timeout == 0) break;
        }
        data[i] = i2c_data_receive(_periph);
    }

    // 发送停止条件
    i2c_stop_on_bus(_periph);
    while (i2c_flag_get(_periph, I2C_FLAG_STPDET)) {}
    i2c_flag_clear(_periph, I2C_FLAG_STPDET);

    // 恢复 ACK
    i2c_ack_config(_periph, I2C_ACK_ENABLE);

    return true;
}

// ============================================================
//  设备寄存器操作
// ============================================================

bool i2c_hw_port::write_reg(uint8_t dev_addr, uint8_t reg,
                             const uint8_t *data, uint16_t len)
{
    // 构造 [reg + data] 组合缓冲区（栈上，零堆分配）
    if (len > 32) return false;  // 栈安全限制

    uint8_t buf[33];
    buf[0] = reg;
    for (uint16_t i = 0; i < len; i++) {
        buf[i + 1] = data[i];
    }
    return write(dev_addr, buf, len + 1);
}

bool i2c_hw_port::read_reg(uint8_t dev_addr, uint8_t reg,
                            uint8_t *data, uint16_t len)
{
    // 先写寄存器地址
    if (!write(dev_addr, &reg, 1)) return false;
    // 再读数据
    return read(dev_addr, data, len);
}
