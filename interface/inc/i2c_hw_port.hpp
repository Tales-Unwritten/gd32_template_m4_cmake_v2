#pragma once
// ============================================================
// @platform GD32F4xx
//   移植到新 MCU 时，本 .hpp 文件需替换：
//     - [PORT] #include "gd32f4xx.h" → 目标 SDK 头文件
//     - [PORT] enum class i2c_hw_id（按目标芯片调整枚举项）
// ============================================================

#ifdef __cplusplus

#include <cstdint>
#include "io_ctrl.hpp"
#include "gd32f4xx.h"       // [PORT] 替换为目标 SDK 头文件

// ============================================================
//  平台中性类型
// ============================================================

enum class i2c_hw_id : uint8_t { i2c0, i2c1, i2c2 };

// ============================================================
//  配置结构体（POD）
// ============================================================

struct I2cHwConfig {
    i2c_hw_id periph;      ///< 外设 ID
    uint32_t  scl_port;    ///< SCL GPIO 端口
    uint32_t  scl_pin;     ///< SCL 引脚掩码
    uint32_t  sda_port;    ///< SDA GPIO 端口
    uint32_t  sda_pin;     ///< SDA 引脚掩码
    uint32_t  af;          ///< 复用功能编号（I2C0/1/2 通常 AF4）
    uint32_t  clock_speed; ///< 时钟速度: 100000 (标准) 或 400000 (快速)
};

// ============================================================
//  硬件 I2C 主机端口
// ============================================================

/**
 * @brief 硬件 I2C 主机端口（阻塞读写）
 *
 * 使用示例（I2C1 @ 400kHz）：
 *   static i2c_hw_port i2c1({
 *       i2c_hw_id::i2c1, GPIOE, GPIO_PIN_2, GPIOE, GPIO_PIN_3,
 *       GPIO_AF_4, 400000
 *   });
 *   i2c1.init();
 *   i2c1.write_reg(0x40, 0x00, tx_data, 2);   // 写设备 0x40 寄存器 0x00
 *   i2c1.read_reg(0x40, 0x3E, rx_data, 2);    // 读设备 0x40 寄存器 0x3E
 */
class i2c_hw_port {
public:
    explicit i2c_hw_port(const I2cHwConfig &cfg);
    ~i2c_hw_port();

    i2c_hw_port(const i2c_hw_port &) = delete;
    i2c_hw_port &operator=(const i2c_hw_port &) = delete;

    // ── 生命周期 ──────────────────────────────────────────
    void init();
    void deinit();
    [[nodiscard]] bool is_initialized() const noexcept { return _initialized; }

    // ── 主机阻塞读写（原始） ──────────────────────────────

    /** @brief 主机阻塞写入
     *  @param dev_addr 7-bit 设备地址（不含 R/W bit）
     *  @param data     数据缓冲区
     *  @param len      字节数
     */
    bool write(uint8_t dev_addr, const uint8_t *data, uint16_t len);

    /** @brief 主机阻塞读取
     *  @param dev_addr 7-bit 设备地址（不含 R/W bit）
     *  @param data     接收缓冲区
     *  @param len      字节数
     */
    bool read(uint8_t dev_addr, uint8_t *data, uint16_t len);

    // ── 设备寄存器操作（兼容 I2C 设备模式） ────────────────

    /** @brief 写设备寄存器（先发 reg，再发 data） */
    bool write_reg(uint8_t dev_addr, uint8_t reg,
                   const uint8_t *data, uint16_t len);

    /** @brief 读设备寄存器（先写 reg，再读 data） */
    bool read_reg(uint8_t dev_addr, uint8_t reg,
                  uint8_t *data, uint16_t len);

    // ── 查询 ──────────────────────────────────────────────

    [[nodiscard]] i2c_hw_id periph() const noexcept { return _cfg.periph; }

private:
    void _enable_clock();

    I2cHwConfig _cfg;
    io_ctrl     _scl;
    io_ctrl     _sda;
    bool        _initialized;
    uint32_t    _periph;
};

#endif /* __cplusplus */
