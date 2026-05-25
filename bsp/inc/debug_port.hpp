#pragma once

#ifdef __cplusplus

#include "usart_port.hpp"

/**
 * @brief  Debug 串口模块（USART0, PA9 TX / PA10 RX, 115200-8N1）
 *
 * 使用方式:
 *   1. 调用 DebugPort_Init() 初始化硬件
 *   2. 调试输出直接使用 debug_uart.send_data(...)
 */

extern usart_port debug_uart;

void DebugPort_Init(void);

#endif
