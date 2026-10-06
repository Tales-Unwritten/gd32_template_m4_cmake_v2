#pragma once

#ifdef __cplusplus

#include <cstdint>
#include <cstring>

#include "inter_io_ctrl.hpp"
#include "gd32f4xx.h"
#include "gd32f4xx_rcu.h"
#include "gd32f4xx_usart.h"
#include "gd32f4xx_misc.h"

/**
 * @brief  USART 端口引脚配置
 *
 * @note   接收缓冲区大小不再由配置承载：改由模板参数 RX_BUF_SIZE 决定
 *         （编译期静态数组，零堆分配）。
 */
struct UsartPortConfig
{
    uint32_t usart_periph;      ///< USART0 / USART1 / USART2 / UART3 / UART4 / USART5 / USART6 / UART7
    port_enum_t tx_port;        ///< TX GPIO 端口（portA~portI）
    pin_enum_t  tx_pin;         ///< TX 引脚（pin0~pin15）
    port_enum_t rx_port;        ///< RX GPIO 端口（portA~portI）
    pin_enum_t  rx_pin;         ///< RX 引脚（pin0~pin15）
    af_enum_t   af;             ///< AF 复用编号（af0~af15）
    uint32_t baudrate;          ///< 波特率
};

/**
 * @brief  串口收发缓冲区（用户通过 buffer() 获取引用）
 *
 * ISR 在接收空闲帧（IDLE）后将 rx_flag 置 1，用户在外部轮询此标志。
 * 处理完毕后用户自行将 rx_flag 清 0、rx_len 清 0。
 */
struct uart_buffer_t
{
    /* ── 接收（ISR → 用户） ── */
    uint8_t        *rx_buf;      ///< 接收缓冲区指针
    volatile uint16_t rx_len;    ///< 当前帧数据长度
    volatile uint8_t  rx_flag;   ///< 0=空闲  1=帧就绪（IDLE 触发）

    /* ── 发送（ISR 内部使用，用户只读 tx_busy） ── */
    const uint8_t   *tx_data;    ///< 用户待发送数据指针
    volatile uint16_t tx_len;   ///< 待发送总字节数
    volatile uint16_t tx_idx;   ///< 当前发送位置
    volatile uint8_t  tx_busy;   ///< 0=空闲  1=中断发送进行中
};

// ============================================================
//  串口抽象基类（软/硬串口共同基类，运行时可多态互换）
// ============================================================

/**
 * @brief  串口端口抽象基类
 *
 * 硬件串口 usart_port<RX> 与软件串口 soft_uart_port<RX> 均继承本类，
 * 协议层可仅依赖 usart_port_base& 完成收发，无需区分硬/软实现。
 */
class usart_port_base
{
public:
    virtual ~usart_port_base() = default;

    // ── 生命周期 ──────────────────────────────────────────
    virtual void init() = 0;
    virtual void deinit() = 0;
    [[nodiscard]] virtual bool is_initialized() const noexcept = 0;

    // ── 发送 ──────────────────────────────────────────────
    virtual bool send_data(const uint8_t *data, uint16_t len) = 0;
    virtual bool send_data_it(const uint8_t *data, uint16_t len) = 0;

    // ── 缓冲区 / 外设查询 ─────────────────────────────────
    virtual uart_buffer_t *buffer() noexcept = 0;
    virtual uint32_t periph() const noexcept = 0;
    virtual uint16_t rx_buf_capacity() const noexcept = 0;
};

// ── 端口映射表注册/注销（非模板自由函数，定义于 inter_usart.cpp）──
//    ISR 通过全局端口表分发；注册时用 port->periph() 定位表项。
void usart_port_register(usart_port_base *port);
void usart_port_unregister(usart_port_base *port);

// ============================================================
//  硬件串口（模板化：RX 缓冲区编译期静态分配，零堆）
// ============================================================

/**
 * @brief  硬件 USART 端口（阻塞发送 + 中断发送 + ISR 接收）
 *
 * @tparam RX_BUF_SIZE 接收缓冲区字节数（编译期确定，静态数组，零堆分配）
 *
 * @note   模板成员函数定义全部内联于本头文件，以保证调用点可见。
 *         各 USARTx_IRQHandler / 全局端口表 / 非模板辅助函数保留在 .cpp。
 */
template <uint16_t RX_BUF_SIZE = 256>
class usart_port : public usart_port_base
{
public:
    static constexpr uint16_t TX_BUF_SIZE = 256;   ///< 内部发送缓冲区大小

    static_assert(RX_BUF_SIZE > 0, "RX_BUF_SIZE must be greater than 0");

    explicit usart_port(const UsartPortConfig &cfg);
    ~usart_port() override;

    usart_port(const usart_port &) = delete;
    usart_port &operator=(const usart_port &) = delete;

    // ── 生命周期 ──────────────────────────────────────────

    void init() override;
    void deinit() override;
    [[nodiscard]] bool is_initialized() const noexcept override { return _initialized; }

    // ── 发送 ──────────────────────────────────────────────

    /** @brief 阻塞发送（超时保护）
     *  @return true=发送成功  false=参数错误或超时 */
    bool send_data(const uint8_t *data, uint16_t len) override;

    /** @brief 中断发送（非阻塞），通过 buf->tx_busy 判断完成
     *  @return true=已启动  false=参数错误或正在发送中 */
    bool send_data_it(const uint8_t *data, uint16_t len) override;

    // ── 缓冲区访问 ────────────────────────────────────────

    /** @brief 获取收发缓冲区引用（轮询 buf.rx_flag 和 buf.tx_busy） */
    [[nodiscard]] uart_buffer_t *buffer() noexcept override { return &_buf; }

    /** @brief 外设基址（ISR 内部查询用，外部无需关心） */
    [[nodiscard]] uint32_t periph() const noexcept override { return _cfg.usart_periph; }

    /** @brief 接收缓冲区容量（ISR 内部查询用，外部无需关心） */
    [[nodiscard]] uint16_t rx_buf_capacity() const noexcept override { return RX_BUF_SIZE; }

private:
    void      _enable_clock();
    IRQn_Type _get_irq() const;

    // ── 成员 ──────────────────────────────────────────────

    UsartPortConfig _cfg;
    io_ctrl         _tx;
    io_ctrl         _rx;
    bool            _initialized;

    // 编译期静态接收缓冲区（零堆分配，内部拥有）
    uint8_t        _rx_buf[RX_BUF_SIZE];

    // 暴露给用户和 ISR 的缓冲区视图
    uart_buffer_t  _buf;

    // 内部发送缓冲区（零堆分配，防止用户指针生命周期悬空）
    uint8_t        _tx_buf[TX_BUF_SIZE];
};

// ============================================================
//  模板成员定义（必须对调用点可见）
// ============================================================

// ── 构造 / 析构 ──────────────────────────────────────────

template <uint16_t RX_BUF_SIZE>
usart_port<RX_BUF_SIZE>::usart_port(const UsartPortConfig &cfg)
    : _cfg(cfg),
      _tx(cfg.tx_port, cfg.tx_pin),
      _rx(cfg.rx_port, cfg.rx_pin),
      _initialized(false),
      _buf{}
{
    _buf.rx_buf = _rx_buf;   // 绑定静态接收缓冲
}

template <uint16_t RX_BUF_SIZE>
usart_port<RX_BUF_SIZE>::~usart_port()
{
    deinit();
}

// ── 外设映射 ─────────────────────────────────────────────

template <uint16_t RX_BUF_SIZE>
void usart_port<RX_BUF_SIZE>::_enable_clock()
{
    const auto p = _cfg.usart_periph;
    if      (p == USART0) rcu_periph_clock_enable(RCU_USART0);
    else if (p == USART1) rcu_periph_clock_enable(RCU_USART1);
    else if (p == USART2) rcu_periph_clock_enable(RCU_USART2);
    else if (p == UART3)  rcu_periph_clock_enable(RCU_UART3);
    else if (p == UART4)  rcu_periph_clock_enable(RCU_UART4);
    else if (p == USART5) rcu_periph_clock_enable(RCU_USART5);
    else if (p == UART6)  rcu_periph_clock_enable(RCU_UART6);
    else if (p == UART7)  rcu_periph_clock_enable(RCU_UART7);
}

template <uint16_t RX_BUF_SIZE>
IRQn_Type usart_port<RX_BUF_SIZE>::_get_irq() const
{
    const auto p = _cfg.usart_periph;
    if      (p == USART0) return USART0_IRQn;
    else if (p == USART1) return USART1_IRQn;
    else if (p == USART2) return USART2_IRQn;
    else if (p == UART3)  return UART3_IRQn;
    else if (p == UART4)  return UART4_IRQn;
    else if (p == USART5) return USART5_IRQn;
    else if (p == UART6)  return UART6_IRQn;
    else if (p == UART7)  return UART7_IRQn;
    return USART0_IRQn;
}

// ── init / deinit ────────────────────────────────────────

template <uint16_t RX_BUF_SIZE>
void usart_port<RX_BUF_SIZE>::init()
{
    if (_initialized) return;

    // 绑定静态接收缓冲视图（零堆分配）
    _buf.rx_buf  = _rx_buf;
    _buf.rx_len  = 0;
    _buf.rx_flag = 0;
    _buf.tx_busy = 0;

    // 时钟
    _enable_clock();

    // GPIO
    _tx.init(mode_af, pullup, otype_pp, speed_high);
    _tx.set_af(_cfg.af);
    _rx.init(mode_af, pullup, otype_pp, speed_high);
    _rx.set_af(_cfg.af);

    // USART 8N1
    usart_deinit(_cfg.usart_periph);
    usart_baudrate_set(_cfg.usart_periph, _cfg.baudrate);
    usart_parity_config(_cfg.usart_periph, USART_PM_NONE);
    usart_word_length_set(_cfg.usart_periph, USART_WL_8BIT);
    usart_stop_bit_set(_cfg.usart_periph, USART_STB_1BIT);

    // 中断：RXNE + IDLE + ERR
    nvic_irq_enable(_get_irq(), 0, 0);
    usart_interrupt_enable(_cfg.usart_periph, USART_INT_RBNE);
    usart_interrupt_enable(_cfg.usart_periph, USART_INT_IDLE);
    usart_interrupt_enable(_cfg.usart_periph, USART_INT_ERR);

    // 使能 USART
    usart_enable(_cfg.usart_periph);
    usart_transmit_config(_cfg.usart_periph, USART_TRANSMIT_ENABLE);
    usart_receive_config(_cfg.usart_periph, USART_RECEIVE_ENABLE);

    usart_port_register(this);
    _initialized = true;
}

template <uint16_t RX_BUF_SIZE>
void usart_port<RX_BUF_SIZE>::deinit()
{
    if (!_initialized) return;

    usart_port_unregister(this);

    nvic_irq_disable(_get_irq());
    usart_interrupt_disable(_cfg.usart_periph, USART_INT_RBNE);
    usart_interrupt_disable(_cfg.usart_periph, USART_INT_IDLE);
    usart_interrupt_disable(_cfg.usart_periph, USART_INT_ERR);
    usart_interrupt_disable(_cfg.usart_periph, USART_INT_TBE);
    usart_interrupt_disable(_cfg.usart_periph, USART_INT_TC);

    usart_disable(_cfg.usart_periph);
    usart_deinit(_cfg.usart_periph);

    _tx.deinit();
    _rx.deinit();

    // 静态缓冲区始终有效，仅复位视图状态
    _buf.rx_buf  = _rx_buf;
    _buf.rx_len  = 0;
    _buf.rx_flag = 0;
    _buf.tx_busy = 0;

    _initialized = false;
}

// ── 阻塞发送（超时保护） ─────────────────────────────────

template <uint16_t RX_BUF_SIZE>
bool usart_port<RX_BUF_SIZE>::send_data(const uint8_t *data, uint16_t len)
{
    if (!_initialized || !data || len == 0) return false;

    usart_interrupt_disable(_cfg.usart_periph, USART_INT_TBE);
    usart_interrupt_disable(_cfg.usart_periph, USART_INT_TC);

    for (uint16_t i = 0; i < len; ++i)
    {
        uint32_t timeout = 0xFFFF;
        while (RESET == usart_flag_get(_cfg.usart_periph, USART_FLAG_TBE))
        {
            if (--timeout == 0) return false;   // 超时，防止硬件异常挂死
        }
        usart_data_transmit(_cfg.usart_periph, data[i]);
    }
    {
        uint32_t timeout = 0xFFFF;
        while (RESET == usart_flag_get(_cfg.usart_periph, USART_FLAG_TC))
        {
            if (--timeout == 0) return false;   // 超时，防止硬件异常挂死
        }
    }
    return true;
}

// ── 中断发送（内部拷贝，安全） ───────────────────────────

template <uint16_t RX_BUF_SIZE>
bool usart_port<RX_BUF_SIZE>::send_data_it(const uint8_t *data, uint16_t len)
{
    if (!_initialized || !data || len == 0) return false;
    if (_buf.tx_busy) return false;             // 正在发送中
    if (len > TX_BUF_SIZE) return false;        // 内部缓冲区不足

    // 内部拷贝：防止用户释放缓冲区后 ISR 访问野指针
    memcpy(_tx_buf, data, len);
    _buf.tx_data = _tx_buf;                     // 指向内部缓冲区
    _buf.tx_len  = len;
    _buf.tx_idx  = 0;
    _buf.tx_busy = 1;

    usart_interrupt_enable(_cfg.usart_periph, USART_INT_TBE);
    usart_interrupt_enable(_cfg.usart_periph, USART_INT_TC);
    return true;
}

#endif /* __cplusplus */
