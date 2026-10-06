#pragma once

#include "inter_usart.hpp"

extern usart_port<256> debug_uart;
extern usart_port<256> rs232_uart;
extern usart_port<256> rs485_uart;
extern io_ctrl en_485;

#ifdef __cplusplus
extern "C"
{
#endif
    void DebugPort_Init(void);
#ifdef __cplusplus
}
#endif
