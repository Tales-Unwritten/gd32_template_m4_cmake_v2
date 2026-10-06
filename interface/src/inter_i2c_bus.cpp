#include "inter_i2c_bus.hpp"
#include "link.h"
#include "gd32f4xx.h"   // DWT / CoreDebug（自标定用）

// ============================================================
//  "一次忙等循环"的 CPU 周期数
//    方案①（默认）：DWT 上电自标定 —— 与优化级别无关，最准
//    方案②（备用）：无 DWT 的平台把 #if 改为 0，用下面的固定值
// ============================================================
static uint32_t _measure_loop_cycles(void)
{
    static uint32_t cached = 0;
    if (cached) return cached;

#if 1   /* 方案①：DWT 自标定 */
    CoreDebug->DEMCR |= CoreDebug_DEMCR_TRCENA_Msk;
    DWT->CYCCNT = 0U;
    DWT->CTRL  |= DWT_CTRL_CYCCNTENA_Msk;

    const uint32_t N = 1000U, ITER = 1000U;
    uint32_t t0 = DWT->CYCCNT;
    for (uint32_t i = 0; i < N; i++)
    {
        volatile uint32_t c = ITER;
        while (c--) { }
    }
    uint32_t per = (DWT->CYCCNT - t0) / (N * ITER);
    cached = (per != 0U) ? per : 5U;   // 测得为 0 时兜底
#else   /* 方案②：固定值（无 DWT 的平台；需实测标定） */
    cached = 5U;                        // GD32F470 @240MHz,-O2 实测约 5 周期/次
#endif
    return cached;
}

// ============================================================
//  构造函数：通过初始化列表构建 io_ctrl 对象
// ============================================================

inter_i2c_bus::inter_i2c_bus(const I2C_Bus_Info_t &cfg)
    : _scl(cfg.PORT_SCL, cfg.SCL),
      _sda(cfg.PORT_SDA, cfg.SDA),
      _delay_count(0),
      _busy(0)
{
    // 半周期忙等循环次数 = sys_clk / (clock_speed * 2 * 每循环周期)
    const uint32_t rate = (cfg.clock_speed != 0U) ? cfg.clock_speed : 400000U;   // 防 0 除
    _delay_count = cfg.sys_clk_hz / (rate * 2U * _measure_loop_cycles());
}

// ============================================================
//  初始化：开漏输出 + 内部上拉
// ============================================================

void inter_i2c_bus::init()
{
    // SCL: 开漏输出，上拉，高速
    _scl.init(mode_output, pullup, otype_od, speed_high);

    // SDA: 开漏输出，上拉，高速
    _sda.init(mode_output, pullup, otype_od, speed_high);

    // 释放总线：SCL = 1, SDA = 1
    _write_scl(1);
    _write_sda(1);
}

void inter_i2c_bus::deinit()
{
    _scl.deinit();
    _sda.deinit();
    _busy = 0;
}

// ============================================================
//  延时
// ============================================================

void inter_i2c_bus::_delay()
{
    volatile uint32_t n = _delay_count;   // volatile：防 -O2 优化为空
    while (n--) { }
}

// ============================================================
//  底层 GPIO 操作
// ============================================================

void inter_i2c_bus::_write_scl(uint8_t BitValue)
{
    if (BitValue)
        _scl.high();
    else
        _scl.low();

    _delay();
}

void inter_i2c_bus::_write_sda(uint8_t BitValue)
{
    if (BitValue)
        _sda.high();
    else
        _sda.low();

    _delay();
}

polarity inter_i2c_bus::_read_sda()
{
    return _sda.read();
}

// ============================================================
//  总线控制
// ============================================================

void inter_i2c_bus::start()
{
    _write_sda(1);
    _write_scl(1);
    _write_sda(0);
    _write_scl(0);
}

void inter_i2c_bus::stop()
{
    _write_sda(0);
    _write_scl(1);
    _write_sda(1);
}

uint8_t inter_i2c_bus::scan(uint8_t *found, uint8_t max_count, uint8_t start_addr, uint8_t end_addr)
{

    if (!found || max_count == 0)
    {
        return 0;
    }
    uint8_t count = 0;
    lock();
    for (uint16_t addr = start_addr; addr <= end_addr && count < max_count; addr++)
    {
        start();
        write_byte((uint8_t)(addr << 1));
        if (wait_ack())
        {
            found[count++] = (uint8_t)addr;
        }
        stop();
    }
    unlock();
    return count;
}

// ============================================================
//  数据传输
// ============================================================

void inter_i2c_bus::write_byte(uint8_t ByteValue)
{
    for (uint8_t i = 0; i < 8; i++)
    {
        _write_sda(ByteValue & (0x80 >> i));
        _write_scl(1);
        _write_scl(0);
    }
}

uint8_t inter_i2c_bus::read_byte()
{
    uint8_t ByteValue = 0;

    // 释放 SDA（开漏模式下写 1 即释放，由上拉电阻拉高）
    _write_sda(1);

    for (uint8_t i = 0; i < 8; i++)
    {
        _write_scl(1);
        if (_read_sda() == High)
            ByteValue |= (0x80 >> i);
        _write_scl(0);
    }
    return ByteValue;
}

bool inter_i2c_bus::wait_ack(uint16_t timeout)
{
    // 释放 SDA，等待从机拉低
    _write_sda(1);
    _write_scl(1);

    while (_read_sda() == High)
    {
        if (--timeout == 0)
        {
            _write_scl(0);
            return false; // 超时，无应答
        }
    }

    _write_scl(0);
    return true; // 收到应答
}

void inter_i2c_bus::write_ack(uint8_t AckValue)
{
    // AckValue: 0 = ACK, 1 = NACK
    _write_sda(AckValue);
    _write_scl(1);
    _write_scl(0);
}

// ============================================================
//  互斥
// ============================================================

void inter_i2c_bus::lock()
{
    // IRQ-safe spinlock：关中断保证原子性
    uint32_t mask = __get_PRIMASK();
    __disable_irq();
    while (_busy)
    {
        __set_PRIMASK(mask); // 短暂恢复中断，让 ISR 运行
        __disable_irq();
    }
    _busy = 1;
    __set_PRIMASK(mask);
}

void inter_i2c_bus::unlock()
{
    _busy = 0;
}

uint8_t inter_i2c_bus::is_busy() const
{
    return _busy;
}

// ============================================================
//  总线恢复：发送 9 个时钟脉冲 + STOP
// ============================================================

void inter_i2c_bus::bus_recovery()
{
    // 先释放 SDA
    _write_sda(1);

    // 发送 9 个时钟脉冲，让从机释放总线
    for (int i = 0; i < 9; i++)
    {
        _write_scl(1);
        _write_scl(0);
    }

    // 发送 STOP 条件
    stop();
}
