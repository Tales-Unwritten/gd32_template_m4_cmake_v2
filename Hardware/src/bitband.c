/* bitband.c */
#include "../Hardware/inc/bitband.h"



#define IS_VALID_GPIO_PERIPH(p) \
    ((p) == GPIOA || (p) == GPIOB || (p) == GPIOC || (p) == GPIOD || \
     (p) == GPIOE || (p) == GPIOF || (p) == GPIOG || (p) == GPIOH)

#define BITBAND_ALIAS(addr, bitnum) \
    ((volatile uint32_t*)(BITBAND_BASE + (((uint32_t)(addr) - PERIPH_BASE) * 32U) + ((bitnum) * 4U)))

void bitband_gpio_set(uint32_t gpio_periph, uint32_t pin) {
    if (IS_VALID_GPIO_PERIPH(gpio_periph) && pin <= GPIO_PIN_MAX) {
        *BITBAND_ALIAS((uint32_t*)(gpio_periph + 0x14), pin) = 1; // ODR
    }
}

void bitband_gpio_clear(uint32_t gpio_periph, uint32_t pin) {
    if (IS_VALID_GPIO_PERIPH(gpio_periph) && pin <= GPIO_PIN_MAX) {
        *BITBAND_ALIAS((uint32_t*)(gpio_periph + 0x14), pin) = 0; // ODR
    }
}

void bitband_gpio_write(uint32_t gpio_periph, uint32_t pin, bool value) {
    if (IS_VALID_GPIO_PERIPH(gpio_periph) && pin <= GPIO_PIN_MAX) {
        *BITBAND_ALIAS((uint32_t*)(gpio_periph + 0x14), pin) = value ? 1 : 0; // ODR
    }
}

bool bitband_gpio_read(uint32_t gpio_periph, uint32_t pin)
{
    if (IS_VALID_GPIO_PERIPH(gpio_periph) && pin <= GPIO_PIN_MAX)
    {
        return (*BITBAND_ALIAS((uint32_t *)(gpio_periph + 0x10), pin) & 0x1); // IDR
    }
    return false;
}

void bitband_gpio_toggle(uint32_t gpio_periph, uint32_t pin)
{
    if (IS_VALID_GPIO_PERIPH(gpio_periph) && pin <= GPIO_PIN_MAX)
    {
        *BITBAND_ALIAS((uint32_t *)(gpio_periph + 0x2C), pin) ^= 1; // BSRR
    }
}
//测试编写的函数是否正确，并把测试的结果用通过或者不通过打印出来
void test_bitband_gpio_functions(void)
{

    // 测试 GPIOA 的第 0 位
    uint32_t gpio_periph = GPIOC;
    uint32_t pin = 5;

    // 设置 GPIOA 第 0 位
    bitband_gpio_set(gpio_periph, pin);
    if (bitband_gpio_read(gpio_periph, pin) == RESET) {
        printf("GPIOA Pin 5 Set: Passed\n");
    } else {
        printf("GPIOA Pin 5 Set: Failed\n");
    }
    delay_ms(1000); // 延时1秒
    // 清除 GPIOA 第 0 位
    bitband_gpio_clear(gpio_periph, pin);
    if (bitband_gpio_read(gpio_periph, pin) == SET) {
        printf("GPIOA Pin 5 Clear: Passed\n");
    } else {
        printf("GPIOA Pin 5 Clear: Failed\n");
    }
    delay_ms(1000); // 延时1秒
    // 切换 GPIOA 第 0 位
    // bitband_gpio_toggle(gpio_periph, pin);
    // if (bitband_gpio_read(gpio_periph, pin) == SET) {
    //     printf("GPIOA Pin 0 Toggle: Passed\n");
    // } else {
    //     printf("GPIOA Pin 0 Toggle: Failed\n");
    // }
    // delay_ms(1000); // 延时1秒
}
