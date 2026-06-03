#pragma once

#include "usart_port.hpp"

extern usart_port rs232_uart;

#ifdef __cplusplus
extern "C" {
#endif
void RS232Port_Init(void);
#ifdef __cplusplus
}
#endif
