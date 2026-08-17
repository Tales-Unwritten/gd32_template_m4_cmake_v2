#pragma once
// ============================================================
// @platform GD32F4xx
//   移植到新 MCU 时，本 .hpp 文件需替换：
//     - [PORT] #include "gd32f4xx.h" → 目标 SDK 头文件
//     - [PORT] enum class spi_id（按目标芯片调整枚举项）
//     - [PORT] spi_clock_polarity_phase 合并映射（不同 SDK 命名不同）
// ============================================================

#ifdef __cplusplus

#include <cstdint>
#include "inter_io_ctrl.hpp"
#include "inter_spi_bus.hpp"
#include "gd32f4xx.h"       // [PORT] 替换为目标 SDK 头文件

// ============================================================
//  平台中性类型
// ============================================================

/** @brief SPI 外设 ID */
enum class spi_id : uint8_t {
    spi0, spi1, spi2, spi3, spi4
};

// ============================================================
//  配置结构体（POD）
// ============================================================

/**
 * @brief SPI 主机端口配置
 *
 * 各字段使用 GD32 标准外设库宏：
 *   prescaler:      SPI_PSC_2 ~ SPI_PSC_256
 *   clock_polarity_phase: SPI_CK_PL_LOW_PH_1EDGE ~ SPI_CK_PL_HIGH_PH_2EDGE
 *   frame_format:   SPI_ENDIAN_MSB / SPI_ENDIAN_LSB
 *   frame_size:     SPI_FRAMESIZE_8BIT / SPI_FRAMESIZE_16BIT
 *   cs_active_level: RESET（低有效）或 SET（高有效）
 */
struct SpiPortConfig {
    spi_id   periph;              ///< 外设 ID
    uint32_t sck_port;            ///< SCK GPIO 端口
    uint32_t sck_pin;             ///< SCK 引脚掩码
    uint32_t mosi_port;           ///< MOSI GPIO 端口
    uint32_t mosi_pin;            ///< MOSI 引脚掩码
    uint32_t miso_port;           ///< MISO GPIO 端口
    uint32_t miso_pin;            ///< MISO 引脚掩码
    uint32_t cs_port;             ///< CS GPIO 端口（0 = 不使用软件 CS）
    uint32_t cs_pin;              ///< CS 引脚掩码（0 = 不使用软件 CS）
    uint32_t cs_active_level;     ///< CS 有效电平: GPIO_PIN_SET 或 GPIO_PIN_RESET
    uint32_t af;                  ///< 复用功能编号（SPI0/1/2 通常为 GPIO_AF_5）
    uint32_t prescaler;           ///< SPI_PSC_2 ~ SPI_PSC_256
    uint32_t clock_polarity_phase;///< SPI_CK_PL_LOW_PH_1EDGE ~ SPI_CK_PL_HIGH_PH_2EDGE
    uint32_t frame_format;        ///< SPI_FRAMEFORMAT_MSB / SPI_FRAMEFORMAT_LSB
    uint16_t frame_size;          ///< SPI_FRAMESIZE_8BIT / SPI_FRAMESIZE_16BIT
};

// ============================================================
//  SPI 主机端口（阻塞模式，基础资源层）
// ============================================================

/**
 * @brief SPI 主机端口（阻塞式全双工传输）
 *
 * 使用示例（单设备，自动 CS 控制）：
 *   static spi_port flash({
 *       spi_id::spi4, GPIOF, GPIO_PIN_7, GPIOF, GPIO_PIN_9,
 *       GPIOF, GPIO_PIN_8, GPIOF, GPIO_PIN_6, GPIO_PIN_RESET,
 *       GPIO_AF_5, SPI_PSC_4, SPI_CK_PL_HIGH_PH_2EDGE,
 *       SPI_FRAMEFORMAT_MSB, SPI_FRAMESIZE_8BIT
 *   });
 *   flash.init();
 *   flash.transfer(tx_buf, rx_buf, 16);
 *
 * 使用示例（多设备共享总线，手动 CS）：
 *   bus.cs_select();
 *   bus.transfer(tx, rx, len);
 *   bus.cs_deselect();
 */
class spi_port : public spi_bus {
public:
    explicit spi_port(const SpiPortConfig &cfg);
    ~spi_port();

    spi_port(const spi_port &) = delete;
    spi_port &operator=(const spi_port &) = delete;

    // ── 生命周期 ──────────────────────────────────────────
    void init() override;
    void deinit() override;
    [[nodiscard]] bool is_initialized() const noexcept { return _initialized; }

    // ── 数据传输（阻塞、全双工） ──────────────────────────

    /**
     * @brief 全双工传输
     * @param tx  发送缓冲区（nullptr = 发送 0xFF 占位）
     * @param rx  接收缓冲区（nullptr = 丢弃接收数据）
     * @param len 传输字节数
     * @note  tx 和 rx 可指向同一缓冲区（原地交换）
     */
    void transfer(const uint8_t *tx, uint8_t *rx, uint16_t len) override;

    /** @brief 单字节全双工传输（发送 data，返回接收值） */
    uint8_t transfer_byte(uint8_t data) override;

    // ── 片选控制 ──────────────────────────────────────────

    /** @brief 拉低/拉高 CS（取决于 cs_active_level 配置） */
    void cs_select() override;

    /** @brief 释放 CS（恢复到无效电平） */
    void cs_deselect() override;

    // ── 查询 ──────────────────────────────────────────────

    [[nodiscard]] spi_id  periph() const noexcept { return _cfg.periph; }
    [[nodiscard]] bool    has_cs() const noexcept { return _has_cs; }

private:
    void      _enable_clock();

    SpiPortConfig _cfg;
    io_ctrl       _sck;
    io_ctrl       _mosi;
    io_ctrl       _miso;
    io_ctrl       _cs;          // 软件 CS（_has_cs=false 时未初始化）
    bool          _has_cs;
    bool          _initialized;
    uint32_t      _periph;      // 外设基址缓存
};

#endif /* __cplusplus */
