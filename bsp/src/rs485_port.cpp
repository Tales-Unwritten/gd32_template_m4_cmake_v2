#include "rs485_port.hpp"

/* USART1, PA2(TX), PA3(RX), AF7, 115200, 256B RX */
usart_port rs485_uart({ USART1, GPIOA, GPIO_PIN_2, GPIOA, GPIO_PIN_3,
                        GPIO_AF_7, 115200, 256 });

io_ctrl rs485_de(GPIOG, GPIO_PIN_13);

void RS485Port_Init(void) {
    rs485_de.init(GPIO_MODE_OUTPUT, GPIO_PUPD_NONE);
    rs485_de.low();
    rs485_uart.init();
}
