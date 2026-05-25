#include "debug_port.hpp"

/* ── 串口实例定义 ──────────────────────────────────────── */

usart_port debug_uart({ USART0, GPIOA, GPIO_PIN_9, GPIOA, GPIO_PIN_10,
                        GPIO_AF_7, 115200, 256 });

/* ── 初始化 ────────────────────────────────────────────── */

void DebugPort_Init(void)
{
    debug_uart.init();
}
