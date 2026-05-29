#include "rs485_port.hpp"

// USART1, PA2(TX), PA3(RX), AF7, 115200, 256B RX
usart_async_port<256> rs485_uart(
    UsartAsyncConfig{
        usart_id::usart1,
        GPIOA, GPIO_PIN_2,
        GPIOA, GPIO_PIN_3,
        GPIO_AF_7,
        115200
    }
);

// RS485 方向控制引脚 (DE/RE), PG13
io_ctrl rs485_de(GPIOG, GPIO_PIN_13);

void RS485Port_Init(void)
{
    rs485_de.init(GPIO_MODE_OUTPUT, GPIO_PUPD_NONE);
    rs485_de.low();  // 默认接收模式
    rs485_uart.init();
}
