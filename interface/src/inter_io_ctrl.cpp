#include "inter_io_ctrl.hpp"

// ============================================================
//  构造 / 析构 / 移动
// ============================================================

io_ctrl::io_ctrl(uint32_t gpio_periph, uint32_t pin)
    : _gpio_periph(gpio_periph),
      _pin(pin),
      _mode(MODE_UNSET),
      _otype(OTYPE_DEF),
      _speed(SPEED_DEF),
      _initialized(false)
{
    assert(pin != 0);
    assert((pin & (pin - 1)) == 0);   // 必须恰好 1 个 bit
}

io_ctrl::~io_ctrl()
{
    deinit();
}

io_ctrl::io_ctrl(io_ctrl &&other) noexcept
    : _gpio_periph(other._gpio_periph),
      _pin(other._pin),
      _mode(other._mode),
      _otype(other._otype),
      _speed(other._speed),
      _initialized(other._initialized)
{
    other._initialized = false;
    other._mode        = MODE_UNSET;
}

io_ctrl &io_ctrl::operator=(io_ctrl &&other) noexcept
{
    if (this != &other)
    {
        deinit();

        _gpio_periph = other._gpio_periph;
        _pin         = other._pin;
        _mode        = other._mode;
        _otype       = other._otype;
        _speed       = other._speed;
        _initialized = other._initialized;

        other._initialized = false;
        other._mode        = MODE_UNSET;
    }
    return *this;
}

// ============================================================
//  时钟使能
// ============================================================

void io_ctrl::_enable_clock()
{
    // rcu_periph_clock_enable 内部幂等
    if      (_gpio_periph == GPIOA) rcu_periph_clock_enable(RCU_GPIOA);
    else if (_gpio_periph == GPIOB) rcu_periph_clock_enable(RCU_GPIOB);
    else if (_gpio_periph == GPIOC) rcu_periph_clock_enable(RCU_GPIOC);
    else if (_gpio_periph == GPIOD) rcu_periph_clock_enable(RCU_GPIOD);
    else if (_gpio_periph == GPIOE) rcu_periph_clock_enable(RCU_GPIOE);
    else if (_gpio_periph == GPIOF) rcu_periph_clock_enable(RCU_GPIOF);
    else if (_gpio_periph == GPIOG) rcu_periph_clock_enable(RCU_GPIOG);
#ifdef GPIOH
    else if (_gpio_periph == GPIOH) rcu_periph_clock_enable(RCU_GPIOH);
#endif
    else { assert(0); }  // 非法端口
}

// ============================================================
//  配置应用
// ============================================================

void io_ctrl::_apply(uint32_t mode, uint32_t pull, uint8_t otype, uint32_t speed)
{
    gpio_mode_set(_gpio_periph, mode, pull, _pin);

    if (mode == GPIO_MODE_OUTPUT || mode == GPIO_MODE_AF)
    {
        gpio_output_options_set(_gpio_periph, otype, speed, _pin);
        _otype = otype;          // 缓存
        _speed = speed;
    }

    _mode = mode;
}

// ============================================================
//  init / reinit / deinit
// ============================================================

void io_ctrl::init(uint32_t mode, uint32_t pull, uint8_t otype, uint32_t speed)
{
    assert(!_initialized);
    if (_initialized) return;    // Release 安全

    _enable_clock();
    _apply(mode, pull, otype, speed);
    _initialized = true;
}

void io_ctrl::reinit(uint32_t mode, uint32_t pull, uint8_t otype, uint32_t speed)
{
    assert(_initialized);
    if (!_initialized) return;   // Release 安全

    _apply(mode, pull, otype, speed);
}

void io_ctrl::deinit()
{
    if (_initialized)
    {
        gpio_mode_set(_gpio_periph, GPIO_MODE_ANALOG, GPIO_PUPD_NONE, _pin);
        _initialized = false;
        _mode        = MODE_UNSET;
    }
}

// ============================================================
//  独立属性修改
// ============================================================

void io_ctrl::set_pull(uint32_t pull)
{
    if (!_check_init()) return;
    gpio_mode_set(_gpio_periph, _mode, pull, _pin);
}

void io_ctrl::set_speed(uint32_t speed)
{
    if (!_check_out()) return;
    gpio_output_options_set(_gpio_periph, _otype, speed, _pin);
    _speed = speed;
}

void io_ctrl::set_otype(uint8_t otype)
{
    if (!_check_out()) return;
    gpio_output_options_set(_gpio_periph, otype, _speed, _pin);
    _otype = otype;
}

// ============================================================
//  复用功能（放宽了 AF 模式限制） ─────────────────────────
// ============================================================

void io_ctrl::set_af(uint32_t alt_func_num)
{
    assert(_initialized);               // 时钟必须已使能
    if (!_initialized) return;

    gpio_af_set(_gpio_periph, alt_func_num, _pin);
}

// ============================================================
//  锁定
// ============================================================

void io_ctrl::lock()
{
    assert(_initialized);
    if (!_initialized) return;

    gpio_pin_lock(_gpio_periph, _pin);
}

// ============================================================
//  输出 — BOP/BC/TG 寄存器均为单次写入，硬件原子
// ============================================================

void io_ctrl::high()
{
    assert(_initialized);
    if (!_initialized) return;
    gpio_bit_set(_gpio_periph, _pin);
}

void io_ctrl::low()
{
    assert(_initialized);
    if (!_initialized) return;
    gpio_bit_reset(_gpio_periph, _pin);
}

void io_ctrl::toggle()
{
    assert(_initialized);
    if (!_initialized) return;

    // TG 寄存器：写 1 的位翻转对应引脚。
    // 这是单次总线写入，不经过读-改-写，硬件级原子，无需关中断。
    gpio_bit_toggle(_gpio_periph, _pin);
}

void io_ctrl::set(bool value)
{
    value ? high() : low();
}

void io_ctrl::write(bit_status bit_value)
{
    assert(_initialized);
    if (!_initialized) return;
    gpio_bit_write(_gpio_periph, _pin, bit_value);
}

// ============================================================
//  输入
// ============================================================

FlagStatus io_ctrl::read()
{
    if (!_check_init()) return RESET;
    return gpio_input_bit_get(_gpio_periph, _pin);
}

FlagStatus io_ctrl::read_output()
{
    assert(_check_out());
    if (!_check_out()) return RESET;
    return gpio_output_bit_get(_gpio_periph, _pin);
}
