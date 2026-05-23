#include "io_ctrl.hpp"

// ============================================================
//  构造 / 析构 / 移动
// ============================================================

io_ctrl::io_ctrl(uint32_t gpio_periph, uint32_t pin)
    : _gpio_periph(gpio_periph),
      _pin(pin),
      _mode(MODE_UNSET),
      _initialized(false)
{
    // pin 必须为合法的单引脚掩码（恰好 1 个 bit 置 1）
    assert(pin != 0);
    assert((pin & (pin - 1)) == 0);
}

io_ctrl::~io_ctrl()
{
    // 委托给 deinit()，统一清理逻辑，避免重复代码
    deinit();
}

io_ctrl::io_ctrl(io_ctrl &&other) noexcept
    : _gpio_periph(other._gpio_periph),
      _pin(other._pin),
      _mode(other._mode),
      _initialized(other._initialized)
{
    // 源对象进入"空壳"状态：不可使用，但可安全析构
    other._initialized = false;
    other._mode        = MODE_UNSET;
}

io_ctrl &io_ctrl::operator=(io_ctrl &&other) noexcept
{
    if (this != &other)
    {
        // 先释放当前持有的资源
        deinit();

        _gpio_periph = other._gpio_periph;
        _pin         = other._pin;
        _mode        = other._mode;
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
    // rcu_periph_clock_enable 内部有幂等保护，多次调用安全
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
    else
    {
        // 非法端口地址，绝不继续执行
        assert(0);
    }
}

// ============================================================
//  配置应用
// ============================================================

void io_ctrl::_apply(uint32_t mode, uint32_t pull, uint8_t otype, uint32_t speed)
{
    // 第一步：设置模式和上下拉（所有模式都需要）
    gpio_mode_set(_gpio_periph, mode, pull, _pin);

    // 第二步：仅输出/AF 模式需要配置输出类型和速度
    //        输入/模拟模式下此步跳过，避免无意义的寄存器写入
    if (mode == GPIO_MODE_OUTPUT || mode == GPIO_MODE_AF)
    {
        gpio_output_options_set(_gpio_periph, otype, speed, _pin);
    }

    // 同步更新内部模式记录
    _mode = mode;
}

// ============================================================
//  init / reinit / deinit
// ============================================================

void io_ctrl::init(uint32_t mode, uint32_t pull, uint8_t otype, uint32_t speed)
{
    // 防止重复初始化：一个 io_ctrl 对象只应 init 一次
    // 如需切换模式，应使用 reinit()
    assert(!_initialized);

    _enable_clock();
    _apply(mode, pull, otype, speed);
    _initialized = true;
}

void io_ctrl::reinit(uint32_t mode, uint32_t pull, uint8_t otype, uint32_t speed)
{
    assert(_initialized);
    _apply(mode, pull, otype, speed);
}

void io_ctrl::deinit()
{
    if (_initialized)
    {
        // GD32 没有 per-pin deinit 函数
        // 将引脚恢复为模拟输入 = 最低功耗 + 释放对引脚的电气驱动
        gpio_mode_set(_gpio_periph, GPIO_MODE_ANALOG, GPIO_PUPD_NONE, _pin);

        _initialized = false;
        _mode        = MODE_UNSET;
    }
}

// ============================================================
//  复用功能 / 锁定
// ============================================================

void io_ctrl::set_af(uint32_t alt_func_num)
{
    assert(_initialized);
    // AF 寄存器仅在 AF 模式下生效，其他模式下写入无意义
    assert(_mode == GPIO_MODE_AF);

    gpio_af_set(_gpio_periph, alt_func_num, _pin);
}

void io_ctrl::lock()
{
    assert(_initialized);
    gpio_pin_lock(_gpio_periph, _pin);
}

// ============================================================
//  输出控制
// ============================================================

void io_ctrl::high()
{
    assert(_initialized);

    // 统一使用库函数，保持风格一致
    // gpio_bit_set 内部为单条 BOP 寄存器写入，编译器通常会内联
    gpio_bit_set(_gpio_periph, _pin);
}

void io_ctrl::low()
{
    assert(_initialized);
    gpio_bit_reset(_gpio_periph, _pin);
}

void io_ctrl::toggle()
{
    assert(_initialized);

    // 关中断保护：防止主循环与 ISR 同时 toggle 同一引脚导致丢失翻转
    //
    // 无保护时的竞态场景：
    //   主循环读 OCTL=LOW → ISR 读 OCTL=LOW → ISR 写 BOP 置高 → 主循环写 BOP 置高
    //   结果：两次 toggle 却只翻转了一次
    //
    uint32_t primask = __get_PRIMASK();
    // __disable_irq();
    gpio_bit_toggle(_gpio_periph, _pin);
    __set_PRIMASK(primask);
}

void io_ctrl::set(bool value)
{
    // 编译器会将此优化为无分支实现（cmov 或直接位操作）
    value ? high() : low();
}

void io_ctrl::write(bit_status bit_value)
{
    assert(_initialized);
    gpio_bit_write(_gpio_periph, _pin, bit_value);
}

// ============================================================
//  输入读取
// ============================================================

FlagStatus io_ctrl::read()
{
    assert(_initialized);

    // ISTAT 寄存器读取实际引脚电平，适用于任何模式：
    //   - 输入模式：读取外部驱动的电平
    //   - 输出模式：读取引脚实际电平（开漏模式下可能与输出锁存不同）
    //   - AF 模式：读取外设驱动的电平
    return gpio_input_bit_get(_gpio_periph, _pin);
}

FlagStatus io_ctrl::read_output()
{
    assert(_initialized);
    // OCTL 仅在输出/AF 模式下有意义
    // 输入模式下 OCTL 仍可读，但反映的是输出锁存器而非引脚实际状态
    assert(_mode == GPIO_MODE_OUTPUT || _mode == GPIO_MODE_AF);

    return gpio_output_bit_get(_gpio_periph, _pin);
}
