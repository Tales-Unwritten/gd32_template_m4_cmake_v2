#include "hard_bsp_led.h"

void led_gpio_config(void)
{

  /* 使能时钟 */
	rcu_periph_clock_enable(RCU_user_led);
	rcu_periph_clock_enable(RCU_flash_led);
	rcu_periph_clock_enable(RCU_state_led);
	/* 配置为输出模式 浮空模式 */
	gpio_mode_set(PORT_user_led,GPIO_MODE_OUTPUT,GPIO_PUPD_NONE,PIN_user_led);
	gpio_mode_set(PORT_flash_led,GPIO_MODE_OUTPUT,GPIO_PUPD_NONE,PIN_flash_led);
	gpio_mode_set(PORT_state_led,GPIO_MODE_OUTPUT,GPIO_PUPD_NONE,PIN_state_led);
	/* 配置为推挽输出 50MHZ */
	gpio_output_options_set(PORT_user_led,GPIO_OTYPE_PP,GPIO_OSPEED_50MHZ,PIN_user_led);
	gpio_output_options_set(PORT_flash_led,GPIO_OTYPE_PP,GPIO_OSPEED_50MHZ,PIN_flash_led);
	gpio_output_options_set(PORT_state_led,GPIO_OTYPE_PP,GPIO_OSPEED_50MHZ,PIN_state_led);

	gpio_bit_write(PORT_user_led, PIN_user_led, SET); // user_led灭
	gpio_bit_write(PORT_flash_led, PIN_flash_led, SET); // flash_led灭
	gpio_bit_write(PORT_state_led, PIN_state_led, SET); // state_led灭

}

void led_on(uint8_t led_num)
{
	switch (led_num)
	{
	case 1:
		gpio_bit_reset(PORT_user_led, PIN_user_led); // user_led亮
		break;
	case 2:
		gpio_bit_reset(PORT_flash_led, PIN_flash_led); // flash_led亮
		break;
	case 3:
		gpio_bit_reset(PORT_state_led, PIN_state_led); // state_led亮
		break;
	default:
		break;
	}
}

void led_off(uint8_t led_num)
{
	switch (led_num)
	{
	case 1:
		gpio_bit_set(PORT_user_led, PIN_user_led); // user_led灭
		break;
	case 2:
		gpio_bit_set(PORT_flash_led, PIN_flash_led); // flash_led灭
		break;
	case 3:
		gpio_bit_set(PORT_state_led, PIN_state_led); // state_led灭
		break;
	default:
		break;
	}
}

//led状态取反

void led_toggle(uint8_t led_num)
{
	switch (led_num)
	{
	case 1:
		gpio_bit_write(PORT_user_led, PIN_user_led, (bit_status)(1 - gpio_input_bit_get(PORT_user_led, PIN_user_led))); // user_led状态取反
		break;
	case 2:
		gpio_bit_write(PORT_flash_led, PIN_flash_led, (bit_status)(1 - gpio_input_bit_get(PORT_flash_led, PIN_flash_led))); // flash_led状态取反
		break;
	case 3:
		gpio_bit_write(PORT_state_led, PIN_state_led, (bit_status)(1 - gpio_input_bit_get(PORT_state_led, PIN_state_led))); // state_led状态取反
		break;
	default:
		break;
	}
}
