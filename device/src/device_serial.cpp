#include "device_serial.hpp"

/* USART0, PA9(TX), PA10(RX), AF7, 115200, 256B RX */
usart_port<256> debug_uart({USART0, portA, pin9, portA, pin10, af7, 115200});

/* USART2, PD5(TX), PD6(RX), AF7, 115200, 256B RX */
usart_port<256> rs232_uart({USART2, portD, pin5, portD, pin6, af7, 115200});

/* USART1, PA2(TX), PA3(RX), AF7, 115200, 256B RX */
usart_port<256> rs485_uart({USART1, portA, pin2, portA, pin3, af7, 115200});

io_ctrl en_485(portG, pin13);

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
