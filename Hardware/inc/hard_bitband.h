/* bitband.h */
#ifndef __BITBAND_H__
#define __BITBAND_H__

#include "gd32f4xx.h"
#include "systick.h"
#include <stdint.h>
#include <stdbool.h>

#define PERIPH_BASE     0x40000000U
#define BITBAND_BASE    0x42000000U
#define GPIO_PIN_MAX    15

#ifdef __cplusplus
extern "C" {
#endif

// 设置 GPIO 输出位为 1
void bitband_gpio_set(uint32_t gpio_periph, uint32_t pin);

// 清除 GPIO 输出位为 0
void bitband_gpio_clear(uint32_t gpio_periph, uint32_t pin);

// 写 GPIO 输出位，value = 0 或 1
void bitband_gpio_write(uint32_t gpio_periph, uint32_t pin, bool value);

// 读取 GPIO 输入位状态（高电平返回 true）
bool bitband_gpio_read(uint32_t gpio_periph, uint32_t pin);

void bitband_gpio_toggle(uint32_t gpio_periph, uint32_t pin);

//测试编写的函数是否正确
void test_bitband_gpio_functions(void);

#ifdef __cplusplus
}
#endif

#endif /* __BITBAND_H__ */