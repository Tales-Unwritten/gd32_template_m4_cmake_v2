#pragma once

#ifdef __cplusplus

#include "usart_port.hpp"

/**
 * @brief  RS232 串口模块（USART2, PD5 TX / PD6 RX, AF7, 115200-8N1）
 *
 * 使用方式:
 *   1. 调用 RS232Port_Init() 初始化硬件
 */

extern usart_port rs232_uart;

void RS232Port_Init(void);

#endif
