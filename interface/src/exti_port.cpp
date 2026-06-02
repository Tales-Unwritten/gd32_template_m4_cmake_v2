// ============================================================
// @platform GD32F4xx（当前平台）
// ============================================================

#include "exti_port.hpp"

#include "gd32f4xx.h"
#include "gd32f4xx_rcu.h"
#include "gd32f4xx_exti.h"
#include "gd32f4xx_syscfg.h"
#include "gd32f4xx_misc.h"

// ════════════════════════════════════════════════════════════
//  全局端口映射表（EXIT 0~15）
// ════════════════════════════════════════════════════════════

namespace {
exti_port *g_exti_map[16] = {};
}

void _exti_fire(exti_port *p)
{
    if (!p || !p->is_initialized()) return;

    // 检查并清除中断标志
    exti_line_enum line = (exti_line_enum)((uint32_t)EXTI_0 << p->_line);
    if (exti_interrupt_flag_get(line) == SET) {
        exti_interrupt_flag_clear(line);

        if (p->_cb) {
            p->_cb(p->_cb_data);
        }
    }
}

// ════════════════════════════════════════════════════════════
//  构造 / 析构
// ════════════════════════════════════════════════════════════

exti_port::exti_port(const ExtiConfig &cfg)
    : _pin(cfg.port, cfg.pin)
    , _cfg(cfg)
    , _cb(nullptr), _cb_data(nullptr)
    , _initialized(false)
    , _line(0)
{}

exti_port::~exti_port() { deinit(); }

// ════════════════════════════════════════════════════════════
//  引脚索引 → EXTI 线号
// ════════════════════════════════════════════════════════════

uint8_t exti_port::_pin_index() const
{
    if (_cfg.pin == 0) return 0xFF;  // 无效引脚

    // pin 是一个 bit，找到它是第几位
    uint32_t p = _cfg.pin;
    uint8_t n = 0;
    while (p > 1) { p >>= 1; n++; }
    return n;
}

// ════════════════════════════════════════════════════════════
//  SYSCFG 端口选择
// ════════════════════════════════════════════════════════════

void exti_port::_config_syscfg()
{
    rcu_periph_clock_enable(RCU_SYSCFG);

    // 端口 → EXTI_SOURCE_GPIOx
    uint8_t port_src = 0;
    if      (_cfg.port == GPIOA) port_src = EXTI_SOURCE_GPIOA;
    else if (_cfg.port == GPIOB) port_src = EXTI_SOURCE_GPIOB;
    else if (_cfg.port == GPIOC) port_src = EXTI_SOURCE_GPIOC;
    else if (_cfg.port == GPIOD) port_src = EXTI_SOURCE_GPIOD;
    else if (_cfg.port == GPIOE) port_src = EXTI_SOURCE_GPIOE;
    else if (_cfg.port == GPIOF) port_src = EXTI_SOURCE_GPIOF;
    else if (_cfg.port == GPIOG) port_src = EXTI_SOURCE_GPIOG;
    else if (_cfg.port == GPIOH) port_src = EXTI_SOURCE_GPIOH;
    else if (_cfg.port == GPIOI) port_src = EXTI_SOURCE_GPIOI;

    uint8_t pin_src = _line;  // EXTI_SOURCE_PIN0 ~ PIN15 = 0~15
    syscfg_exti_line_config(port_src, pin_src);
}

// ════════════════════════════════════════════════════════════
//  IRQ 映射
// ════════════════════════════════════════════════════════════

IRQn_Type exti_port::_get_irq() const
{
    if (_line == 0) return EXTI0_IRQn;
    if (_line == 1) return EXTI1_IRQn;
    if (_line == 2) return EXTI2_IRQn;
    if (_line == 3) return EXTI3_IRQn;
    if (_line == 4) return EXTI4_IRQn;
    if (_line <= 9) return EXTI5_9_IRQn;
    return EXTI10_15_IRQn;
}

// ════════════════════════════════════════════════════════════
//  ISR 注册
// ════════════════════════════════════════════════════════════

void exti_port::_register_isr()  { g_exti_map[_line] = this; }
void exti_port::_unregister_isr() { g_exti_map[_line] = nullptr; }

// ════════════════════════════════════════════════════════════
//  init / deinit
// ════════════════════════════════════════════════════════════

void exti_port::init()
{
    if (_initialized) return;

    _line = _pin_index();
    if (_line > 15) return;

    // GPIO：输入模式
    _pin.init(GPIO_MODE_INPUT, GPIO_PUPD_NONE);

    // SYSCFG 选择端口
    _config_syscfg();

    // EXTI 触发配置
    exti_trig_type_enum trig;
    switch (_cfg.trigger) {
        case exti_trigger::rising:  trig = EXTI_TRIG_RISING;  break;
        case exti_trigger::falling: trig = EXTI_TRIG_FALLING; break;
        default:                     trig = EXTI_TRIG_BOTH;    break;
    }
    exti_line_enum line = (exti_line_enum)((uint32_t)EXTI_0 << _line);
    exti_init(line, EXTI_INTERRUPT, trig);
    exti_interrupt_enable(line);
    exti_interrupt_flag_clear(line);

    nvic_irq_enable(_get_irq(), 0, 0);
    _register_isr();
    _initialized = true;
}

void exti_port::deinit()
{
    if (!_initialized) return;

    exti_line_enum line = (exti_line_enum)((uint32_t)EXTI_0 << _line);
    exti_interrupt_disable(line);
    nvic_irq_disable(_get_irq());
    _unregister_isr();

    _pin.deinit();
    _cb = nullptr;
    _initialized = false;
}

// ════════════════════════════════════════════════════════════
//  回调 + 使能
// ════════════════════════════════════════════════════════════

void exti_port::on_interrupt(callback_t cb, void *data)
{
    // 关中断保护：防止 ISR 在两次写入之间触发，拿到不一致的 cb + data
    uint32_t mask = __get_PRIMASK();
    __disable_irq();
    _cb = cb;
    _cb_data = data;
    __set_PRIMASK(mask);
}

void exti_port::enable()  { if (_initialized) nvic_irq_enable(_get_irq(), 0, 0); }
void exti_port::disable() { if (_initialized) nvic_irq_disable(_get_irq()); }

// ════════════════════════════════════════════════════════════
//  ISR 入口
// ════════════════════════════════════════════════════════════

extern "C" void EXTI0_IRQHandler(void)  { _exti_fire(g_exti_map[0]); }
extern "C" void EXTI1_IRQHandler(void)  { _exti_fire(g_exti_map[1]); }
extern "C" void EXTI2_IRQHandler(void)  { _exti_fire(g_exti_map[2]); }
extern "C" void EXTI3_IRQHandler(void)  { _exti_fire(g_exti_map[3]); }
extern "C" void EXTI4_IRQHandler(void)  { _exti_fire(g_exti_map[4]); }

extern "C" void EXTI5_9_IRQHandler(void) {
    for (uint8_t i = 5; i <= 9; i++) _exti_fire(g_exti_map[i]);
}

extern "C" void EXTI10_15_IRQHandler(void) {
    for (uint8_t i = 10; i <= 15; i++) _exti_fire(g_exti_map[i]);
}
