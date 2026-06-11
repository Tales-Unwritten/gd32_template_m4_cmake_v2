#pragma once

#ifdef __cplusplus

#include <cstdint>
#include "inter_io_ctrl.hpp"

/**
 * @brief  I2C 总线引脚配置（GD32 平台）
 */
typedef struct
{
    uint32_t PORT_SCL;   // SCL 所在端口，如 GPIOA
    uint32_t PORT_SDA;   // SDA 所在端口，如 GPIOB
    uint32_t SCL;        // SCL 引脚掩码，如 GPIO_PIN_6
    uint32_t SDA;        // SDA 引脚掩码，如 GPIO_PIN_7
} I2C_Bus_Info_t;

class inter_i2c_bus
{
public:
    inter_i2c_bus(const I2C_Bus_Info_t &cfg, uint8_t delay_us);

    // 禁止拷贝
    inter_i2c_bus(const inter_i2c_bus &) = delete;
    inter_i2c_bus &operator=(const inter_i2c_bus &) = delete;

    void i2c_init();

    // 总线控制:
    void i2c_start();
    void i2c_stop();

    void i2c_write_byte(uint8_t ByteValue);
    uint8_t i2c_read_byte();

    uint8_t i2c_wait_ack(uint16_t timeout = 1000);
    void i2c_write_ack(uint8_t AckValue);

    // 互斥控制
    void i2c_lock();
    void i2c_unlock();
    uint8_t i2c_is_busy();

    // 总线恢复
    void i2c_bus_recovery();

    void i2c_delay();
    void i2c_deinit();

private:
    void i2c_w_scl(uint8_t BitValue);
    void i2c_w_sda(uint8_t BitValue);
    FlagStatus i2c_r_sda();

private:
    io_ctrl _scl;          // SCL 引脚对象
    io_ctrl _sda;          // SDA 引脚对象
    uint8_t _delay_val;
    volatile uint8_t _busy;
};

#endif /* __cplusplus */
