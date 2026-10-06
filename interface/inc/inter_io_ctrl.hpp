#pragma once
#include "gd32f4xx.h"
#include "gd32f4xx_gpio.h"
#include <cassert>
#include <cstdint>

#ifdef __cplusplus


enum polarity : uint8_t
{
    Low  = RESET,
    High = !RESET,
    active_low  = Low,   // 语义别名：低有效（如 CS 选通信号）
    active_high = High,  // 语义别名：高有效
};

enum port_enum_t : uint32_t
{
    port_none = 0,   // 哨兵：未绑定端口（0 与任何真实 GPIO 基址均不同）
    portA = GPIOA,
    portB = GPIOB,
    portC = GPIOC,
    portD = GPIOD,
    portE = GPIOE,
    portF = GPIOF,
    portG = GPIOG,
    portH = GPIOH,
    portI = GPIOI,
};

enum pin_enum_t : uint16_t
{
    pin_none = 0,
    pin0   = GPIO_PIN_0,
    pin1   = GPIO_PIN_1,
    pin2   = GPIO_PIN_2,
    pin3   = GPIO_PIN_3,
    pin4   = GPIO_PIN_4,
    pin5   = GPIO_PIN_5,
    pin6   = GPIO_PIN_6,
    pin7   = GPIO_PIN_7,
    pin8   = GPIO_PIN_8,
    pin9   = GPIO_PIN_9,
    pin10  = GPIO_PIN_10,
    pin11  = GPIO_PIN_11,
    pin12  = GPIO_PIN_12,
    pin13  = GPIO_PIN_13,
    pin14  = GPIO_PIN_14,
    pin15  = GPIO_PIN_15,
    pinall = GPIO_PIN_ALL,
};

enum pull_enum_t :uint32_t
{
    nopull  =GPIO_PUPD_NONE,
    pullup  =GPIO_PUPD_PULLUP,
    pulldown=GPIO_PUPD_PULLDOWN,
};

enum mode_enum_t:uint32_t
{
    mode_none   = 0xFFFFFFFF,
    mode_input  = GPIO_MODE_INPUT,
    mode_output = GPIO_MODE_OUTPUT,
    mode_af     = GPIO_MODE_AF,
    mode_analog = GPIO_MODE_ANALOG,
};

enum speed_enum_t : uint32_t
{
    speed_low    = GPIO_OSPEED_2MHZ,
    speed_medium = GPIO_OSPEED_25MHZ,
    speed_high   = GPIO_OSPEED_50MHZ,
    speed_max    = GPIO_OSPEED_MAX,

};

enum otype_enum_t: uint32_t
{
  otype_pp = GPIO_OTYPE_PP,
  otype_od = GPIO_OTYPE_OD,
};

enum af_enum_t :uint32_t
{
    af0 = GPIO_AF_0,
    af1 = GPIO_AF_1,
    af2 = GPIO_AF_2,
    af3 = GPIO_AF_3,
    af4 = GPIO_AF_4,
    af5 = GPIO_AF_5,
    af6 = GPIO_AF_6,
    af7 = GPIO_AF_7,
    af8 = GPIO_AF_8,
    af9 = GPIO_AF_9,
    af10= GPIO_AF_10,
    af11= GPIO_AF_11,
    af12= GPIO_AF_12,
    af13= GPIO_AF_13,
    af14= GPIO_AF_14,
    af15= GPIO_AF_15,
};

/**
 * @brief  GPIO 单引脚管理类（非模板、零堆分配）
 *
 * - 构造时绑定端口+引脚，init() 后可用
 * - 禁止拷贝，允许移动（移动后源对象失效）
 * - 所有位操作通过 BOP/BC/TG 寄存器，硬件级原子
 * - 断言仅在 Debug 生效；Release 下通过返回值/早期返回保证安全
 */
class io_ctrl
{
  public:
    explicit io_ctrl(port_enum_t gpio_periph, pin_enum_t pin);
    ~io_ctrl();

    io_ctrl(const io_ctrl &) = delete;
    io_ctrl &operator=(const io_ctrl &) = delete;

    io_ctrl(io_ctrl &&other) noexcept;
    io_ctrl &operator=(io_ctrl &&other) noexcept;

    // ── 生命周期 ──────────────────────────────────────────

    /**
     * @brief 初始化 GPIO 引脚（调用前必须未初始化）
     * @param mode   GPIO_MODE_INPUT / OUTPUT / AF / ANALOG
     * @param pull   GPIO_PUPD_NONE / PULLUP / PULLDOWN
     * @param otype  GPIO_OTYPE_PP / OD（仅输出/AF 有效）
     * @param speed  GPIO_OSPEED_2MHZ ~ MAX（仅输出/AF 有效）
     */
    void init(mode_enum_t mode, pull_enum_t pull = nopull, otype_enum_t otype = otype_pp,
              speed_enum_t speed = speed_high);

    /** @brief 重新配置已初始化引脚的模式（不必先 deinit） */
    void reinit(mode_enum_t mode, pull_enum_t pull = nopull, otype_enum_t otype = otype_pp,
                speed_enum_t speed = speed_high);

    /** @brief 反初始化，恢复为模拟输入（最低功耗），可再次 init() */
    void deinit();

    // ── 独立属性修改（仅已初始化时有效） ──────────────────

    /** @brief 修改上/下拉（输入/输出/AF 均有效） */
    void set_pull(pull_enum_t pull);

    /** @brief 修改输出速度（仅输出/AF 有效） */
    void set_speed(speed_enum_t speed);

    /** @brief 修改输出类型（仅输出/AF 有效） */
    void set_otype(otype_enum_t otype);

    // ── 复用功能 ──────────────────────────────────────────

    /**
     * @brief 设置复用功能编号（GPIO_AF0 ~ GPIO_AF15）
     * @note  可在 init() 之后任意时刻调用；通常搭配 AF 模式使用。
     *        若在非 AF 模式下设置，后续 reinit() 为 AF 时即刻生效。
     */
    void set_af(af_enum_t alt_func_num);

    // ── 锁定 ──────────────────────────────────────────────

    /** @brief 锁定引脚配置（仅系统复位解除） */
    void lock();

    // ── 输出（BOP/BC 寄存器，硬件级原子） ─────────────────

    void high();
    void low();
    void toggle();        // TG 寄存器写入，原子无需关中断
    void set(bool value); // true=high, false=low
    void write(polarity bit_value);

    // ── 输入 ──────────────────────────────────────────────

    [[nodiscard]] polarity read();        // ISTAT，任意模式有效
    [[nodiscard]] polarity read_output(); // OCTL，输出/AF 模式有效

    // ── 查询 ──────────────────────────────────────────────

    [[nodiscard]] bool is_initialized() const noexcept
    {
        return _initialized;
    }
    [[nodiscard]] port_enum_t port() const noexcept
    {
        return _gpio_periph;
    }
    [[nodiscard]] pin_enum_t pin() const noexcept
    {
        return _pin;
    }
   [[nodiscard]] mode_enum_t mode() const noexcept
    {
        return _mode;
    }

    /** @brief 快速判初始化，支持 if(pin) 写法 */
    [[nodiscard]] explicit operator bool() const noexcept
    {
        return _initialized;
    }

  private:
    port_enum_t _gpio_periph;
    pin_enum_t _pin;
    mode_enum_t _mode;
    otype_enum_t _otype;  // 缓存的输出类型，供 set_speed 保持
    speed_enum_t _speed; // 缓存的输出速度，供 set_otype 保持
    bool _initialized;

    static constexpr mode_enum_t  MODE_UNSET = mode_none;
    static constexpr otype_enum_t OTYPE_DEF = otype_pp;
    static constexpr speed_enum_t SPEED_DEF = speed_high;

    void _enable_clock();
    void _apply(mode_enum_t mode, pull_enum_t pull, otype_enum_t otype, speed_enum_t speed);

    /** @brief Release 安全的状态检查（断言之外的第二道防线） */
    [[nodiscard]] bool _check_init() const noexcept
    {
        return _initialized;
    }
    [[nodiscard]] bool _check_out() const noexcept
    {
        return _initialized && (_mode == GPIO_MODE_OUTPUT || _mode == GPIO_MODE_AF);
    }
};

#endif /* __cplusplus */
