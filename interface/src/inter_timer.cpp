// ============================================================
// @platform GD32F4xx（当前平台）
//   移植到新 MCU 时，本文件需要完整重写，但公共接口签名不变。
//   需要替换的映射：
//     - 外设基址:   TIMER0~TIMER7 → 目标平台外设基址
//     - 时钟使能:   rcu_periph_clock_enable(RCU_TIMERx)
//     - 外设初始化:  timer_init(timer_parameter_struct)
//     - PWM 配置:   timer_channel_output_config / pulse_value_config
//     - 中断管理:   nvic_irq_enable → 目标平台 NVIC API
//     - ISR 入口:   TIMER1_IRQHandler → 目标平台中断向量名
//     - GPIO 复用:  AF 编号 → 目标平台 AF 编号
// ============================================================

#include "inter_timer.hpp"

#include <new>

#include "gd32f4xx.h"
#include "gd32f4xx_rcu.h"
#include "gd32f4xx_timer.h"
#include "gd32f4xx_misc.h"

// ============================================================
//  全局端口映射表
// ============================================================

namespace {

constexpr uint8_t TIMER_MAX = 8;
timer_port *g_port_map[TIMER_MAX] = {};

uint8_t _timer_index(timer_id id)
{
    return static_cast<uint8_t>(id);
}

} // anonymous namespace

// ============================================================
//  ISR 触发入口（供 ISR 调用）
// ============================================================

void _timer_isr_fire(timer_port *port)
{
    if (!port || !port->is_initialized()) return;

    // 检查更新中断标志
    if (timer_interrupt_flag_get(port->_periph, TIMER_INT_FLAG_UP) == SET)
    {
        timer_interrupt_flag_clear(port->_periph, TIMER_INT_FLAG_UP);

        if (port->_timeout_cb) {
            port->_timeout_cb(port->_timeout_cb_data);
        }
    }
}

// ============================================================
//  构造 / 析构
// ============================================================

timer_port::timer_port(const TimerPortConfig &cfg)
    : _cfg(cfg)
    , _ch_pins{nullptr, nullptr, nullptr, nullptr}
    , _initialized(false)
    , _periph(0)
    , _timeout_cb(nullptr)
    , _timeout_cb_data(nullptr)
{
    // placement new: 在 aligned storage 上构造实际使用的通道
    // [FIX] 限制 channel_count 最大为 4，防止数组越界
    uint8_t count = (cfg.channel_count > 4) ? 4 : cfg.channel_count;
    for (uint8_t i = 0; i < count; i++) {
        if (cfg.channels[i].port != 0) {
            _ch_pins[i] = new (_ch_storage[i].data)
                io_ctrl(cfg.channels[i].port, cfg.channels[i].pin);
        }
    }
}

timer_port::~timer_port()
{
    deinit();
}

// ============================================================
//  时钟使能
// ============================================================

void timer_port::_enable_clock()
{
    // [PORT] GD32 时钟映射
    switch (_cfg.periph) {
        case timer_id::timer0: rcu_periph_clock_enable(RCU_TIMER0); break;
        case timer_id::timer1: rcu_periph_clock_enable(RCU_TIMER1); break;
        case timer_id::timer2: rcu_periph_clock_enable(RCU_TIMER2); break;
        case timer_id::timer3: rcu_periph_clock_enable(RCU_TIMER3); break;
        case timer_id::timer4: rcu_periph_clock_enable(RCU_TIMER4); break;
        case timer_id::timer5: rcu_periph_clock_enable(RCU_TIMER5); break;
        case timer_id::timer6: rcu_periph_clock_enable(RCU_TIMER6); break;
        case timer_id::timer7: rcu_periph_clock_enable(RCU_TIMER7); break;
    }
}

// ============================================================
//  IRQ 映射
// ============================================================

IRQn_Type timer_port::_get_irq() const
{
    // [PORT] GD32 中断向量映射
    // TIMER0/TIMER7 共享中断线，需在 ISR 中判断来源
    switch (_cfg.periph) {
        case timer_id::timer0: return TIMER0_UP_TIMER9_IRQn;
        case timer_id::timer1: return TIMER1_IRQn;
        case timer_id::timer2: return TIMER2_IRQn;
        case timer_id::timer3: return TIMER3_IRQn;
        case timer_id::timer4: return TIMER4_IRQn;
#ifdef TIMER5_IRQn
        case timer_id::timer5: return TIMER5_IRQn;
#else
        case timer_id::timer5: return TIMER6_IRQn;  // 降级到 TIMER6
#endif
        case timer_id::timer6: return TIMER6_IRQn;
        case timer_id::timer7: return TIMER7_UP_TIMER12_IRQn;
    }
    return TIMER2_IRQn;
}

// ============================================================
//  ISR 注册 / 注销
// ============================================================

void timer_port::_register_isr()
{
    uint8_t idx = _timer_index(_cfg.periph);
    if (idx < TIMER_MAX) g_port_map[idx] = this;
}

void timer_port::_unregister_isr()
{
    uint8_t idx = _timer_index(_cfg.periph);
    if (idx < TIMER_MAX) g_port_map[idx] = nullptr;
}

// ============================================================
//  init
// ============================================================

void timer_port::init()
{
    if (_initialized) return;

    // 外设基址映射
    // [PORT] GD32 外设基址
    switch (_cfg.periph) {
        case timer_id::timer0: _periph = TIMER0; break;
        case timer_id::timer1: _periph = TIMER1; break;
        case timer_id::timer2: _periph = TIMER2; break;
        case timer_id::timer3: _periph = TIMER3; break;
        case timer_id::timer4: _periph = TIMER4; break;
        case timer_id::timer5: _periph = TIMER5; break;
        case timer_id::timer6: _periph = TIMER6; break;
        case timer_id::timer7: _periph = TIMER7; break;
    }

    _enable_clock();

    // 定时器基本配置
    timer_parameter_struct timer_cfg;
    timer_struct_para_init(&timer_cfg);

    timer_cfg.prescaler         = _cfg.prescaler;
    timer_cfg.period            = _cfg.period;
    timer_cfg.clockdivision     = TIMER_CKDIV_DIV1;
    timer_cfg.counterdirection  = TIMER_COUNTER_UP;
    timer_cfg.repetitioncounter = 0;

    timer_init(_periph, &timer_cfg);

    // PWM 通道配置
    // GPIO AF 初始化
    for (uint8_t i = 0; i < _cfg.channel_count && i < 4; i++) {
        if (_cfg.channels[i].port == 0) continue;

        // deinit 后重建：_ch_pins[i] 已被销毁为 nullptr
        if (_ch_pins[i] == nullptr) {
            _ch_pins[i] = new (_ch_storage[i].data)
                io_ctrl(_cfg.channels[i].port, _cfg.channels[i].pin);
        }

        _ch_pins[i]->init(GPIO_MODE_AF, GPIO_PUPD_NONE, GPIO_OTYPE_PP, GPIO_OSPEED_50MHZ);
        _ch_pins[i]->set_af(_cfg.channels[i].af);

        // PWM 输出配置
        timer_oc_parameter_struct oc_cfg;
        timer_channel_output_struct_para_init(&oc_cfg);
        oc_cfg.outputstate  = TIMER_CCX_ENABLE;
        oc_cfg.ocpolarity   = TIMER_OC_POLARITY_HIGH;
        timer_channel_output_config(_periph, (uint16_t)i, &oc_cfg);
        timer_channel_output_pulse_value_config(_periph, (uint16_t)i, 0);
        timer_channel_output_mode_config(_periph, (uint16_t)i, TIMER_OC_MODE_PWM0);
    }

    // 清除残留中断标志，再使能中断
    timer_interrupt_flag_clear(_periph, TIMER_INT_FLAG_UP);
    timer_interrupt_enable(_periph, TIMER_INT_UP);
    nvic_irq_enable(_get_irq(), 0, 0);

    _register_isr();
    _initialized = true;
}

// ============================================================
//  deinit
// ============================================================

void timer_port::deinit()
{
    if (!_initialized) return;

    timer_interrupt_disable(_periph, TIMER_INT_UP);
    nvic_irq_disable(_get_irq());
    _unregister_isr();

    timer_disable(_periph);

    for (uint8_t i = 0; i < _cfg.channel_count && i < 4; i++) {
        if (_ch_pins[i]) {
            _ch_pins[i]->deinit();
            _ch_pins[i]->~io_ctrl();
            _ch_pins[i] = nullptr;
        }
    }

    _timeout_cb = nullptr;
    _periph = 0;
    _initialized = false;
}

// ============================================================
//  基本定时
// ============================================================

void timer_port::start()
{
    if (!_initialized) return;
    timer_enable(_periph);
}

void timer_port::stop()
{
    if (!_initialized) return;
    timer_disable(_periph);
}

uint32_t timer_port::counter() const
{
    if (!_initialized) return 0;
    return timer_counter_read(_periph);
}

void timer_port::set_period(uint32_t period)
{
    if (!_initialized) return;
    timer_autoreload_value_config(_periph, period);
}

// ============================================================
//  PWM
// ============================================================

void timer_port::pwm_set_duty(uint8_t channel, uint32_t value)
{
    if (!_initialized || channel >= _cfg.channel_count) return;
    timer_channel_output_pulse_value_config(_periph, (uint16_t)channel, value);
}

// ============================================================
//  回调
// ============================================================

void timer_port::on_timeout(timeout_cb_t cb, void *user_data)
{
    _timeout_cb      = cb;
    _timeout_cb_data = user_data;
}

// ============================================================
//  ISR 入口
// ============================================================

extern "C" void TIMER0_UP_TIMER9_IRQHandler(void) {
    // TIMER0 和 TIMER9 共享中断线。当前仅处理 TIMER0。
    _timer_isr_fire(g_port_map[_timer_index(timer_id::timer0)]);
}

extern "C" void TIMER1_IRQHandler(void) {
    _timer_isr_fire(g_port_map[_timer_index(timer_id::timer1)]);
}

extern "C" void TIMER2_IRQHandler(void) {
    _timer_isr_fire(g_port_map[_timer_index(timer_id::timer2)]);
}

extern "C" void TIMER3_IRQHandler(void) {
    _timer_isr_fire(g_port_map[_timer_index(timer_id::timer3)]);
}

extern "C" void TIMER4_IRQHandler(void) {
    _timer_isr_fire(g_port_map[_timer_index(timer_id::timer4)]);
}

#ifdef TIMER5_IRQn
extern "C" void TIMER5_IRQHandler(void) {
    _timer_isr_fire(g_port_map[_timer_index(timer_id::timer5)]);
}
#endif

extern "C" void TIMER6_IRQHandler(void) {
    _timer_isr_fire(g_port_map[_timer_index(timer_id::timer6)]);
}

extern "C" void TIMER7_UP_TIMER12_IRQHandler(void) {
    _timer_isr_fire(g_port_map[_timer_index(timer_id::timer7)]);
}
