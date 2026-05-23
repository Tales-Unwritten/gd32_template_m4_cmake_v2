#include "device_i2c_bus.hpp"
 #include "link.h"
// ============================================================
//  构造函数：通过初始化列表构建 io_ctrl 对象
// ============================================================

device_i2c_bus::device_i2c_bus(const I2C_Bus_Info_t &cfg, uint8_t delay_us)
    : _scl(cfg.PORT_SCL, cfg.SCL),
      _sda(cfg.PORT_SDA, cfg.SDA),
      _delay_val(delay_us),
      _busy(0)
{
}

// ============================================================
//  初始化：开漏输出 + 内部上拉
// ============================================================

void device_i2c_bus::i2c_init()
{
    // SCL: 开漏输出，上拉，高速
    _scl.init(GPIO_MODE_OUTPUT,
              GPIO_PUPD_PULLUP,
              GPIO_OTYPE_OD,
              GPIO_OSPEED_50MHZ);

    // SDA: 开漏输出，上拉，高速
    _sda.init(GPIO_MODE_OUTPUT,
              GPIO_PUPD_PULLUP,
              GPIO_OTYPE_OD,
              GPIO_OSPEED_50MHZ);

    // 释放总线：SCL = 1, SDA = 1
    i2c_w_scl(1);
    i2c_w_sda(1);
}

// ============================================================
//  延时
// ============================================================

void device_i2c_bus::i2c_delay()
{
    delay_us(_delay_val);
}

// ============================================================
//  底层 GPIO 操作
// ============================================================

void device_i2c_bus::i2c_w_scl(uint8_t BitValue)
{
    if (BitValue)
        _scl.high();
    else
        _scl.low();

    i2c_delay();
}

void device_i2c_bus::i2c_w_sda(uint8_t BitValue)
{
    if (BitValue)
        _sda.high();
    else
        _sda.low();

    i2c_delay();
}

FlagStatus device_i2c_bus::i2c_r_sda()
{
    return _sda.read();
}

// ============================================================
//  总线控制
// ============================================================

void device_i2c_bus::i2c_start()
{
    i2c_w_sda(1);
    i2c_w_scl(1);
    i2c_w_sda(0);
    i2c_w_scl(0);
}

void device_i2c_bus::i2c_stop()
{
    i2c_w_sda(0);
    i2c_w_scl(1);
    i2c_w_sda(1);
}

// ============================================================
//  数据传输
// ============================================================

void device_i2c_bus::i2c_write_byte(uint8_t ByteValue)
{
    for (uint8_t i = 0; i < 8; i++)
    {
        i2c_w_sda(ByteValue & (0x80 >> i));
        i2c_w_scl(1);
        i2c_w_scl(0);
    }
}

uint8_t device_i2c_bus::i2c_read_byte()
{
    uint8_t ByteValue = 0;

    // 释放 SDA（开漏模式下写 1 即释放，由上拉电阻拉高）
    i2c_w_sda(1);

    for (uint8_t i = 0; i < 8; i++)
    {
        i2c_w_scl(1);
        if (i2c_r_sda() == SET)
            ByteValue |= (0x80 >> i);
        i2c_w_scl(0);
    }
    return ByteValue;
}

uint8_t device_i2c_bus::i2c_wait_ack(uint16_t timeout)
{
    // 释放 SDA，等待从机拉低
    i2c_w_sda(1);
    i2c_w_scl(1);

    while (i2c_r_sda() == SET)
    {
        if (--timeout == 0)
        {
            i2c_w_scl(0);
            return 0;   // 超时，无应答
        }
    }

    i2c_w_scl(0);
    return 1;   // 收到应答
}

void device_i2c_bus::i2c_write_ack(uint8_t AckValue)
{
    // AckValue: 0 = ACK, 1 = NACK
    i2c_w_sda(AckValue);
    i2c_w_scl(1);
    i2c_w_scl(0);
}

// ============================================================
//  互斥
// ============================================================

void device_i2c_bus::i2c_lock()
{
    while (_busy)
        ;
    _busy = 1;
}

void device_i2c_bus::i2c_unlock()
{
    _busy = 0;
}

uint8_t device_i2c_bus::i2c_is_busy()
{
    return _busy;
}

// ============================================================
//  总线恢复：发送 9 个时钟脉冲 + STOP
// ============================================================

void device_i2c_bus::i2c_bus_recovery()
{
    // 先释放 SDA
    i2c_w_sda(1);

    // 发送 9 个时钟脉冲，让从机释放总线
    for (int i = 0; i < 9; i++)
    {
        i2c_w_scl(1);
        i2c_w_scl(0);
    }

    // 发送 STOP 条件
    i2c_stop();
}
