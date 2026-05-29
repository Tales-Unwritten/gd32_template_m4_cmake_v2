#pragma once

#include "usart_async_port.hpp"

/** @brief RS232 串口（USART2, PD5/PD6, AF7, 115200, 256 字节 RX） */
extern usart_async_port<256> rs232_uart;

#ifdef __cplusplus
extern "C" {
#endif

void RS232Port_Init(void);

#ifdef __cplusplus
}
#endif
