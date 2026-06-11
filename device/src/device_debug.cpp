#include "device_debug.hpp"

/* USART0, PA9(TX), PA10(RX), AF7, 115200, 256B RX */
usart_port debug_uart({ USART0, GPIOA, GPIO_PIN_9, GPIOA, GPIO_PIN_10,
                        GPIO_AF_7, 460800, 256 });

void DebugPort_Init(void) { debug_uart.init(); }
