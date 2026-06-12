#include "../Hardware/inc/dip_switch.h"

#if (DIP_SWITCH_4_8BIT)
#define bit_num 8
static const rcu_periph_enum dip_periph_clock[] = {
	DIP_PERIPH_CLOCK0,
	DIP_PERIPH_CLOCK1,
	DIP_PERIPH_CLOCK2,
	DIP_PERIPH_CLOCK3,
	DIP_PERIPH_CLOCK4,
	DIP_PERIPH_CLOCK5,
	DIP_PERIPH_CLOCK6,
	DIP_PERIPH_CLOCK7};

static const uint32_t dip_gpio_port[] = {
	DIP_GPIO_PORT0,
	DIP_GPIO_PORT1,
	DIP_GPIO_PORT2,
	DIP_GPIO_PORT3,
	DIP_GPIO_PORT4,
	DIP_GPIO_PORT5,
	DIP_GPIO_PORT6,
	DIP_GPIO_PORT7};

static const uint32_t DIP_gpio_pin[] = {
	DIP_GPIO_PIN0,
	DIP_GPIO_PIN1,
	DIP_GPIO_PIN2,
	DIP_GPIO_PIN3,
	DIP_GPIO_PIN4,
	DIP_GPIO_PIN5,
	DIP_GPIO_PIN6,
	DIP_GPIO_PIN7};
#else
#define bit_num 4
static const rcu_periph_enum dip_periph_clock[] = {
	DIP_PERIPH_CLOCK0,
	DIP_PERIPH_CLOCK1,
	DIP_PERIPH_CLOCK2,
	DIP_PERIPH_CLOCK3};
static const uint32_t dip_gpio_port[] = {
	DIP_GPIO_PORT0,
	DIP_GPIO_PORT1,
	DIP_GPIO_PORT2,
	DIP_GPIO_PORT3};
static const uint32_t DIP_gpio_pin[] = {
	DIP_GPIO_PIN0,
	DIP_GPIO_PIN1,
	DIP_GPIO_PIN2,
	DIP_GPIO_PIN3};

#endif // DIP_SWITCH_4_8BIT

void dip_switch_init(void)
{

	for (size_t i = 0; i < sizeof(dip_periph_clock) / sizeof(dip_periph_clock[0]); i++)
	{
		rcu_periph_clock_enable(dip_periph_clock[i]); // 使能每个 GPIOB 引脚所在的端口时钟
	}

	for (size_t i = 0; i < sizeof(dip_gpio_port) / sizeof(dip_gpio_port[0]); i++)
	{
		gpio_mode_set(dip_gpio_port[i], GPIO_MODE_INPUT, GPIO_PUPD_NONE, DIP_gpio_pin[i]);
	}
}

uint8_t dip_switch_read(void)
{
	uint8_t val = 0;

	for (uint8_t i = 0; i < bit_num; ++i)
	{
		// 读取 GPIOB 的第 i 位，如果为高电平（拨到 OFF），就视为 1
		if (gpio_input_bit_get(dip_gpio_port[i], (DIP_gpio_pin[i])) == SET)
		{
			val |= (1 << i); // 设置结果的第 i 位为1
		}
	}
	return val; // 返回地址，4位最大0x0F，8位最大0xFF
}

void verify_dip_switch(void)
{
	dip_switch_init(); // 初始化拨码开关

	uint8_t switch_value = dip_switch_read(); // 读取拨码开关的值

	// 打印拨码开关的值
	printf("DIP Switch Value: 0x%02X\n", switch_value);
}
