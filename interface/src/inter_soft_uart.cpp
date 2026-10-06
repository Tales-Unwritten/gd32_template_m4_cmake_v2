// ============================================================
//  inter_soft_uart.cpp
//
//  soft_uart_port<RX_BUF_SIZE> 已模板化：其全部成员函数定义
//  （构造/析构、init/deinit、putc/puts/write、try_getc/getc、
//   set_baud/poll 及基类接口）均内联于 inter_soft_uart.hpp，
//  以保证模板定义对调用点可见。
//
//  软串口无 ISR、无全局端口表，故本编译单元不产生任何非模板符号，
//  仅保留占位以维持构建系统中的源文件条目。
// ============================================================

#include "inter_soft_uart.hpp"
