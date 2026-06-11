#pragma once
#include "gd32f4xx.h"
#include <cstdint>
#include <cassert>

#ifdef __cplusplus

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
    explicit io_ctrl(uint32_t gpio_periph, uint32_t pin);
    ~io_ctrl();

    io_ctrl(const io_ctrl &)            = delete;
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
    void init(uint32_t mode,
              uint32_t pull  = GPIO_PUPD_NONE,
              uint8_t  otype = GPIO_OTYPE_PP,
              uint32_t speed = GPIO_OSPEED_50MHZ);

    /** @brief 重新配置已初始化引脚的模式（不必先 deinit） */
    void reinit(uint32_t mode,
                uint32_t pull  = GPIO_PUPD_NONE,
                uint8_t  otype = GPIO_OTYPE_PP,
                uint32_t speed = GPIO_OSPEED_50MHZ);

    /** @brief 反初始化，恢复为模拟输入（最低功耗），可再次 init() */
    void deinit();

    // ── 独立属性修改（仅已初始化时有效） ──────────────────

    /** @brief 修改上/下拉（输入/输出/AF 均有效） */
    void set_pull(uint32_t pull);

    /** @brief 修改输出速度（仅输出/AF 有效） */
    void set_speed(uint32_t speed);

    /** @brief 修改输出类型（仅输出/AF 有效） */
    void set_otype(uint8_t otype);

    // ── 复用功能 ──────────────────────────────────────────

    /**
     * @brief 设置复用功能编号（GPIO_AF0 ~ GPIO_AF15）
     * @note  可在 init() 之后任意时刻调用；通常搭配 AF 模式使用。
     *        若在非 AF 模式下设置，后续 reinit() 为 AF 时即刻生效。
     */
    void set_af(uint32_t alt_func_num);

    // ── 锁定 ──────────────────────────────────────────────

    /** @brief 锁定引脚配置（仅系统复位解除） */
    void lock();

    // ── 输出（BOP/BC 寄存器，硬件级原子） ─────────────────

    void high();
    void low();
    void toggle();                    // TG 寄存器写入，原子无需关中断
    void set(bool value);             // true=high, false=low
    void write(bit_status bit_value);

    // ── 输入 ──────────────────────────────────────────────

    [[nodiscard]] FlagStatus read();         // ISTAT，任意模式有效
    [[nodiscard]] FlagStatus read_output();  // OCTL，输出/AF 模式有效

    // ── 查询 ──────────────────────────────────────────────

    [[nodiscard]] bool     is_initialized() const noexcept { return _initialized; }
    [[nodiscard]] uint32_t port()           const noexcept { return _gpio_periph; }
    [[nodiscard]] uint32_t pin()            const noexcept { return _pin; }
    [[nodiscard]] uint32_t mode()           const noexcept { return _mode; }

    /** @brief 快速判初始化，支持 if(pin) 写法 */
    [[nodiscard]] explicit operator bool() const noexcept { return _initialized; }

private:
    uint32_t _gpio_periph;
    uint32_t _pin;
    uint32_t _mode;
    uint8_t  _otype;       // 缓存的输出类型，供 set_speed 保持
    uint32_t _speed;       // 缓存的输出速度，供 set_otype 保持
    bool     _initialized;

    static constexpr uint32_t MODE_UNSET = 0xFFFFFFFF;
    static constexpr uint8_t  OTYPE_DEF  = GPIO_OTYPE_PP;
    static constexpr uint32_t SPEED_DEF  = GPIO_OSPEED_50MHZ;

    void _enable_clock();
    void _apply(uint32_t mode, uint32_t pull, uint8_t otype, uint32_t speed);

    /** @brief Release 安全的状态检查（断言之外的第二道防线） */
    [[nodiscard]] bool _check_init() const noexcept { return _initialized; }
    [[nodiscard]] bool _check_out()  const noexcept { return _initialized && (_mode == GPIO_MODE_OUTPUT || _mode == GPIO_MODE_AF); }
};

#endif /* __cplusplus */
