/**
 * @file    protocol.cpp
 * @brief   多协议公共入口 —— 初始化 & 任务分发
 *
 * 协议切换: 修改 protocol.hpp 中 ACTIVE_PROTOCOL 宏
 *   PROTOCOL_MODBUS      → Modbus RTU 从机
 *   PROTOCOL_STRING_CMD  → ASCII 字符串指令（实现见 protocol_string.cpp）
 *
 * 数据流:
 *   USART ISR → uart_buffer_t.rx_flag = 1
 *     → Protocol_Task() 根据 ACTIVE_PROTOCOL 分发
 *       → Modbus_Task()     (middleware/modbus/)
 *       → StringCmd_Task()  (protocol_string.cpp)
 */

#include "protocol.hpp"
#include "debug_port.hpp"
#include "ina228.hpp"

#if ACTIVE_PROTOCOL == PROTOCOL_MODBUS
#include "modbus.hpp"
#endif

#if ACTIVE_PROTOCOL == PROTOCOL_STRING_CMD
/* 前向声明，实现在 protocol_string.cpp */
void StringCmd_Task(void);
#endif

extern INA228 INA228_2;  // 定义在 user/src/founction.cpp

/* ============================================================
 *  公共接口
 * ============================================================ */

void Protocol_Init(void)
{
#if ACTIVE_PROTOCOL == PROTOCOL_MODBUS
    Modbus_Init();
#endif
}

void Protocol_Task(void)
{
#if ACTIVE_PROTOCOL == PROTOCOL_MODBUS
    Modbus_Task();
#elif ACTIVE_PROTOCOL == PROTOCOL_STRING_CMD
    StringCmd_Task();
#endif
}

/* ============================================================
 *  UpdateModbusRegisters —— INA228 → Modbus StatusBlock
 * ============================================================ */

#if ACTIVE_PROTOCOL == PROTOCOL_MODBUS

/* StatusBlock 布局索引（与 modbus_map.cpp 中定义一致） */
enum {
    REG_BUSV_HI = 0,  REG_BUSV_LO = 1,     // Bus Voltage  (μV)  uint32
    REG_SHUNTV_HI=2,  REG_SHUNTV_LO=3,     // Shunt Voltage (μV)  uint32
    REG_CUR_HI  =4,   REG_CUR_LO  =5,      // Current       (mA)  uint32
    REG_PWR_HI  =6,   REG_PWR_LO  =7,      // Power         (mW)  uint32
    REG_TEMP_HI =8,   REG_TEMP_LO =9,      // Temperature   (m°C) uint32
    REG_ENERGY_3=10,  REG_ENERGY_2=11,     // Energy (mJ) uint64
    REG_ENERGY_1=12,  REG_ENERGY_0=13,
    REG_CHARGE_3=14,  REG_CHARGE_2=15,     // Charge (mC) uint64
    REG_CHARGE_1=16,  REG_CHARGE_0=17,
    REG_MFR_ID  =18,                       // Manufacturer ID
    REG_DEV_ID  =19,                       // Device ID
    REG_ALERT   =20,                       // Alert Flags
    STATUSBLOCK_COUNT = 21                 // 已使用寄存器数
};

void UpdateModbusRegisters(void)
{
    extern uint16_t StatusBlock[];  // 定义在 modbus_map.cpp

    int32_t vm = INA228_2.getBusVoltage_mV();
    int32_t sv = INA228_2.getShuntVoltage_uV();
    int32_t im = INA228_2.getCurrent_mA();
    int32_t pm = INA228_2.getPower_mW();
    int32_t tm = INA228_2.getTemperature_mC();
    int64_t em = INA228_2.getEnergy_mJ();
    int64_t cm = INA228_2.getCharge_mC();

    int64_t vu = (int64_t)vm * 1000;  // mV → μV

    /* 大端序写入 — 高位在前，Modbus 标准字节序 */
    StatusBlock[REG_BUSV_HI]  = (uint16_t)((uint64_t)vu >> 16);
    StatusBlock[REG_BUSV_LO]  = (uint16_t)(vu);
    StatusBlock[REG_SHUNTV_HI]= (uint16_t)((uint32_t)sv >> 16);
    StatusBlock[REG_SHUNTV_LO]= (uint16_t)(sv);
    StatusBlock[REG_CUR_HI]   = (uint16_t)((uint32_t)im >> 16);
    StatusBlock[REG_CUR_LO]   = (uint16_t)(im);
    StatusBlock[REG_PWR_HI]   = (uint16_t)((uint32_t)pm >> 16);
    StatusBlock[REG_PWR_LO]   = (uint16_t)(pm);
    StatusBlock[REG_TEMP_HI]  = (uint16_t)((uint32_t)tm >> 16);
    StatusBlock[REG_TEMP_LO]  = (uint16_t)(tm);
    StatusBlock[REG_ENERGY_3] = (uint16_t)((uint64_t)em >> 48);
    StatusBlock[REG_ENERGY_2] = (uint16_t)((uint64_t)em >> 32);
    StatusBlock[REG_ENERGY_1] = (uint16_t)((uint64_t)em >> 16);
    StatusBlock[REG_ENERGY_0] = (uint16_t)(em);
    StatusBlock[REG_CHARGE_3] = (uint16_t)((uint64_t)cm >> 48);
    StatusBlock[REG_CHARGE_2] = (uint16_t)((uint64_t)cm >> 32);
    StatusBlock[REG_CHARGE_1] = (uint16_t)((uint64_t)cm >> 16);
    StatusBlock[REG_CHARGE_0] = (uint16_t)(cm);
    StatusBlock[REG_MFR_ID]   = INA228_2.getManufacturerID();
    StatusBlock[REG_DEV_ID]   = INA228_2.getDeviceID();
    StatusBlock[REG_ALERT]    = INA228_2.readDiagAlertRaw();
}

#endif /* MODBUS */
