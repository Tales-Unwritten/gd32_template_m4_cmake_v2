// ============================================================
// 文件名: modbus_map.hpp
// 说  明: Modbus 寄存器映射模块的头文件
//         声明了寄存器类型枚举和读/写函数原型。
//
// 使用方式:
//   在需要访问 Modbus 寄存器的源文件中:
//     #include "modbus_map.hpp"
//
//   然后直接调用:
//     uint16_t val = Modbus_ReadRegister(addr, type);
//     Modbus_WriteRegister(addr, value);
// ============================================================

#pragma once
#include <stdint.h>

#ifdef __cplusplus

// ============================================================
// 【1】Modbus 区块类型枚举
//
//   MODBUS_BLOCK_HOLDING : 保持寄存器（Holding Register）
//       → 可读可写，用于设备信息、控制参数等
//       → 对应 Modbus 功能码 0x03（读）和 0x06/0x10（写）
//
//   MODBUS_BLOCK_INPUT   : 输入寄存器（Input Register）
//       → 只读，用于实时数据、传感器采集值等
//       → 对应 Modbus 功能码 0x04（读），无写功能码
//
// 扩展说明:
//   如果需要支持线圈（Coil）或离散输入（Discrete Input），
//   可在此枚举中追加:
//     MODBUS_BLOCK_COIL,
//     MODBUS_BLOCK_DISCRETE_INPUT,
//   并在 modbus_map.cpp 中增加对应的处理逻辑。
// ============================================================

typedef enum
{
    MODBUS_BLOCK_HOLDING, // 保持寄存器块（可读可写）
    MODBUS_BLOCK_INPUT    // 输入寄存器块（只读）
} modbus_block_type_t;


// ============================================================
// 【2】函数声明
// ============================================================

// ------------------------------------------------------------
// Modbus_ReadRegister —— 读取单个寄存器的值
//
// 调用示例:
//   // 读取设备型号（HOLDING 寄存器，地址 0x0000）
//   uint16_t model = Modbus_ReadRegister(0x0000, MODBUS_BLOCK_HOLDING);
//
//   // 读取总线电压高 16 位（INPUT 寄存器，地址 0x0040）
//   uint16_t v_high = Modbus_ReadRegister(0x0040, MODBUS_BLOCK_INPUT);
//
//   // 读取总线电压低 16 位（INPUT 寄存器，地址 0x0041）
//   uint16_t v_low  = Modbus_ReadRegister(0x0041, MODBUS_BLOCK_INPUT);
//
//   // 拼接为 uint32_t
//   uint32_t voltage_uv = ((uint32_t)v_high << 16) | (uint32_t)v_low;
//
// 返回值: 成功返回寄存器值，失败返回 0
// ------------------------------------------------------------
uint16_t Modbus_ReadRegister(uint16_t addr, modbus_block_type_t type);

// ------------------------------------------------------------
// Modbus_WriteRegister —— 写入单个寄存器的值（仅限 HOLDING 类型）
//
// 调用示例:
//   // 写入继电器控制命令（HOLDING 寄存器，地址 0x000A）
//   Modbus_WriteRegister(0x000A, 1);   // 1 = 闭合
//   Modbus_WriteRegister(0x000A, 0);   // 0 = 断开
//
//   // 写入 uint32_t 目标温度（拆成两个寄存器，地址 0x000C~0x000D）
//   uint32_t target = 50000;
//   Modbus_WriteRegister(0x000C, (uint16_t)(target >> 16));     // 高16位
//   Modbus_WriteRegister(0x000D, (uint16_t)(target & 0xFFFF));  // 低16位
//
// ⚠ 对 INPUT 类型寄存器调用此函数会被静默忽略（只读保护）
// ------------------------------------------------------------
void Modbus_WriteRegister(uint16_t addr, uint16_t value);

#endif //__cplusplus
