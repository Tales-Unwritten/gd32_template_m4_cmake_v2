#pragma once
/**
 * @file    protocol.hpp
 * @brief   多协议抽象层 —— 协议类型定义 + 公共接口
 *
 * 通过 ACTIVE_PROTOCOL 宏在 Modbus / 字符串指令间切换。
 * 协议实现分散在独立文件中:
 *   - protocol.cpp       公共入口 (Protocol_Init / Protocol_Task)
 *   - protocol_string.cpp 字符串指令 (handler / 命令表 / 解析器)
 *   - middleware/modbus/   Modbus RTU 协议栈
 */

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ══════════════════════════════════════════════════════════
 *  协议选择
 * ══════════════════════════════════════════════════════════ */
#define PROTOCOL_NONE         0
#define PROTOCOL_MODBUS       1
#define PROTOCOL_STRING_CMD   2

#ifndef ACTIVE_PROTOCOL
#define ACTIVE_PROTOCOL  PROTOCOL_MODBUS
#endif

/* ══════════════════════════════════════════════════════════
 *  公共接口（所有协议模式通用）
 * ══════════════════════════════════════════════════════════ */

void Protocol_Init(void);   // 初始化活跃协议
void Protocol_Task(void);   // 协议轮询（主循环中周期调用）

/* ══════════════════════════════════════════════════════════
 *  Modbus 辅助 —— 仅在 PROTOCOL_MODBUS 模式下可用
 * ══════════════════════════════════════════════════════════ */

#if ACTIVE_PROTOCOL == PROTOCOL_MODBUS
/** @brief 将 INA228 传感器数据刷新到 Modbus StatusBlock 寄存器 */
void UpdateModbusRegisters(void);
#endif

#ifdef __cplusplus
}
#endif
