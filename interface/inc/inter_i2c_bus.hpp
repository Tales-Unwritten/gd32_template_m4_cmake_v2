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

    void init();

    // 总线控制:
    void start();
    void stop();

    uint8_t scan(uint8_t *found,uint8_t max_count, uint8_t start_addr,uint8_t end_addr);

    void write_byte(uint8_t ByteValue);
    uint8_t read_byte();

    uint8_t wait_ack(uint16_t timeout = 1000);
    void write_ack(uint8_t AckValue);

    // 互斥控制
    void lock();
    void unlock();
    uint8_t is_busy();

    // 总线恢复
    void bus_recovery();

    void _delay();
    void deinit();

private:
    void _write_scl(uint8_t BitValue);
    void _write_sda(uint8_t BitValue);
    FlagStatus _read_sda();

private:
    io_ctrl _scl;          // SCL 引脚对象
    io_ctrl _sda;          // SDA 引脚对象
    uint8_t _delay_val;
    volatile uint8_t _busy;
};

#endif /* __cplusplus */
