#include "debug_port.hpp"

// USART0, PA9(TX), PA10(RX), AF7, 115200, 256B RX
usart_async_port<256> debug_uart(
    UsartAsyncConfig{
        usart_id::usart0,
        GPIOA, GPIO_PIN_9,
        GPIOA, GPIO_PIN_10,
        GPIO_AF_7,
        115200
    }
);

void DebugPort_Init(void)
{
    debug_uart.init();
}
