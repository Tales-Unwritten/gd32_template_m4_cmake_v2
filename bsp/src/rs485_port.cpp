#include "rs485_port.hpp"

/* ── 串口实例定义 ──────────────────────────────────────── */

usart_port rs485_uart({ USART1, GPIOA, GPIO_PIN_2, GPIOA, GPIO_PIN_3,
                        GPIO_AF_7, 115200, 256 });

/* RS485 方向控制引脚: PG13, 高电平 = 发送, 低电平 = 接收 */
io_ctrl rs485_de(GPIOG, GPIO_PIN_13);

/* ── 初始化 ────────────────────────────────────────────── */

void RS485Port_Init(void)
{
    /* 方向控制引脚初始化为推挽输出，默认低电平（接收模式） */
    rs485_de.init(GPIO_MODE_OUTPUT, GPIO_PUPD_NONE, GPIO_OTYPE_PP, GPIO_OSPEED_50MHZ);
    rs485_de.low();

    /* 初始化串口外设 */
    rs485_uart.init();
}
