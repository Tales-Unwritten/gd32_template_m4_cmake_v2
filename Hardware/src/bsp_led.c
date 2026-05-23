#include "bsp_led.h"

void led_gpio_config(void)
{
	
  /* 使能时钟 */
	rcu_periph_clock_enable(RCU_LED1);
	rcu_periph_clock_enable(RCU_LED2);
	rcu_periph_clock_enable(RCU_LED3);
	/* 配置为输出模式 浮空模式 */
	gpio_mode_set(PORT_LED1,GPIO_MODE_OUTPUT,GPIO_PUPD_NONE,PIN_LED1);
	gpio_mode_set(PORT_LED2,GPIO_MODE_OUTPUT,GPIO_PUPD_NONE,PIN_LED2);
	gpio_mode_set(PORT_LED3,GPIO_MODE_OUTPUT,GPIO_PUPD_NONE,PIN_LED3);
	/* 配置为推挽输出 50MHZ */
	gpio_output_options_set(PORT_LED1,GPIO_OTYPE_PP,GPIO_OSPEED_50MHZ,PIN_LED1);
	gpio_output_options_set(PORT_LED2,GPIO_OTYPE_PP,GPIO_OSPEED_50MHZ,PIN_LED2);
	gpio_output_options_set(PORT_LED3,GPIO_OTYPE_PP,GPIO_OSPEED_50MHZ,PIN_LED3);

	gpio_bit_write(PORT_LED1, PIN_LED1, SET); // LED1灭
	gpio_bit_write(PORT_LED2, PIN_LED2, SET); // LED2灭
	gpio_bit_write(PORT_LED3, PIN_LED3, SET); // LED3灭

}

void led_on(uint8_t led_num)
{
	switch (led_num)
	{
	case 1:
		gpio_bit_reset(PORT_LED1, PIN_LED1); // LED1亮
		break;
	case 2:
		gpio_bit_reset(PORT_LED2, PIN_LED2); // LED2亮
		break;
	case 3:
		gpio_bit_reset(PORT_LED3, PIN_LED3); // LED3亮
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
		gpio_bit_set(PORT_LED1, PIN_LED1); // LED1灭
		break;
	case 2:
		gpio_bit_set(PORT_LED2, PIN_LED2); // LED2灭
		break;
	case 3:
		gpio_bit_set(PORT_LED3, PIN_LED3); // LED3灭
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
		gpio_bit_write(PORT_LED1, PIN_LED1, (bit_status)(1 - gpio_input_bit_get(PORT_LED1, PIN_LED1))); // LED1状态取反
		break;
	case 2:
		gpio_bit_write(PORT_LED2, PIN_LED2, (bit_status)(1 - gpio_input_bit_get(PORT_LED2, PIN_LED2))); // LED2状态取反
		break;
	case 3:
		gpio_bit_write(PORT_LED3, PIN_LED3, (bit_status)(1 - gpio_input_bit_get(PORT_LED3, PIN_LED3))); // LED3状态取反
		break;
	default:
		break;
	}
}
