#pragma once
#ifdef __cplusplus
#include <cstdint>

/**
 * @brief I2C 总线抽象接口（软/硬 I2C 统一入口）
 * 软件 inter_i2c_bus 与硬件 i2c_hw_bus 实现同一接口，
 * 设备驱动依赖本基类即可透明切换软/硬 I2C。
 * 平台特有扩展保留在各自实现类中，不进入本基类。
 */
class i2c_bus
{
public:
    virtual ~i2c_bus() = default;

    virtual void    init()   = 0;
    virtual void    deinit() = 0;

    virtual void    start() = 0;
    virtual void    stop()  = 0;
    virtual void    write_byte(uint8_t data) = 0;
    virtual uint8_t read_byte() = 0;
    virtual bool    wait_ack(uint16_t timeout = 1000) = 0;
    virtual void    write_ack(uint8_t ack) = 0;   // 0=ACK, 1=NACK

    /** @brief 连续接收 len 字节并结束本次读传输（末字节 NACK + STOP）
     *  默认实现用 read_byte()+write_ack() 逐字节完成，软 I2C 直接可用。
     *  硬件 I2C 应覆写（末字节时序不同）。调用方不要再调 stop()。 */
    virtual void read_bytes(uint8_t *buf, uint16_t len)
    {
        if (buf == nullptr || len == 0U) return;
        for (uint16_t i = 0; i < len; i++)
        {
            buf[i] = read_byte();
            write_ack((i == (uint16_t)(len - 1U)) ? 1U : 0U);
        }
        stop();
    }

    virtual void    lock()   = 0;
    virtual void    unlock() = 0;
    virtual uint8_t is_busy() const = 0;

    /** @brief 从机卡死时发 9 个时钟 + STOP 恢复总线 */
    virtual void bus_recovery() = 0;
};
#endif /* __cplusplus */
