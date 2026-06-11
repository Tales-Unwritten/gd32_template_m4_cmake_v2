#pragma once

#include "inter_usart.hpp"

extern usart_port debug_uart;

#ifdef __cplusplus
extern "C" {
#endif
void DebugPort_Init(void);
#ifdef __cplusplus
}
#endif
