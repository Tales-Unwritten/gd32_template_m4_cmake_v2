#pragma once
// ============================================================
// @platform GD32F4xx
//   移植到新 MCU 时，本 .hpp 文件需替换：
//     - [PORT] #include "gd32f4xx.h" → 目标 SDK 头文件
//     - [PORT] enum class usart_id（按目标芯片调整枚举项）
// ============================================================

#ifdef __cplusplus

#include <cstdint>
#include "io_ctrl.hpp"
#include "gd32f4xx.h"       // [PORT] 替换为目标 SDK 头文件

// ============================================================
//  平台中性类型
// ============================================================

/** @brief USART 外设 ID（平台中性枚举） */
enum class usart_id : uint8_t {
    usart0, usart1, usart2, uart3, uart4, usart5, uart6, uart7
};

// ============================================================
//  配置结构体（POD）
// ============================================================

/** @brief USART 异步端口配置 */
struct UsartAsyncConfig {
    usart_id id;            ///< 外设 ID
    uint32_t tx_port;       ///< TX GPIO 端口
    uint32_t tx_pin;        ///< TX 引脚掩码
    uint32_t rx_port;       ///< RX GPIO 端口
    uint32_t rx_pin;        ///< RX 引脚掩码
    uint32_t af;            ///< 复用功能编号 (GPIO_AF_7 for USART)
    uint32_t baudrate;      ///< 波特率
};

// ============================================================
//  收发缓冲区（兼容 Modbus 中间件、外部可访问）
// ============================================================

struct uart_buffer_t {
    uint8_t        *rx_buf;       ///< 接收缓冲区指针
    volatile uint16_t rx_len;     ///< 当前帧数据长度
    volatile uint8_t  rx_flag;    ///< 0=空闲  1=帧就绪（IDLE 触发）

    const uint8_t   *tx_data;     ///< 用户待发送数据指针
    volatile uint16_t tx_len;    ///< 待发送总字节数
    volatile uint16_t tx_idx;    ///< 当前发送位置
    volatile uint8_t  tx_busy;    ///< 0=空闲  1=中断发送进行中
};

// ============================================================
//  ISR 上下文（非模板，零虚函数开销）
//  模板类填充此结构体后注入全局映射表，
//  ISR 通过此结构体访问缓冲区，不依赖模板实例类型。
// ============================================================

struct usart_isr_ctx_t {
    uint32_t              periph;       ///< 外设基址 (USART0/USART1/...)
    volatile uint8_t     *rx_buf;       ///< → _buf.rx_buf
    volatile uint16_t    *rx_len;       ///< → &_buf.rx_len
    volatile uint8_t     *rx_flag;      ///< → &_buf.rx_flag
    uint16_t              rx_capacity;  ///< 接收缓冲区容量
    const uint8_t        *tx_data;      ///< → _buf.tx_data
    volatile uint16_t     tx_len;       ///< → _buf.tx_len
    volatile uint16_t     tx_idx;       ///< → _buf.tx_idx
    volatile uint8_t     *tx_busy;      ///< → &_buf.tx_busy
};

// ============================================================
//  ISR 注册 / 注销（自由函数，实现在 usart_async_port.cpp）
// ============================================================

void usart_isr_register(usart_id id, usart_isr_ctx_t *ctx);
void usart_isr_unregister(usart_id id);

// ============================================================
//  模板类：中断驱动的异步 USART（零堆分配）
// ============================================================

/**
 * @brief  中断驱动的异步 USART 端口
 *
 * @tparam RX_SIZE  接收缓冲区字节数（编译时确定，静态数组）
 * @tparam TX_SIZE  发送缓冲区字节数（默认 64）
 *
 * 使用示例：
 *   static usart_async_port<256> debug_uart({usart_id::usart0,
 *       GPIOA, GPIO_PIN_9, GPIOA, GPIO_PIN_10, GPIO_AF_7, 115200});
 *   debug_uart.init();
 */
template <size_t RX_SIZE, size_t TX_SIZE = 64>
class usart_async_port {
public:
    explicit usart_async_port(const UsartAsyncConfig &cfg);
    ~usart_async_port();

    usart_async_port(const usart_async_port &) = delete;
    usart_async_port &operator=(const usart_async_port &) = delete;

    // ── 生命周期 ──────────────────────────────────────────
    void init();
    void deinit();
    [[nodiscard]] bool is_initialized() const noexcept { return _initialized; }

    // ── 发送 ──────────────────────────────────────────────

    /** @brief 阻塞发送（轮询 TBE + TC） */
    void send_data(const uint8_t *data, uint16_t len);

    /** @brief 中断发送（非阻塞），通过 tx_busy() 判断完成 */
    void send_data_it(const uint8_t *data, uint16_t len);

    // ── 接收（用户轮询） ──────────────────────────────────

    /** @brief 是否有完整帧就绪（IDLE 触发） */
    [[nodiscard]] bool rx_ready() const noexcept { return _buf.rx_flag != 0; }

    /** @brief 清除接收标志 + 长度（处理后调用） */
    void rx_clear();

    /** @brief 接收缓冲区只读指针 */
    [[nodiscard]] const uint8_t *rx_data() const noexcept { return _buf.rx_buf; }

    /** @brief 当前帧长度 */
    [[nodiscard]] uint16_t rx_len() const noexcept { return _buf.rx_len; }

    // ── 发送状态 ──────────────────────────────────────────

    /** @brief 中断发送是否忙碌 */
    [[nodiscard]] bool tx_busy() const noexcept { return _buf.tx_busy != 0; }

    // ── 缓冲区访问（兼容 Modbus 中间件） ──────────────────

    /** @brief 获取 uart_buffer_t 视图（Modbus 兼容接口） */
    [[nodiscard]] uart_buffer_t *buffer() noexcept { return &_buf; }

    // ── 查询 ──────────────────────────────────────────────

    [[nodiscard]] usart_id    id()    const noexcept { return _cfg.id; }
    [[nodiscard]] uint32_t    periph() const noexcept { return _periph; }
    [[nodiscard]] const UsartAsyncConfig &config() const noexcept { return _cfg; }

private:
    void      _enable_clock();
    IRQn_Type _get_irq() const;

    UsartAsyncConfig  _cfg;
    io_ctrl           _tx;
    io_ctrl           _rx;
    bool              _initialized;

    // 静态缓冲区（零堆分配）
    uint8_t           _rx_arr[RX_SIZE];
    uint8_t           _tx_arr[TX_SIZE];

    // 缓冲区视图（Modbus 兼容 + ISR 直接操作）
    uart_buffer_t     _buf;

    // ISR 上下文（注入全局映射表）
    usart_isr_ctx_t   _isr_ctx;

    // 外设基址缓存（init 后有效）
    uint32_t          _periph;
};

// ============================================================
//  模板实现（头文件中，inline）
// ============================================================

// ── 构造 / 析构 ────────────────────────────────────────────

template <size_t RX_SIZE, size_t TX_SIZE>
usart_async_port<RX_SIZE, TX_SIZE>::usart_async_port(const UsartAsyncConfig &cfg)
    : _cfg(cfg)
    , _tx(cfg.tx_port, cfg.tx_pin)
    , _rx(cfg.rx_port, cfg.rx_pin)
    , _initialized(false)
    , _buf{}
    , _isr_ctx{}
    , _periph(0)
{
    _rx_arr[0] = '\0';
    _tx_arr[0] = '\0';
}

template <size_t RX_SIZE, size_t TX_SIZE>
usart_async_port<RX_SIZE, TX_SIZE>::~usart_async_port()
{
    deinit();
}

// ── 时钟使能 ───────────────────────────────────────────────

template <size_t RX_SIZE, size_t TX_SIZE>
void usart_async_port<RX_SIZE, TX_SIZE>::_enable_clock()
{
    // [PORT] GD32 时钟映射
    switch (_cfg.id) {
        case usart_id::usart0: rcu_periph_clock_enable(RCU_USART0); break;
        case usart_id::usart1: rcu_periph_clock_enable(RCU_USART1); break;
        case usart_id::usart2: rcu_periph_clock_enable(RCU_USART2); break;
        case usart_id::uart3:  rcu_periph_clock_enable(RCU_UART3);  break;
        case usart_id::uart4:  rcu_periph_clock_enable(RCU_UART4);  break;
        case usart_id::usart5: rcu_periph_clock_enable(RCU_USART5); break;
        case usart_id::uart6:  rcu_periph_clock_enable(RCU_UART6);  break;
        case usart_id::uart7:  rcu_periph_clock_enable(RCU_UART7);  break;
    }
}

// ── IRQ 映射 ────────────────────────────────────────────────

template <size_t RX_SIZE, size_t TX_SIZE>
IRQn_Type usart_async_port<RX_SIZE, TX_SIZE>::_get_irq() const
{
    // [PORT] GD32 中断向量映射
    switch (_cfg.id) {
        case usart_id::usart0: return USART0_IRQn;
        case usart_id::usart1: return USART1_IRQn;
        case usart_id::usart2: return USART2_IRQn;
        case usart_id::uart3:  return UART3_IRQn;
        case usart_id::uart4:  return UART4_IRQn;
        case usart_id::usart5: return USART5_IRQn;
        case usart_id::uart6:  return UART6_IRQn;
        case usart_id::uart7:  return UART7_IRQn;
    }
    return USART0_IRQn;
}

// ── init ────────────────────────────────────────────────────

template <size_t RX_SIZE, size_t TX_SIZE>
void usart_async_port<RX_SIZE, TX_SIZE>::init()
{
    if (_initialized) return;

    // 获取外设基址
    // [PORT] GD32 外设基址映射
    switch (_cfg.id) {
        case usart_id::usart0: _periph = USART0; break;
        case usart_id::usart1: _periph = USART1; break;
        case usart_id::usart2: _periph = USART2; break;
        case usart_id::uart3:  _periph = UART3;  break;
        case usart_id::uart4:  _periph = UART4;  break;
        case usart_id::usart5: _periph = USART5; break;
        case usart_id::uart6:  _periph = UART6;  break;
        case usart_id::uart7:  _periph = UART7;  break;
    }

    // 填充缓冲区视图
    _buf.rx_buf  = _rx_arr;
    _buf.rx_len  = 0;
    _buf.rx_flag = 0;
    _buf.tx_data = nullptr;
    _buf.tx_len  = 0;
    _buf.tx_idx  = 0;
    _buf.tx_busy = 0;

    // 填充 ISR 上下文
    _isr_ctx.periph     = _periph;
    _isr_ctx.rx_buf     = _rx_arr;
    _isr_ctx.rx_len     = &_buf.rx_len;
    _isr_ctx.rx_flag    = &_buf.rx_flag;
    _isr_ctx.rx_capacity = RX_SIZE;
    _isr_ctx.tx_data    = nullptr;
    _isr_ctx.tx_len     = 0;
    _isr_ctx.tx_idx     = 0;
    _isr_ctx.tx_busy    = &_buf.tx_busy;

    // 时钟
    _enable_clock();

    // GPIO AF
    _tx.init(GPIO_MODE_AF, GPIO_PUPD_PULLUP, GPIO_OTYPE_PP, GPIO_OSPEED_50MHZ);
    _tx.set_af(_cfg.af);
    _rx.init(GPIO_MODE_AF, GPIO_PUPD_PULLUP, GPIO_OTYPE_PP, GPIO_OSPEED_50MHZ);
    _rx.set_af(_cfg.af);

    // USART 8N1
    usart_deinit(_periph);
    usart_baudrate_set(_periph, _cfg.baudrate);
    usart_parity_config(_periph, USART_PM_NONE);
    usart_word_length_set(_periph, USART_WL_8BIT);
    usart_stop_bit_set(_periph, USART_STB_1BIT);

    // 中断：RXNE + IDLE + ERR
    nvic_irq_enable(_get_irq(), 0, 0);
    usart_interrupt_enable(_periph, USART_INT_RBNE);
    usart_interrupt_enable(_periph, USART_INT_IDLE);
    usart_interrupt_enable(_periph, USART_INT_ERR);

    // 使能
    usart_enable(_periph);
    usart_transmit_config(_periph, USART_TRANSMIT_ENABLE);
    usart_receive_config(_periph, USART_RECEIVE_ENABLE);

    usart_isr_register(_cfg.id, &_isr_ctx);
    _initialized = true;
}

// ── deinit ──────────────────────────────────────────────────

template <size_t RX_SIZE, size_t TX_SIZE>
void usart_async_port<RX_SIZE, TX_SIZE>::deinit()
{
    if (!_initialized) return;

    usart_isr_unregister(_cfg.id);

    nvic_irq_disable(_get_irq());
    usart_interrupt_disable(_periph, USART_INT_RBNE);
    usart_interrupt_disable(_periph, USART_INT_IDLE);
    usart_interrupt_disable(_periph, USART_INT_ERR);
    usart_interrupt_disable(_periph, USART_INT_TBE);
    usart_interrupt_disable(_periph, USART_INT_TC);

    usart_disable(_periph);
    usart_deinit(_periph);

    _tx.deinit();
    _rx.deinit();

    _buf.rx_len  = 0;
    _buf.rx_flag = 0;
    _buf.tx_busy = 0;
    _initialized = false;
}

// ── 阻塞发送 ────────────────────────────────────────────────

template <size_t RX_SIZE, size_t TX_SIZE>
void usart_async_port<RX_SIZE, TX_SIZE>::send_data(const uint8_t *data, uint16_t len)
{
    if (!_initialized || !data || len == 0) return;

    usart_interrupt_disable(_periph, USART_INT_TBE);
    usart_interrupt_disable(_periph, USART_INT_TC);

    for (uint16_t i = 0; i < len; ++i) {
        while (RESET == usart_flag_get(_periph, USART_FLAG_TBE));
        usart_data_transmit(_periph, data[i]);
    }
    while (RESET == usart_flag_get(_periph, USART_FLAG_TC));
}

// ── 中断发送 ────────────────────────────────────────────────

template <size_t RX_SIZE, size_t TX_SIZE>
void usart_async_port<RX_SIZE, TX_SIZE>::send_data_it(const uint8_t *data, uint16_t len)
{
    if (!_initialized || !data || len == 0) return;
    if (_buf.tx_busy) return;

    _buf.tx_data  = data;
    _buf.tx_len   = len;
    _buf.tx_idx   = 0;
    _buf.tx_busy  = 1;

    // 同步到 ISR 上下文
    _isr_ctx.tx_data = data;
    _isr_ctx.tx_len  = len;
    _isr_ctx.tx_idx  = 0;

    usart_interrupt_enable(_periph, USART_INT_TBE);
    usart_interrupt_enable(_periph, USART_INT_TC);
}

// ── 接收清除 ────────────────────────────────────────────────

template <size_t RX_SIZE, size_t TX_SIZE>
void usart_async_port<RX_SIZE, TX_SIZE>::rx_clear()
{
    // 关中断保护：防止 ISR 在写入之间修改 rx_flag/rx_len
    uint32_t mask = __get_PRIMASK();
    __disable_irq();
    _buf.rx_flag = 0;
    _buf.rx_len  = 0;
    __set_PRIMASK(mask);
}

#endif /* __cplusplus */
