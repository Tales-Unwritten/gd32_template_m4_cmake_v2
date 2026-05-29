#include "founction.hpp"
#include "usart_async_port.hpp"
#include "debug_port.hpp"
#include "rs485_port.hpp"
#include "rs232_port.hpp"
#include "protocol.hpp"
#include "pc_task.hpp"
#include "device_i2c_bus.hpp"
#include "device_i2c_dev.hpp"
#include "gd32f4xx_gpio.h"
#include "i2c_test_simple.hpp"
#include "ina226.hpp"
#include "ina228.hpp"
#include "w25q128_flash.hpp"
#include "timer_port.hpp"
#include "io_ctrl.hpp"
#include "systick.h"
#include <cstring>
#include <stdio.h>

// debug_uart 定义已迁移至 bsp/src/debug_port.cpp

device_i2c_bus i2c_bus({GPIOE, GPIOE, GPIO_PIN_2, GPIO_PIN_3}, 1);
// device_i2c_bus i2c_bus1({GPIOG, GPIOG, GPIO_PIN_4, GPIO_PIN_5}, 2);


// INA228 INA228_1(&i2c_bus, INA228::ADDR_0x40);
INA228 INA228_2(&i2c_bus, INA228::ADDR_0x40);

// INA226 INA226_1(&i2c_bus, INA226::ADDR_0x40);

io_ctrl led1(GPIOC, GPIO_PIN_4);
io_ctrl led2(GPIOC, GPIO_PIN_5);
io_ctrl led3(GPIOA, GPIO_PIN_7);

// ── SPI 验证测试：W25Q128 Flash（SPI4, GPIOF PF6~PF9） ───
w25q128_flash g_flash;

// ── 定时器验证：TIMER2 1Hz 中断翻转 LED3 ──────────────────
timer_port g_tick_timer({
    timer_id::timer2, 59999, 999, {}, 0
});

void _on_timer_tick(void *) {
    led3.toggle();
}

void function_init(void) {
  i2c_bus.i2c_init();
  // i2c_bus1.i2c_init();

  // INA228_1.init();
  INA228_2.init();
  // INA226_1.init();
  // INA226_1.dumpRegisters();

  led1.init(GPIO_MODE_OUTPUT, GPIO_PUPD_NONE, GPIO_OTYPE_PP, GPIO_OSPEED_50MHZ);
  led2.init(GPIO_MODE_OUTPUT, GPIO_PUPD_NONE, GPIO_OTYPE_PP, GPIO_OSPEED_50MHZ);
  led3.init(GPIO_MODE_OUTPUT, GPIO_PUPD_NONE, GPIO_OTYPE_PP, GPIO_OSPEED_50MHZ);

  DebugPort_Init();     // 初始化调试串口
  // RS485Port_Init();   // 初始化 RS485 串口（需要时取消注释）
  // RS232Port_Init();   // 初始化 RS232 串口（需要时取消注释）

  Protocol_Init();       // 初始化当前活跃协议（Modbus / 字符串指令）

  // ── SPI 验证：读取 W25Q128 Flash JEDEC ID ────────────
  g_flash.init();
  uint32_t flash_id = g_flash.read_jedec_id();
  bool pass = (flash_id == 0xEF7018);
  const char *result = pass ? "PASS" : "FAIL";
  debug_uart.send_data((const uint8_t *)"\r\n[SPI_TEST] JEDEC ID: ", 22);
  for (int8_t n = 5; n >= 0; n--) {
      uint8_t hex = (flash_id >> (n * 4)) & 0x0F;
      uint8_t ch = hex < 10 ? '0' + hex : 'A' + (hex - 10);
      debug_uart.send_data(&ch, 1);
  }
  debug_uart.send_data((const uint8_t *)" ", 1);
  debug_uart.send_data((const uint8_t *)result, pass ? 4 : 4);
  debug_uart.send_data((const uint8_t *)"\r\n", 2);

  // ── 定时器验证（临时禁用，排查 ISR 冲突） ────────────
  // g_tick_timer.init();
  // g_tick_timer.on_timeout(_on_timer_tick, nullptr);
  // g_tick_timer.start();
}

/* ============================================================
 *  UpdateModbusRegisters —— INA228 → Modbus StatusBlock
 *
 *  迁移原因: 此函数属于"应用层业务逻辑"（传感器值 → 寄存器映射），
 *           INA228_2 定义在本文件，放在此处避免 protocol.cpp 反向依赖
 *           应用层头文件（ina228.hpp）。protocol.cpp 保持纯粹的分发职责。
 * ============================================================ */
#if ACTIVE_PROTOCOL == PROTOCOL_MODBUS
#include "modbus_map.hpp"

/* StatusBlock 布局索引（与 modbus_map.cpp 中定义一致） */
enum {
    REG_BUSV_HI  = 0,  REG_BUSV_LO  = 1,   // Bus Voltage   (μV)   uint32
    REG_SHUNTV_HI= 2,  REG_SHUNTV_LO= 3,   // Shunt Voltage (μV)   uint32
    REG_CUR_HI   = 4,  REG_CUR_LO   = 5,   // Current       (mA)   uint32
    REG_PWR_HI   = 6,  REG_PWR_LO   = 7,   // Power         (mW)   uint32
    REG_TEMP_HI  = 8,  REG_TEMP_LO  = 9,   // Temperature   (m°C)  uint32
    REG_ENERGY_3 =10,  REG_ENERGY_2 =11,   // Energy        (mJ)   uint64
    REG_ENERGY_1 =12,  REG_ENERGY_0 =13,
    REG_CHARGE_3 =14,  REG_CHARGE_2 =15,   // Charge        (mC)   uint64
    REG_CHARGE_1 =16,  REG_CHARGE_0 =17,
    REG_MFR_ID   =18,                      // Manufacturer ID
    REG_DEV_ID   =19,                      // Device ID
    REG_ALERT    =20,                      // Alert Flags
};

void UpdateModbusRegisters(void)
{
    extern uint16_t StatusBlock[];  // 定义在 middleware/modbus/src/modbus_map.cpp

    int32_t vm = INA228_2.getBusVoltage_mV();
    int32_t sv = INA228_2.getShuntVoltage_uV();
    int32_t im = INA228_2.getCurrent_mA();
    int32_t pm = INA228_2.getPower_mW();
    int32_t tm = INA228_2.getTemperature_mC();
    int64_t em = INA228_2.getEnergy_mJ();
    int64_t cm = INA228_2.getCharge_mC();

    int64_t vu = (int64_t)vm * 1000;  // mV → μV，大端双寄存器存储

    /* 大端序写入 — 高 16 位在低地址，符合 Modbus 规范 */
    StatusBlock[REG_BUSV_HI]   = (uint16_t)((uint64_t)vu >> 16);
    StatusBlock[REG_BUSV_LO]   = (uint16_t)(vu);
    StatusBlock[REG_SHUNTV_HI] = (uint16_t)((uint32_t)sv >> 16);
    StatusBlock[REG_SHUNTV_LO] = (uint16_t)(sv);
    StatusBlock[REG_CUR_HI]    = (uint16_t)((uint32_t)im >> 16);
    StatusBlock[REG_CUR_LO]    = (uint16_t)(im);
    StatusBlock[REG_PWR_HI]    = (uint16_t)((uint32_t)pm >> 16);
    StatusBlock[REG_PWR_LO]    = (uint16_t)(pm);
    StatusBlock[REG_TEMP_HI]   = (uint16_t)((uint32_t)tm >> 16);
    StatusBlock[REG_TEMP_LO]   = (uint16_t)(tm);
    StatusBlock[REG_ENERGY_3]  = (uint16_t)((uint64_t)em >> 48);
    StatusBlock[REG_ENERGY_2]  = (uint16_t)((uint64_t)em >> 32);
    StatusBlock[REG_ENERGY_1]  = (uint16_t)((uint64_t)em >> 16);
    StatusBlock[REG_ENERGY_0]  = (uint16_t)(em);
    StatusBlock[REG_CHARGE_3]  = (uint16_t)((uint64_t)cm >> 48);
    StatusBlock[REG_CHARGE_2]  = (uint16_t)((uint64_t)cm >> 32);
    StatusBlock[REG_CHARGE_1]  = (uint16_t)((uint64_t)cm >> 16);
    StatusBlock[REG_CHARGE_0]  = (uint16_t)(cm);
    StatusBlock[REG_MFR_ID]    = INA228_2.getManufacturerID();
    StatusBlock[REG_DEV_ID]    = INA228_2.getDeviceID();
    StatusBlock[REG_ALERT]     = INA228_2.readDiagAlertRaw();
}
#endif /* ACTIVE_PROTOCOL == PROTOCOL_MODBUS */

void function_loop(void) {
  APP_PC_Task();         // 协议轮询

#if ACTIVE_PROTOCOL == PROTOCOL_MODBUS
  UpdateModbusRegisters();  // INA228 传感器数据刷新到 Modbus 寄存器
#endif

  led1.toggle();
  led2.toggle();
  led3.toggle();

  delay_ms(10);           // 保持主循环节拍，避免外设异常
}
