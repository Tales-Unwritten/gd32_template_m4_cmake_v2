#pragma once

#ifdef __cplusplus

#include <cstdint>
#include "inter_i2c.hpp"
#include "inter_io_ctrl.hpp"

/**
 * @brief  I2C 总线引脚配置（GD32 平台）
 */
typedef struct
{
    port_enum_t PORT_SCL;   // SCL 所在端口，如 portA
    port_enum_t PORT_SDA;   // SDA 所在端口，如 portB
    pin_enum_t  SCL;        // SCL 引脚，如 pin6
    pin_enum_t  SDA;        // SDA 引脚，如 pin7
    uint32_t    clock_speed = 400000;      // 目标 SCL 频率 Hz（默认 400k）
    uint32_t    sys_clk_hz  = 240000000;   // 系统主频 Hz（默认 240M，不依赖 SystemCoreClock 符号）
} I2C_Bus_Info_t;

class inter_i2c_bus : public i2c_bus
{
public:
    explicit inter_i2c_bus(const I2C_Bus_Info_t &cfg);

    // 禁止拷贝
    inter_i2c_bus(const inter_i2c_bus &) = delete;
    inter_i2c_bus &operator=(const inter_i2c_bus &) = delete;

    void init() override;

    // 总线控制:
    void start() override;
    void stop() override;

    // 平台扩展：从机地址扫描（非虚，不进入 i2c_bus 基类）
    uint8_t scan(uint8_t *found,uint8_t max_count, uint8_t start_addr,uint8_t end_addr);

    void write_byte(uint8_t ByteValue) override;
    uint8_t read_byte() override;

    bool wait_ack(uint16_t timeout = 1000) override;
    void write_ack(uint8_t AckValue) override;

    // 互斥控制
    void lock() override;
    void unlock() override;
    uint8_t is_busy() const override;

    // 总线恢复
    void bus_recovery() override;

    // 平台扩展：半周期延时（非虚）
    void _delay();
    void deinit() override;

private:
    void _write_scl(uint8_t BitValue);
    void _write_sda(uint8_t BitValue);
    polarity _read_sda();

private:
    io_ctrl _scl;          // SCL 引脚对象
    io_ctrl _sda;          // SDA 引脚对象
    uint32_t _delay_count;   // 半周期忙等循环次数（由 clock_speed/sys_clk_hz 反算）
    volatile uint8_t _busy;
};

#endif /* __cplusplus */
