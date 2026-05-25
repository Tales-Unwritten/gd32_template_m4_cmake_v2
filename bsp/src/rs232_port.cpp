#include "rs232_port.hpp"

/* ── 串口实例定义 ──────────────────────────────────────── */

usart_port rs232_uart({ USART2, GPIOD, GPIO_PIN_5, GPIOD, GPIO_PIN_6,
                        GPIO_AF_7, 115200, 256 });

/* ── 初始化 ────────────────────────────────────────────── */

void RS232Port_Init(void)
{
    rs232_uart.init();
}
