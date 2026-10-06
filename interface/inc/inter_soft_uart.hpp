#pragma once

#ifdef __cplusplus

#include <cstdint>

#include "inter_io_ctrl.hpp"
#include "inter_usart.hpp"   // uart_buffer_t + usart_port_base（软/硬串口同构）
#include "gd32f4xx.h"
#include "systick.h"         // get_tick（getc 超时）

/**
 * @brief 软件串口配置（GPIO 位反转，DWT 周期计数精确时序）
 *
 * 架构对齐 interface 层规范：
 *  - TX/RX 引脚由 io_ctrl 管理（铁律 2.5：GPIO 唯一抽象）
 *  - DWT 周期计数器提供精确位时序（Cortex-M3/4/7 标准调试模块）
 *  - 零堆分配、禁拷贝、init/deinit 生命周期管理
 *  - 主频自动跟随 SystemCoreClock（改时钟配置无需改本驱动）
 *
 * 原实现来源：soft_uart（纯 C 版），改写为 C++ inter 风格。
 *
 * 波特率精度 @240MHz（SystemCoreClock 自动适配）：
 *   9600: 25000 周期/bit（误差 0.00%）  19200: 12500（0.00%）
 *   38400: 6250（0.00%）  57600: 4167（0.02%）  115200: 2083（0.02%）
 */
struct SoftUartConfig
{
    port_enum_t tx_port;           // TX 端口，如 portB
    pin_enum_t  tx_pin;            // TX 引脚，如 pin6
    port_enum_t rx_port;           // RX 端口
    pin_enum_t  rx_pin;            // RX 引脚
    uint32_t baud = 115200;        // 波特率（见 BAUD_xxx 常量）
};

// ============================================================
//  软件串口（模板化：RX 缓冲区编译期静态分配，零堆）
// ============================================================

/**
 * @brief 软件串口端口（阻塞发送 + 轮询接收）
 *
 * @tparam RX_BUF_SIZE 接收缓冲区字节数（编译期确定，静态数组，零堆分配）
 *
 * 继承 usart_port_base：与硬件串口 usart_port<RX> 可多态互换。
 * 软串口专属扩展方法（putc/puts/write/try_getc/getc/set_baud/poll）不进入基类。
 *
 * 使用示例：
 *   static soft_uart_port<> suart({
 *       portB, pin6, portB, pin7,
 *       soft_uart_port<>::BAUD_115200
 *   });
 *   suart.init();
 *   suart.puts("hello\r\n");
 *
 *   uint8_t b;
 *   if (suart.try_getc(&b)) { ... }        // 非阻塞轮询
 *   if (suart.getc(&b, 100)) { ... }       // 阻塞 100ms 超时
 *
 * 注意：
 *  - try_getc() 无起始位时立即返回；检测到起始位后阻塞约 10 位时间
 *    接收完整字节。应在主循环中尽可能频繁调用（两字节间隔期间
 *    不监控 RX 线）。
 *  - 发送期间关闭中断可避免 ISR 抢占导致位时序拉长（如需）。
 *
 * @note 模板成员函数定义全部内联于本头文件，以保证调用点可见。
 */
template <uint16_t RX_BUF_SIZE = 256>
class soft_uart_port : public usart_port_base
{
public:
    // ── 波特率常量 ──────────────────────────────────────────
    static constexpr uint32_t BAUD_9600   = 9600U;
    static constexpr uint32_t BAUD_19200  = 19200U;
    static constexpr uint32_t BAUD_38400  = 38400U;
    static constexpr uint32_t BAUD_57600  = 57600U;
    static constexpr uint32_t BAUD_115200 = 115200U;

    static_assert(RX_BUF_SIZE > 0, "RX_BUF_SIZE must be greater than 0");

    explicit soft_uart_port(const SoftUartConfig &cfg);
    ~soft_uart_port() override;

    soft_uart_port(const soft_uart_port &) = delete;
    soft_uart_port &operator=(const soft_uart_port &) = delete;

    // ── 生命周期 ──────────────────────────────────────────

    void init() override;                  // 使能 DWT + 配置引脚（幂等）
    void deinit() override;                // 引脚恢复模拟输入，可再次 init()
    [[nodiscard]] bool is_initialized() const noexcept override { return _initialized; }

    // ── 发送（阻塞，约 10 位周期/字节；@115200 ≈ 87µs） ────

    void putc(uint8_t byte);       // 单字节（起始位 + 8 数据位 + 停止位）
    void puts(const char *str);    // 字符串
    void write(const uint8_t *data, uint16_t len);   // 原始缓冲区

    // ── 接收 ──────────────────────────────────────────────

    /**
     * @brief 非阻塞轮询接收（下降沿检测 + 毛刺过滤 + 中点采样）
     * @param byte [输出] 接收字节
     * @return true = 收到完整字节；false = 无起始位
     * @note  无数据时 < 1µs 返回；有数据时阻塞约 9.5 位周期
     */
    bool try_getc(uint8_t *byte);

    /**
     * @brief 阻塞接收，带超时
     * @param byte       [输出] 接收字节
     * @param timeout_ms 超时毫秒（0 = 无限等待）
     * @return true = 收到；false = 超时
     */
    bool getc(uint8_t *byte, uint32_t timeout_ms = 0);

    // ── 运行时配置 ────────────────────────────────────────

    void set_baud(uint32_t baud);  // 运行时改波特率（重新计算位周期）

    // ════════════════════════════════════════════════════════
    //  基类 usart_port_base 接口实现（与硬件串口对齐，
    //  协议层（Modbus 等）可经基类指针零适配桥接）
    // ════════════════════════════════════════════════════════

    /** @brief 阻塞发送（参数校验失败返回 false），等价 write() */
    bool send_data(const uint8_t *data, uint16_t len) override;

    /**
     * @brief 发送（对齐硬件串口签名）
     * @note  软串口无硬件中断发送能力：本方法退化为阻塞发送，
     *        返回时数据已全部发出（接口兼容，语义同步）
     */
    bool send_data_it(const uint8_t *data, uint16_t len) override;

    /** @brief 获取收发缓冲区引用（类型与硬件串口完全相同，协议层零适配） */
    [[nodiscard]] uart_buffer_t *buffer() noexcept override { return &_buf; }

    /** @brief 无硬件外设，恒返回 0（签名对齐 usart_port::periph()） */
    [[nodiscard]] uint32_t periph() const noexcept override { return 0; }

    /** @brief 接收缓冲区容量（编译期模板参数 RX_BUF_SIZE） */
    [[nodiscard]] uint16_t rx_buf_capacity() const noexcept override { return RX_BUF_SIZE; }

    /**
     * @brief 接收轮询（主循环必须周期调用）
     *
     * 内部 try_getc() 收字节入 rx_buf；连续 3.5 字符时间（Modbus RTU
     * 标准帧间隔）无新字节时置 rx_flag=1（模拟硬件 IDLE 中断判帧）。
     *
     * @return true = 当前有完整帧待处理（rx_flag==1）
     * @note  处理完帧后由用户清 rx_flag/rx_len
     */
    bool poll();

    /**
     * @brief 帧空闲判定阈值（20 字符）
     * @note  软串口为主循环轮询接收：判帧阈值必须大于主循环周期，
     *        否则帧内字节间隙（轮询间隔）会被误判为帧结束。
     *        3.5 字符（Modbus 标准）仅适用于硬件 ISR 场景。
     */
    static constexpr uint8_t FRAME_IDLE_CHARS = 20;

private:
    void _dwt_enable();            // 使能 DWT 周期计数器
    void _delay_cycles(uint32_t cycles);   // 精确忙等 N 周期（溢出安全）

    io_ctrl _tx;
    io_ctrl _rx;
    uint32_t _baud;
    uint32_t _cycles_per_bit;      // 一位周期 = SystemCoreClock / baud
    uint8_t  _rx_prev;             // 上一 RX 状态（边沿检测）
    bool     _initialized;

    // ── 协议桥接（与 usart_port 相同的缓冲视图） ──
    uint8_t        _rx_buf[RX_BUF_SIZE];   // 零堆分配接收缓冲
    uart_buffer_t  _buf;
    uint32_t       _last_rx_cycle;         // 最后收字节时刻（DWT 周期）
    uint32_t       _idle_cycles;           // 帧空闲判定阈值（3.5 字符）
};

// ============================================================
//  模板成员定义（必须对调用点可见）
// ============================================================

// ── 构造 / 析构 ──────────────────────────────────────────

template <uint16_t RX_BUF_SIZE>
soft_uart_port<RX_BUF_SIZE>::soft_uart_port(const SoftUartConfig &cfg)
    : _tx(cfg.tx_port, cfg.tx_pin)
    , _rx(cfg.rx_port, cfg.rx_pin)
    , _baud(cfg.baud)
    , _cycles_per_bit(0)
    , _rx_prev(1)              // 初始化假定线空闲（HIGH）
    , _initialized(false)
    , _buf{}
    , _last_rx_cycle(0)
    , _idle_cycles(0)
{
    _buf.rx_buf = _rx_buf;     // 绑定静态接收缓冲
}

template <uint16_t RX_BUF_SIZE>
soft_uart_port<RX_BUF_SIZE>::~soft_uart_port()
{
    deinit();
}

// ── DWT 周期计数器（Cortex-M3/4/7 精确时序） ─────────────

template <uint16_t RX_BUF_SIZE>
void soft_uart_port<RX_BUF_SIZE>::_dwt_enable()
{
    CoreDebug->DEMCR |= CoreDebug_DEMCR_TRCENA_Msk;  // 使能 DWT 模块
    DWT->CYCCNT = 0U;                                // 复位计数器
    DWT->CTRL  |= DWT_CTRL_CYCCNTENA_Msk;            // 使能周期计数
}

template <uint16_t RX_BUF_SIZE>
void soft_uart_port<RX_BUF_SIZE>::_delay_cycles(uint32_t cycles)
{
    uint32_t start = DWT->CYCCNT;
    while ((DWT->CYCCNT - start) < cycles)
    {
        /* 忙等；无符号减法在 32 位溢出时正确回绕 */
    }
}

// ── 生命周期 ─────────────────────────────────────────────

template <uint16_t RX_BUF_SIZE>
void soft_uart_port<RX_BUF_SIZE>::init()
{
    if (_initialized)
        return;   // 幂等

    _dwt_enable();

    // TX：推挽输出，初始 HIGH（串口空闲电平）
    _tx.init(mode_output, nopull, otype_pp, speed_high);
    _tx.high();

    // RX：上拉输入（悬空时 = HIGH）
    _rx.init(mode_input, pullup);

    _cycles_per_bit = SystemCoreClock / _baud;
    _idle_cycles    = _cycles_per_bit * 10U * FRAME_IDLE_CHARS;   // 20 字符判帧（轮询式软串口专用）
    _rx_prev        = 1;
    _buf.rx_buf     = _rx_buf;
    _buf.rx_len     = 0;
    _buf.rx_flag    = 0;
    _buf.tx_busy    = 0;
    _initialized    = true;
}

template <uint16_t RX_BUF_SIZE>
void soft_uart_port<RX_BUF_SIZE>::deinit()
{
    if (!_initialized)
        return;

    _tx.deinit();
    _rx.deinit();
    _buf.rx_len  = 0;
    _buf.rx_flag = 0;
    _initialized = false;
}

// ── 发送（阻塞） ─────────────────────────────────────────

template <uint16_t RX_BUF_SIZE>
void soft_uart_port<RX_BUF_SIZE>::putc(uint8_t byte)
{
    if (!_initialized)
        return;

    const uint32_t cpb = _cycles_per_bit;

    // 起始位（LOW）
    _tx.low();
    _delay_cycles(cpb);

    // 8 个数据位，低位在前
    for (uint8_t i = 0; i < 8; i++)
    {
        if (byte & (1U << i))
            _tx.high();
        else
            _tx.low();
        _delay_cycles(cpb);
    }

    // 停止位（HIGH）
    _tx.high();
    _delay_cycles(cpb);
}

template <uint16_t RX_BUF_SIZE>
void soft_uart_port<RX_BUF_SIZE>::puts(const char *str)
{
    if (!str)
        return;
    while (*str)
        putc((uint8_t)*str++);
}

template <uint16_t RX_BUF_SIZE>
void soft_uart_port<RX_BUF_SIZE>::write(const uint8_t *data, uint16_t len)
{
    if (!data)
        return;
    for (uint16_t i = 0; i < len; i++)
        putc(data[i]);
}

// ============================================================
//  接收：非阻塞轮询（下降沿检测 + 毛刺过滤 + 中点采样）
//
//   1. 读取 RX 引脚状态
//   2. 无下降沿（HIGH→LOW）→ 立即返回 false
//   3. 检测到下降沿 → 等 0.5 位时间到起始位中点
//   4. 毛刺过滤：验证起始位仍为 LOW
//   5. 中点采样 8 个数据位（低位在前）
//   6. 前进到停止位中点，同步边沿检测器
// ============================================================

template <uint16_t RX_BUF_SIZE>
bool soft_uart_port<RX_BUF_SIZE>::try_getc(uint8_t *byte)
{
    if (!_initialized || !byte)
        return false;

    const uint32_t cpb  = _cycles_per_bit;
    const uint32_t half = cpb >> 1;

    // 1. 读取当前引脚状态（始终更新边沿检测器）
    uint8_t cur = (_rx.read() == High) ? 1 : 0;

    // 2. 下降沿检测
    if (!(_rx_prev && !cur))
    {
        _rx_prev = cur;
        return false;   // 无起始位
    }

    // 3. 起始位刚刚开始：等待中点
    _rx_prev = cur;
    _delay_cycles(half);

    // 4. 毛刺过滤（假起始位）
    if (_rx.read() == High)
        return false;

    // 5. 中点采样 8 个数据位
    uint8_t val = 0;
    for (uint8_t i = 0; i < 8; i++)
    {
        _delay_cycles(cpb);
        if (_rx.read() == High)
            val |= (uint8_t)(1U << i);   // 低位在前
    }

    // 6. 前进到停止位中点，同步边沿检测器
    _delay_cycles(cpb);
    _rx_prev = (_rx.read() == High) ? 1 : 0;

    *byte = val;
    return true;
}

// ============================================================
//  接收：阻塞 + 超时（get_tick 毫秒精度）
// ============================================================

template <uint16_t RX_BUF_SIZE>
bool soft_uart_port<RX_BUF_SIZE>::getc(uint8_t *byte, uint32_t timeout_ms)
{
    if (!_initialized || !byte)
        return false;

    // timeout_ms == 0：无限等待
    if (timeout_ms == 0)
    {
        while (true)
        {
            if (try_getc(byte))
                return true;
        }
    }

    uint32_t start = get_tick();
    while (get_tick() - start <= timeout_ms)
    {
        if (try_getc(byte))
            return true;
    }
    return false;   // 超时
}

// ── 运行时配置 ───────────────────────────────────────────

template <uint16_t RX_BUF_SIZE>
void soft_uart_port<RX_BUF_SIZE>::set_baud(uint32_t baud)
{
    _baud = baud;
    if (_initialized)
    {
        _cycles_per_bit = SystemCoreClock / baud;
        _idle_cycles    = _cycles_per_bit * 10U * FRAME_IDLE_CHARS;
    }
}

// ============================================================
//  基类接口实现（对齐 usart_port）
// ============================================================

template <uint16_t RX_BUF_SIZE>
bool soft_uart_port<RX_BUF_SIZE>::send_data(const uint8_t *data, uint16_t len)
{
    if (!_initialized || !data || len == 0)
        return false;

    for (uint16_t i = 0; i < len; i++)
        putc(data[i]);
    return true;
}

template <uint16_t RX_BUF_SIZE>
bool soft_uart_port<RX_BUF_SIZE>::send_data_it(const uint8_t *data, uint16_t len)
{
    // 软串口无硬件中断发送：退化为同步阻塞发送（接口兼容）
    return send_data(data, len);
}

template <uint16_t RX_BUF_SIZE>
bool soft_uart_port<RX_BUF_SIZE>::poll()
{
    if (!_initialized)
        return false;

    // 收完当前可用字节（try_getc 无起始位时立即返回）
    uint8_t b;
    while (try_getc(&b))
    {
        if (_buf.rx_len < RX_BUF_SIZE)
        {
            _buf.rx_buf[_buf.rx_len] = b;
            _buf.rx_len = (uint16_t)(_buf.rx_len + 1);   // volatile 读写分离（C++20 弃用 ++）
        }
        _last_rx_cycle = DWT->CYCCNT;
    }

    // 帧空闲判定：3.5 字符时间无新字节 → 帧就绪（模拟 IDLE）
    if (_buf.rx_len > 0 && (DWT->CYCCNT - _last_rx_cycle) >= _idle_cycles)
    {
        _buf.rx_flag = 1;
    }

    return _buf.rx_flag != 0;
}

#endif /* __cplusplus */
