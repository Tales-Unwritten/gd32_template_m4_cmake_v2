#include "device_serial.hpp"

/* USART0, PA9(TX), PA10(RX), AF7, 115200, 256B RX */
usart_port debug_uart({USART0, GPIOA, GPIO_PIN_9, GPIOA, GPIO_PIN_10, GPIO_AF_7, 115200, 256});

/* USART2, PD5(TX), PD6(RX), AF7, 115200, 256B RX */
usart_port rs232_uart({USART2, GPIOD, GPIO_PIN_5, GPIOD, GPIO_PIN_6, GPIO_AF_7, 115200, 256});

/* USART1, PA2(TX), PA3(RX), AF7, 115200, 256B RX */
usart_port rs485_uart({USART1, GPIOA, GPIO_PIN_2, GPIOA, GPIO_PIN_3, GPIO_AF_7, 115200, 256});

io_ctrl en_485(GPIOG, GPIO_PIN_13);

void DebugPort_Init(void)
{
    debug_uart.init();
}

void RS485Port_Init(void)
{
    rs485_uart.init();
}

void RS232Port_Init(void)
{
    rs232_uart.init();
}
