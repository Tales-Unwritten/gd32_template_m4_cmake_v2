#pragma once

#include "inter_usart.hpp"
#include "inter_io_ctrl.hpp"

extern usart_port rs485_uart;
extern io_ctrl    rs485_de;

#ifdef __cplusplus
extern "C" {
#endif
void RS485Port_Init(void);
#ifdef __cplusplus
}
#endif
