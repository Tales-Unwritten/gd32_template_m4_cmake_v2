#pragma once
#include "gd32f4xx.h"
#include <cstdint>
#include <cassert>

#ifdef __cplusplus

class io_ctrl
{
public:
    /**
     * @brief  构造：绑定 GPIO 端口与引脚
     * @param  gpio_periph  GPIO 端口外设 (GPIOA ~ GPIOG)
     * @param  pin          单引脚掩码 (GPIO_PIN_0 ~ GPIO_PIN_15)
     */
    explicit io_ctrl(uint32_t gpio_periph, uint32_t pin);
    ~io_ctrl();

    // 禁止拷贝，避免意外重复管理同一引脚
    io_ctrl(const io_ctrl &) = delete;
    io_ctrl &operator=(const io_ctrl &) = delete;

    // 允许移动，转移引脚所有权
    io_ctrl(io_ctrl &&other) noexcept;
    io_ctrl &operator=(io_ctrl &&other) noexcept;

    /**
     * @brief  初始化 GPIO 引脚
     * @param  mode   GPIO_MODE_INPUT / GPIO_MODE_OUTPUT /
     *               GPIO_MODE_AF / GPIO_MODE_ANALOG
     * @param  pull   GPIO_PUPD_NONE / GPIO_PUPD_PULLUP / GPIO_PUPD_PULLDOWN
     * @param  otype  GPIO_OTYPE_PP / GPIO_OTYPE_OD（仅输出/AF模式有效）
     * @param  speed  GPIO_OSPEED_2MHZ ~ GPIO_OSPEED_MAX（仅输出/AF模式有效）
     */
    void init(uint32_t mode,
              uint32_t pull   = GPIO_PUPD_NONE,
              uint8_t  otype  = GPIO_OTYPE_PP,
              uint32_t speed  = GPIO_OSPEED_50MHZ);

    /**
     * @brief  重新配置已初始化的引脚（模式切换）
     * @note   不需要先 deinit，直接调用即可覆盖当前配置
     */
    void reinit(uint32_t mode,
                uint32_t pull   = GPIO_PUPD_NONE,
                uint8_t  otype  = GPIO_OTYPE_PP,
                uint32_t speed  = GPIO_OSPEED_50MHZ);

    /**
     * @brief  反初始化引脚，恢复为模拟输入（最低功耗态）
     * @note   调用后对象回到未初始化状态，可再次调用 init()
     */
    void deinit();

    /**
     * @brief  设置复用功能编号
     * @param  alt_func_num  GPIO_AF0 ~ GPIO_AF15
     * @note   仅在 AF 模式下有意义，其他模式下断言失败
     */
    void set_af(uint32_t alt_func_num);

    /**
     * @brief  锁定引脚配置，锁定后仅系统复位可解除
     */
    void lock();

    // ── 输出控制 ──────────────────────────────────────────
    /** @brief 置位引脚（输出高电平） */
    void high();
    /** @brief 清零引脚（输出低电平） */
    void low();
    /** @brief 翻转引脚电平（中断安全） */
    void toggle();
    /** @brief 便捷方法：true=高, false=低 */
    void set(bool value);
    /** @brief 写入指定电平 */
    void write(bit_status bit_value);

    // ── 输入读取 ──────────────────────────────────────────
    /** @brief  读取引脚实际电平（适用于任何模式） */
    [[nodiscard]] FlagStatus read();
    /** @brief  读取输出锁存器状态（仅输出/AF模式有意义） */
    [[nodiscard]] FlagStatus read_output();

    // ── 状态查询 ──────────────────────────────────────────
    [[nodiscard]] bool     is_initialized() const noexcept { return _initialized; }
    [[nodiscard]] uint32_t port()           const noexcept { return _gpio_periph; }
    [[nodiscard]] uint32_t pin()            const noexcept { return _pin; }
    [[nodiscard]] uint32_t mode()           const noexcept { return _mode; }

private:
    uint32_t _gpio_periph;
    uint32_t _pin;
    uint32_t _mode;         // 当前配置的模式，用于运行时安全检查
    bool     _initialized;

    /** @brief 未初始化时的模式占位值 */
    static constexpr uint32_t MODE_UNSET = 0xFFFFFFFF;

    /** @brief 使能端口时钟（幂等，多次调用安全） */
    void _enable_clock();

    /** @brief 应用 GPIO 配置并更新内部模式记录 */
    void _apply(uint32_t mode, uint32_t pull, uint8_t otype, uint32_t speed);
};

#endif /* __cplusplus */
