#pragma once
// ============================================================
// @platform GD32F4xx
// ============================================================

#ifdef __cplusplus

#include <cstdint>
#include "io_ctrl.hpp"
#include "gd32f4xx.h"

enum class exti_trigger : uint8_t { rising, falling, both };

struct ExtiConfig {
    uint32_t       port;       ///< GPIOA~GPIOI
    uint32_t       pin;        ///< GPIO_PIN_0 ~ GPIO_PIN_15
    exti_trigger   trigger;    ///< rising / falling / both
};

/**
 * @brief 外部中断（EXTI）
 *
 *   static exti_port btn({GPIOA, GPIO_PIN_0, exti_trigger::rising});
 *   btn.init();
 *   btn.on_interrupt([](void*) { led.toggle(); });
 *   btn.enable();
 */
class exti_port {
public:
    using callback_t = void (*)(void *user_data);

    explicit exti_port(const ExtiConfig &cfg);
    ~exti_port();

    exti_port(const exti_port &) = delete;
    exti_port &operator=(const exti_port &) = delete;

    void init();
    void deinit();
    [[nodiscard]] bool is_initialized() const noexcept { return _initialized; }

    void on_interrupt(callback_t cb, void *data = nullptr);
    void enable();
    void disable();

    // ISR 访问（friend）
    friend void _exti_fire(exti_port *p);

private:
    void      _config_syscfg();
    uint8_t   _pin_index() const;
    IRQn_Type _get_irq() const;
    void      _register_isr();
    void      _unregister_isr();

    io_ctrl     _pin;
    ExtiConfig  _cfg;
    callback_t  _cb;
    void       *_cb_data;
    bool        _initialized;
    uint8_t     _line;       // EXTI 线号 0~15
};

#endif
