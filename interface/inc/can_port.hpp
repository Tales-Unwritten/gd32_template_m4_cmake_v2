#pragma once
// ============================================================
// @platform GD32F4xx
//   移植到新 MCU 时，本 .hpp 文件需替换：
//     - [PORT] #include "gd32f4xx.h" → 目标 SDK 头文件
//     - [PORT] enum class can_id（按目标芯片调整枚举项）
//     - [PORT] can_trasnmit_message_struct → 目标 CAN 帧结构体
// ============================================================

#ifdef __cplusplus

#include <cstdint>
#include "io_ctrl.hpp"
#include "gd32f4xx.h"       // [PORT] 替换为目标 SDK 头文件

// ============================================================
//  平台中性类型
// ============================================================

enum class can_id : uint8_t { can0, can1 };

// ============================================================
//  配置结构体（POD）
// ============================================================

struct CanPortConfig {
    can_id   periph;       ///< 外设 ID
    uint32_t tx_port;      ///< CAN TX GPIO 端口
    uint32_t tx_pin;       ///< CAN TX 引脚掩码
    uint32_t rx_port;      ///< CAN RX GPIO 端口
    uint32_t rx_pin;       ///< CAN RX 引脚掩码
    uint32_t af;           ///< 复用功能编号（CAN0/1 通常 AF9）
    uint32_t baudrate;     ///< 波特率: 125000, 250000, 500000, 1000000
    uint32_t mode;         ///< CAN_NORMAL_MODE / CAN_LOOPBACK_MODE / CAN_SILENT_MODE
    uint8_t  filter_bank;  ///< 过滤器起始 bank（CAN0=0, CAN1=15）
};

// ============================================================
//  CAN 端口
// ============================================================

/**
 * @brief CAN 总线端口（阻塞收发 + 查询 FIFO）
 *
 * 使用示例（CAN0, PB8/PB9, 500kbps）：
 *   static can_port can({
 *       can_id::can0, GPIOB, GPIO_PIN_9, GPIOB, GPIO_PIN_8,
 *       GPIO_AF_9, 500000
 *   });
 *   can.init();
 *   can.send(0x123, false, data, 8);
 *
 * 使用示例（CAN loopback 测试）：
 *   can.send(0x7FF, false, "HELLO123", 8);
 *   uint8_t rx[8]; uint32_t id; uint8_t len; bool ext;
 *   if (can.recv(id, ext, rx, len)) { ... }
 */
class can_port {
public:
    explicit can_port(const CanPortConfig &cfg);
    ~can_port();

    can_port(const can_port &) = delete;
    can_port &operator=(const can_port &) = delete;

    // ── 生命周期 ──────────────────────────────────────────
    void init();
    void deinit();
    [[nodiscard]] bool is_initialized() const noexcept { return _initialized; }

    // ── 发送（阻塞） ──────────────────────────────────────

    /** @brief 发送 CAN 帧
     *  @param id        帧 ID（标准帧 11-bit 或扩展帧 29-bit）
     *  @param is_ext    是否为扩展帧
     *  @param data      数据指针
     *  @param len       数据长度（0~8 字节）
     *  @return          发送成功返回 true
     */
    bool send(uint32_t id, bool is_ext, const uint8_t *data, uint8_t len);

    // ── 接收（非阻塞，查询 FIFO0） ────────────────────────

    /** @brief 查询是否有接收帧就绪 */
    [[nodiscard]] bool rx_ready() const;

    /** @brief 接收 CAN 帧（非阻塞）
     *  @param[out] id       接收到的帧 ID
     *  @param[out] is_ext   是否为扩展帧
     *  @param[out] data     数据缓冲区（至少 8 字节）
     *  @param[out] len      实际数据长度
     *  @return              有帧时返回 true
     */
    bool recv(uint32_t &id, bool &is_ext, uint8_t *data, uint8_t &len);

    // ── 查询 ──────────────────────────────────────────────

    [[nodiscard]] can_id periph() const noexcept { return _cfg.periph; }
    [[nodiscard]] uint32_t hw() const noexcept { return _periph; }

private:
    void _enable_clock();
    uint32_t _can_clock() const;  // CAN 外设时钟频率（Hz）

    CanPortConfig _cfg;
    io_ctrl       _tx;
    io_ctrl       _rx;
    bool          _initialized;
    uint32_t      _periph;
};

#endif /* __cplusplus */
