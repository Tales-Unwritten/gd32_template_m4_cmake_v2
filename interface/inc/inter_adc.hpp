#pragma once
// ============================================================
// @platform GD32F4xx
//   移植到新 MCU 时替换：
//     - [PORT] #include "gd32f4xx.h" → 目标 SDK 头文件
//     - [PORT] enum class adc_id（按目标芯片调整枚举项）
// ============================================================

#ifdef __cplusplus

#include <cstdint>
#include "inter_io_ctrl.hpp"
#include "gd32f4xx.h"

enum class adc_id : uint8_t { adc0, adc1, adc2 };

// ============================================================
//  通道配置
// ============================================================

struct AdcChannelConfig {
    uint32_t port;          ///< GPIO 端口（0 = 不配置引脚，仅选通道号）
    uint32_t pin;           ///< 引脚掩码
    uint32_t channel;       ///< ADC 通道号: ADC_CHANNEL_0 ~ ADC_CHANNEL_18
    uint32_t sample_time;   ///< 本通道采样时间: ADC_SAMPLETIME_3 ~ _480
};

// ============================================================
//  端口配置
// ============================================================

struct AdcPortConfig {
    adc_id            periph;           ///< 外设 ID
    uint32_t          resolution;       ///< ADC_RESOLUTION_12B / 10B / 8B / 6B
    uint32_t          data_alignment;   ///< ADC_DATAALIGN_RIGHT / LEFT
    uint32_t          trigger_mode;     ///< ADC_TRGMOD_SOFTWARE（默认）
    uint32_t          vref_mv;          ///< 参考电压 mV（默认 3300）
    AdcChannelConfig  channels[8];      ///< 通道列表（port=0 结束）
    uint8_t           channel_count;    ///< 1~8
};

// ============================================================
//  ADC 端口
// ============================================================

/**
 * @brief ADC 端口（阻塞单通道 + 多通道扫描）
 *
 * 测试用：PA0 接 3.3V, PB3 接 3.3V
 *
 *   static adc_port adc({
 *       adc_id::adc0, ADC_RESOLUTION_12B, ADC_DATAALIGN_RIGHT,
 *       ADC_TRGMOD_SOFTWARE, 3300,
 *       {{{GPIOA, GPIO_PIN_0, ADC_CHANNEL_0, ADC_SAMPLETIME_144},
 *         {GPIOB, GPIO_PIN_3, ADC_CHANNEL_3, ADC_SAMPLETIME_144}}},
 *       2
 *   });
 *   adc.init();
 *   adc.calibrate();
 *   uint32_t mv = adc.read_mv(0);  // 读 PA0
 */
class adc_port {
public:
    explicit adc_port(const AdcPortConfig &cfg);
    ~adc_port();

    adc_port(const adc_port &) = delete;
    adc_port &operator=(const adc_port &) = delete;

    // ── 生命周期 ──────────────────────────────────────────
    void init();
    void deinit();
    [[nodiscard]] bool is_initialized() const noexcept { return _initialized; }

    // ── 自校准（init 后调用一次） ─────────────────────────
    void calibrate();

    // ── 单通道阻塞转换 ────────────────────────────────────

    uint16_t read_raw(uint8_t ch_index);
    uint32_t read_mv(uint8_t ch_index);

    // ── 扫描（所有通道顺序转换） ──────────────────────────

    void scan_raw(uint16_t *results);
    void scan_mv(uint32_t *results);

    // ── 查询 ──────────────────────────────────────────────
    [[nodiscard]] adc_id periph() const noexcept { return _cfg.periph; }
    [[nodiscard]] uint8_t channel_count() const noexcept { return _cfg.channel_count; }

private:
    void _enable_clock();
    void _config_channel(uint8_t index);
    uint16_t _do_convert();

    AdcPortConfig _cfg;

    // PWM 模式同款 aligned storage + placement new
    struct { alignas(io_ctrl) uint8_t data[sizeof(io_ctrl)]; } _ch_storage[8];
    io_ctrl *_ch_pins[8];

    bool     _initialized;
    uint32_t _periph;
};

#endif
