#pragma once

#include "usart_async_port.hpp"

/** @brief Debug 串口（USART0, PA9/PA10, 115200-8N1, 256 字节 RX） */
extern usart_async_port<256> debug_uart;

#ifdef __cplusplus
extern "C" {
#endif

void DebugPort_Init(void);

#ifdef __cplusplus
}
#endif
