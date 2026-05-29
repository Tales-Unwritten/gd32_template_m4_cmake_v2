/**
 * @file    modbus_port_cfg.cpp
 * @brief   Modbus 端口配置 —— 平台胶水层
 *
 * 本文件是 modbus 协议栈唯一与具体硬件/平台耦合的文件。
 * 移植到新平台或纯 C 项目时，只需重写此文件：
 *   - 替换 include 为对应平台的串口驱动头文件
 *   - 调整端口注册表中的实例和发送逻辑
 *
 * 其余 modbus_*.cpp 文件为纯协议核心，无需修改。
 */

/* ── 端口使能宏 ────────────────────────────────────────── */
/*
 * 按需定义/注释以启用或禁用端口。
 * 默认启用 debug 端口（USART0）。
 */
#define MODBUS_PORT_DEBUG
// #define MODBUS_PORT_RS485
// #define MODBUS_PORT_RS232

/* ── 平台串口驱动 ──────────────────────────────────────── */

#ifdef MODBUS_PORT_DEBUG
#include "debug_port.hpp"
#endif

#ifdef MODBUS_PORT_RS485
#include "rs485_port.hpp"
#endif

#ifdef MODBUS_PORT_RS232
#include "rs232_port.hpp"
#endif

#include "modbus.hpp"
#include "usart_async_port.hpp"

/* ── 多端口注册表 ──────────────────────────────────────── */
/*
 * 发送逻辑通过 captureless lambda 内联。
 * lambda 访问全局对象无需捕获 → 编译器隐式转换为 C 函数指针，零开销。
 *
 * debug / RS232 : 中断发送 (send_data_it)
 * RS485         : 阻塞发送 + 自动 DE 方向控制
 *
 * 移植到纯 C 项目时，将 lambda 替换为 C 函数指针:
 *   void Debug_Send_C(uint8_t *b, uint16_t l) { ... }
 *   { debug_uart.buffer(), Debug_Send_C },
 */

const modbus_port_t modbus_ports[] = {
#ifdef MODBUS_PORT_DEBUG
    {
        debug_uart.buffer(),
        [](uint8_t *b, uint16_t l) { debug_uart.send_data_it(b, l); }
    },
#endif
#ifdef MODBUS_PORT_RS485
    {
        rs485_uart.buffer(),
        [](uint8_t *b, uint16_t l) {
            rs485_de.low();
            rs485_uart.send_data(b, l);
            rs485_de.high();
        }
    },
#endif
#ifdef MODBUS_PORT_RS232
    {
        rs232_uart.buffer(),
        [](uint8_t *b, uint16_t l) { rs232_uart.send_data_it(b, l); }
    },
#endif
};

const uint8_t modbus_port_count = sizeof(modbus_ports) / sizeof(modbus_ports[0]);
