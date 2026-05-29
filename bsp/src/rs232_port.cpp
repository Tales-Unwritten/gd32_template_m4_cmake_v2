#include "rs232_port.hpp"

// USART2, PD5(TX), PD6(RX), AF7, 115200, 256B RX
usart_async_port<256> rs232_uart(
    UsartAsyncConfig{
        usart_id::usart2,
        GPIOD, GPIO_PIN_5,
        GPIOD, GPIO_PIN_6,
        GPIO_AF_7,
        115200
    }
);

void RS232Port_Init(void)
{
    rs232_uart.init();
}
