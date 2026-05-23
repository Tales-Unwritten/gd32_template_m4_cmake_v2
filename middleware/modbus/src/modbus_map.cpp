// ============================================================
// 文件名: modbus_map.cpp
// 说  明: Modbus 寄存器映射表的定义与读写实现
//         本文件定义了所有 Modbus 可访问的内存区块，
//         以及按地址查找并读/写寄存器的核心函数。
//
// 使用指南:
//   1. 在下方数组中声明你的数据区块（设备信息/控制参数/实时数据等）
//   2. 将区块注册到 ModbusBlockTable[] 中，指定起始地址、长度、类型
//   3. 固件业务逻辑直接读写这些数组即可与 Modbus 主站交换数据
//   4. Modbus 协议层收到读/写请求后，调用本文件提供的两个函数完成寄存器访问
// ============================================================

#include "modbus_map.hpp"

// ============================================================
// 【1】数据区块定义
//     每个区块对应一段连续的 Modbus 寄存器地址空间。
//     数据类型统一为 uint16_t 数组，因为 Modbus 寄存器宽度 = 16 位。
//
//     命名建议:
//       - DeviceBlock  : 设备基本信息（固件版本、设备型号等，出厂后一般不变）
//       - ControlBlock : 控制参数（Modbus 主站下发的指令/设定值）
//       - StatusBlock  : 实时数据（设备采集的传感器值、运行状态等）
//
//     扩展方式:
//       如果需要更多区块，按同样方式定义数组，
//       然后在下方 ModbusBlockTable[] 中添加对应条目即可。
// ============================================================

/** 设备信息区（HOLDING 寄存器，可读可写）
 *  地址范围: 0x0000 ~ 0x0009（共 10 个寄存器）
 *  用途示例:
 *    [0] = 设备型号      [1] = 固件版本
 *    [2] = 硬件版本      [3] = 协议版本
 *    [4]~[9] = 保留
 *
 *  使用方式:
 *    固件初始化时填入默认值，Modbus 主站可读取；
 *    主站也可写入（如校准参数），固件侧直接读 DeviceBlock[n] 即可。
 */
uint16_t DeviceBlock[10] =
{
    1,      // [0] 设备型号，示例: 1 = 型号A
    100,    // [1] 固件版本，示例: 100 表示 v1.00
    // 其余元素默认初始化为 0，可按需赋值
};

/** 控制参数区（HOLDING 寄存器，可读可写）
 *  地址范围: 0x000A ~ 0x003B（共 50 个寄存器）
 *  用途示例:
 *    [0]     = 继电器控制命令     （如 0=断开, 1=闭合）
 *    [1]     = 电机转速设定值     （rpm）
 *    [2]~[3] = 目标温度设定值     （uint32_t，需拆成两个寄存器）
 *    [4]~[49]= 保留给其他控制参数
 *
 *  使用方式:
 *    - Modbus 主站通过写 HOLDING 寄存器将控制指令写入本区
 *    - 固件业务逻辑周期性读取 ControlBlock[n] 并执行对应操作
 *    - 例: 固件中 relay_Write(ControlBlock[0]) 即可响应继电器命令
 */
uint16_t ControlBlock[50];

/** 实时数据区（INPUT 寄存器，只读）
 *  地址范围: 0x0040 ~ 0x0071（共 50 个寄存器）
 *  用途示例:
 *    [0]~[1] = 总线电压           （uint32_t，单位 μV，大端序：[0]=高16位, [1]=低16位）
 *    [2]~[3] = 总线电流           （uint32_t，单位 μA）
 *    [4]     = 温度               （uint16_t，单位 0.1°C）
 *    [5]     = 继电器状态回读     （uint16_t，0=断开, 1=闭合）
 *    [6]~[49]= 保留给其他实时数据
 *
 *  使用方式:
 *    - 固件业务逻辑周期性采集硬件数据，填入 StatusBlock[n]
 *    - Modbus 主站通过读 INPUT 寄存器获取实时数据（只能读，不能写）
 *    - 例: StatusBlock[0] = (uint16_t)(get_bus_voltage_uv() >> 16);
 *          StatusBlock[1] = (uint16_t)(get_bus_voltage_uv() & 0xFFFF);
 */
uint16_t StatusBlock[50];


// ============================================================
// 【2】区块表结构体定义
//     每一行描述一个"地址段 ↔ 内存数组"的映射关系。
//     Modbus 协议层通过查这张表来定位寄存器对应的内存位置。
// ============================================================

typedef struct
{
    uint16_t            start_addr;  // 该区块在 Modbus 地址空间中的起始地址
    uint16_t            length;      // 该区块包含的寄存器数量
    modbus_block_type_t type;        // 寄存器类型: MODBUS_BLOCK_HOLDING 或 MODBUS_BLOCK_INPUT
    uint16_t            *data;       // 指向实际存储数据的数组

} modbus_block_t;


// ============================================================
// 【3】区块注册表（Modbus 地址映射总表）
//     所有需要被 Modbus 访问的数据区块都必须在这里注册。
//
//     格式: { 起始地址, 寄存器数量, 类型, 数据数组指针 }
//
//     注意事项:
//       - 各区块的地址范围不能重叠
//       - HOLDING 类型: 主站可读可写（用于控制参数、设备信息等）
//       - INPUT  类型: 主站只能读   （用于实时数据、传感器值等）
//       - 新增区块时，在此数组中追加一行即可
//
//     当前地址分配:
//       ┌──────────────┬───────────┬──────┬────────────────┐
//       │ 地址范围      │ 数量      │ 类型  │ 数据           │
//       ├──────────────┼───────────┼──────┼────────────────┤
//       │ 0x0000~0x0009│ 10        │HOLDING│ DeviceBlock    │
//       │ 0x000A~0x003B│ 50        │HOLDING│ ControlBlock   │
//       │ 0x0040~0x0071│ 50        │INPUT │ StatusBlock    │
//       └──────────────┴───────────┴──────┴────────────────┘
// ============================================================

modbus_block_t ModbusBlockTable[] =
{
    //  起始地址   长度   类型                    数据数组
    {   0x0000,     10,   MODBUS_BLOCK_HOLDING,   DeviceBlock   },  // 设备信息
    {   0x000A,     50,   MODBUS_BLOCK_HOLDING,   ControlBlock  },  // 控制参数
    {   0x0040,     50,   MODBUS_BLOCK_INPUT,     StatusBlock   },  // 实时数据
    // ↑↑↑ 新增区块请在此行之前追加 ↑↑↑
};

/** 区块总数（自动计算，无需手动修改） */
uint16_t BlockCount = sizeof(ModbusBlockTable) / sizeof(modbus_block_t);


// ============================================================
// 【4】Modbus 读寄存器函数
//
// 函数原型: uint16_t Modbus_ReadRegister(uint16_t addr, modbus_block_type_t type)
//
// 功能说明:
//   根据寄存器地址和类型，从对应的区块中读取一个 uint16_t 值。
//   HOLDING 和 INPUT 类型的寄存器都可以读取。
//
// 参数说明:
//   addr - 要读取的 Modbus 寄存器地址（如 0x0040）
//   type - 寄存器类型（MODBUS_BLOCK_HOLDING 或 MODBUS_BLOCK_INPUT）
//
// 返回值:
//   成功 → 对应地址的 uint16_t 数据
//   失败（地址无效或类型不匹配）→ 返回 0
//
// 典型调用场景:
//   ① Modbus 协议层收到主站的"读保持寄存器"请求时:
//      uint16_t val = Modbus_ReadRegister(0x000A, MODBUS_BLOCK_HOLDING);
//
//   ② Modbus 协议层收到主站的"读输入寄存器"请求时:
//      uint16_t val = Modbus_ReadRegister(0x0040, MODBUS_BLOCK_INPUT);
//
//   ③ 固件内部读取某个寄存器的值:
//      uint16_t val = Modbus_ReadRegister(0x000A, MODBUS_BLOCK_HOLDING);
//
//   ④ 读取一个 uint32_t 数据（需要读两个连续寄存器并拼接）:
//      uint16_t high = Modbus_ReadRegister(0x0040, MODBUS_BLOCK_INPUT);
//      uint16_t low  = Modbus_ReadRegister(0x0041, MODBUS_BLOCK_INPUT);
//      uint32_t voltage_uv = ((uint32_t)high << 16) | (uint32_t)low;
// ============================================================

uint16_t Modbus_ReadRegister(uint16_t addr, modbus_block_type_t type)
{
    // 遍历区块表，查找匹配的区块
    for (uint16_t i = 0; i < BlockCount; i++)
    {
        modbus_block_t *block = &ModbusBlockTable[i];

        // 第一步: 类型过滤 —— 只匹配指定类型的区块
        //   如果主站请求读 INPUT 寄存器，就跳过所有 HOLDING 区块，反之亦然
        if (block->type != type)
        {
            continue;
        }

        // 第二步: 地址范围检查 —— 判断目标地址是否落在当前区块内
        //   例如 addr=0x0041, block={0x0040, 50}
        //   → 0x0041 >= 0x0040 ✓ 且 0x0041 < 0x0040+50=0x0072 ✓
        if (addr >= block->start_addr && addr < block->start_addr + block->length)
        {
            // 第三步: 计算数组下标并返回数据
            //   index = 目标地址 - 区块起始地址
            //   例如 addr=0x0041, start_addr=0x0040 → index=1 → StatusBlock[1]
            uint16_t index = addr - block->start_addr;
            return block->data[index];
        }
    }

    // 未找到匹配的区块（地址无效或类型不对），返回 0
    return 0;
}


// ============================================================
// 【5】Modbus 写寄存器函数
//
// 函数原型: void Modbus_WriteRegister(uint16_t addr, uint16_t value)
//
// 功能说明:
//   根据寄存器地址，将一个 uint16_t 值写入对应的 HOLDING 区块。
//   ⚠ 只能写 HOLDING 类型的寄存器！INPUT 类型会被跳过（只读保护）。
//
// 参数说明:
//   addr  - 要写入的 Modbus 寄存器地址（如 0x000A）
//   value - 要写入的 uint16_t 值
//
// 典型调用场景:
//   ① Modbus 协议层收到主站的"写单个寄存器"请求时:
//      Modbus_WriteRegister(0x000A, 1);   // 写入继电器控制命令
//
//   ② Modbus 协议层收到主站的"写多个寄存器"请求时，循环调用:
//      Modbus_WriteRegister(0x000A, 1);
//      Modbus_WriteRegister(0x000B, 3000); // 写入电机转速设定
//
//   ③ 写入一个 uint32_t 数据（需要拆成两个寄存器分别写入）:
//      uint32_t target_temp = 50000;  // 目标温度 50.000°C (单位 0.001°C)
//      Modbus_WriteRegister(0x000C, (uint16_t)(target_temp >> 16));    // 高16位
//      Modbus_WriteRegister(0x000D, (uint16_t)(target_temp & 0xFFFF)); // 低16位
//
// ⚠ 注意: 写入 INPUT 类型寄存器（如 StatusBlock）会被静默忽略，
//    这是设计上的只读保护，INPUT 区只能由固件内部写入。
// ============================================================

void Modbus_WriteRegister(uint16_t addr, uint16_t value)
{
    // 遍历区块表，查找匹配的 HOLDING 区块
    for (uint16_t i = 0; i < BlockCount; i++)
    {
        modbus_block_t *block = &ModbusBlockTable[i];

        // 关键保护: 跳过所有非 HOLDING 类型的区块
        //   INPUT 类型的寄存器（如 StatusBlock）不允许 Modbus 主站写入，
        //   只能由固件内部直接赋值 StatusBlock[n] = xxx 来更新。
        if (block->type != MODBUS_BLOCK_HOLDING)
        {
            continue;
        }

        // 地址范围检查 —— 与读寄存器逻辑相同
        if (addr >= block->start_addr && addr < block->start_addr + block->length)
        {
            // 计算下标并写入数据
            uint16_t index = addr - block->start_addr;
            block->data[index] = value;
            return;  // 写入成功，立即返回
        }
    }
    // 未找到匹配的区块，静默忽略（地址无效）
}
