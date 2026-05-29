#pragma once

#include "usart_async_port.hpp"
#include "io_ctrl.hpp"

/** @brief RS485 串口（USART1, PA2/PA3, AF7, PG13 DE 引脚） */
extern usart_async_port<256> rs485_uart;
extern io_ctrl               rs485_de;

#ifdef __cplusplus
extern "C" {
#endif

void RS485Port_Init(void);

#ifdef __cplusplus
}
#endif
