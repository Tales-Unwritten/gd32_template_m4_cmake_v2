#pragma once

#ifdef __cplusplus

#include "usart_port.hpp"
#include "io_ctrl.hpp"

/**
 * @brief  RS485 串口模块（USART1, PA2 TX / PA3 RX, AF7, 115200-8N1）
 *
 * 方向控制引脚: PG13（高电平 = 发送，低电平 = 接收）
 *
 * 使用方式:
 *   1. 调用 RS485Port_Init() 初始化硬件（含方向控制引脚）
 *   2. Modbus 发送在 middleware/modbus 层通过 lambda 内联完成，
 *      自动处理 DE 方向切换，无需外部包装函数
 */

extern usart_port rs485_uart;
extern io_ctrl    rs485_de;      // DE/RE 方向控制引脚

void RS485Port_Init(void);

#endif
