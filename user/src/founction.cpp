#include "founction.hpp"
#include "usart_async_port.hpp"
#include "debug_port.hpp"
#include "protocol.hpp"
#include "pc_task.hpp"
#include "device_i2c_bus.hpp"
#include "device_i2c_dev.hpp"
#include "gd32f4xx_gpio.h"
#include "ina228.hpp"
#include "io_ctrl.hpp"
#include "w25q128_flash.hpp"
#include "timer_port.hpp"
#include "flash_port.hpp"
#include "systick.h"
#include <cstring>
#include <stdio.h>

device_i2c_bus i2c_bus({GPIOE, GPIOE, GPIO_PIN_2, GPIO_PIN_3}, 1);

INA228 INA228_2(&i2c_bus, INA228::ADDR_0x40);

io_ctrl led1(GPIOC, GPIO_PIN_4);
io_ctrl led2(GPIOC, GPIO_PIN_5);
io_ctrl led3(GPIOA, GPIO_PIN_7);

// ── W25Q128 Flash（SPI4, GPIOF PF6~PF9） ──────────────────
w25q128_flash g_flash;

// ── 定时器：1Hz 中断翻转 LED3（PA7） ──────────────────────
timer_port g_tick_timer({
    timer_id::timer2, 59999, 999, {}, 0
});

void _on_timer_tick(void *) {
    led3.toggle();
}

// ════════════════════════════════════════════════════════════
//  function_init — 系统初始化（仅功能代码，不含测试）
// ════════════════════════════════════════════════════════════

void function_init(void) {
  DebugPort_Init();

  i2c_bus.i2c_init();
  INA228_2.init();

  led1.init(GPIO_MODE_OUTPUT, GPIO_PUPD_NONE, GPIO_OTYPE_PP, GPIO_OSPEED_50MHZ);
  led2.init(GPIO_MODE_OUTPUT, GPIO_PUPD_NONE, GPIO_OTYPE_PP, GPIO_OSPEED_50MHZ);
  led3.init(GPIO_MODE_OUTPUT, GPIO_PUPD_NONE, GPIO_OTYPE_PP, GPIO_OSPEED_50MHZ);

  Protocol_Init();

  // SPI Flash 自检
  g_flash.init();
  uint32_t flash_id = g_flash.read_jedec_id();
  bool pass = (flash_id == 0xEF7018);
  const char *result = pass ? "OK" : "FAIL";
  debug_uart.send_data((const uint8_t *)"\r\n[BOOT] Flash ", 14);
  for (int8_t n = 5; n >= 0; n--) {
      uint8_t hex = (flash_id >> (n * 4)) & 0x0F;
      uint8_t ch = hex < 10 ? '0' + hex : 'A' + (hex - 10);
      debug_uart.send_data(&ch, 1);
  }
  debug_uart.send_data((const uint8_t *)" ", 1);
  debug_uart.send_data((const uint8_t *)result, pass ? 2 : 4);
  debug_uart.send_data((const uint8_t *)"\r\n", 2);

  // 定时器：1Hz LED 闪烁
  g_tick_timer.init();
  g_tick_timer.on_timeout(_on_timer_tick, nullptr);
  g_tick_timer.start();

  // ── Flash 安全测试（0x08040000, 256B 安全区） ────
  // 原理：Flash 写前必须擦除（擦后全 0xFF），读-擦-写-读验证
  {
      uint32_t test_addr = 0x08040000;
      flash_port::init(test_addr, 256);

      // Step 1: 读擦除前的值
      uint8_t before = flash_port::read_byte(test_addr);

      // Step 2: 擦除
      bool er_ok = flash_port::erase(test_addr);

      // Step 3: 读擦除后的值（应为 0xFF）
      uint8_t after_erase = flash_port::read_byte(test_addr);

      // Step 4: 写测试值
      bool wr_ok = flash_port::write_byte(test_addr, 0xA5);

      // Step 5: 读回
      uint8_t written = flash_port::read_byte(test_addr);

      // Step 6: 4-byte 模式测试
      const char *pattern = "FLSH";
      bool wr4_ok = flash_port::write_bytes(test_addr + 4, (const uint8_t *)pattern, 4);
      char readback[5] = {};
      flash_port::read_bytes(test_addr + 4, (uint8_t *)readback, 4);

      flash_port::lock();

      // 输出结果
      debug_uart.send_data((const uint8_t *)"\r\n[FLASH] before=", 16);
      debug_uart.send_data(&before, 1);  // raw byte
      debug_uart.send_data((const uint8_t *)" erase=", 7);
      uint8_t er_ch = er_ok ? 'Y' : 'N';
      debug_uart.send_data(&er_ch, 1);
      debug_uart.send_data((const uint8_t *)" erased=", 8);
      debug_uart.send_data(&after_erase, 1);
      debug_uart.send_data((const uint8_t *)" write=", 7);
      uint8_t wr_ch = wr_ok ? 'Y' : 'N';
      debug_uart.send_data(&wr_ch, 1);
      debug_uart.send_data((const uint8_t *)" value=0x", 9);
      for (int8_t n = 1; n >= 0; n--) {
          uint8_t h = (written >> (n*4)) & 0x0F;
          uint8_t c = h < 10 ? '0'+h : 'A'+(h-10);
          debug_uart.send_data(&c, 1);
      }
      bool pat_ok = (readback[0]=='F' && readback[1]=='L' && readback[2]=='S' && readback[3]=='H');
      debug_uart.send_data((const uint8_t *)" pattern=", 9);
      debug_uart.send_data((const uint8_t *)readback, 4);
      debug_uart.send_data((const uint8_t *)(pat_ok ? " OK" : " FAIL"), pat_ok ? 3 : 5);
      debug_uart.send_data((const uint8_t *)"\r\n", 2);
  }
}

// ════════════════════════════════════════════════════════════
//  UpdateModbusRegisters — INA228 → Modbus StatusBlock
// ════════════════════════════════════════════════════════════
#if ACTIVE_PROTOCOL == PROTOCOL_MODBUS
#include "modbus_map.hpp"

enum {
    REG_BUSV_HI  = 0,  REG_BUSV_LO  = 1,
    REG_SHUNTV_HI= 2,  REG_SHUNTV_LO= 3,
    REG_CUR_HI   = 4,  REG_CUR_LO   = 5,
    REG_PWR_HI   = 6,  REG_PWR_LO   = 7,
    REG_TEMP_HI  = 8,  REG_TEMP_LO  = 9,
    REG_ENERGY_3 =10,  REG_ENERGY_2 =11,
    REG_ENERGY_1 =12,  REG_ENERGY_0 =13,
    REG_CHARGE_3 =14,  REG_CHARGE_2 =15,
    REG_CHARGE_1 =16,  REG_CHARGE_0 =17,
    REG_MFR_ID   =18,  REG_DEV_ID   =19,
    REG_ALERT    =20,
};

void UpdateModbusRegisters(void)
{
    extern uint16_t StatusBlock[];
    int32_t vm = INA228_2.getBusVoltage_mV();
    int32_t sv = INA228_2.getShuntVoltage_uV();
    int32_t im = INA228_2.getCurrent_mA();
    int32_t pm = INA228_2.getPower_mW();
    int32_t tm = INA228_2.getTemperature_mC();
    int64_t em = INA228_2.getEnergy_mJ();
    int64_t cm = INA228_2.getCharge_mC();
    int64_t vu = (int64_t)vm * 1000;
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
#endif

// ════════════════════════════════════════════════════════════
//  function_loop — 主循环
// ════════════════════════════════════════════════════════════

void function_loop(void) {
  APP_PC_Task();
#if ACTIVE_PROTOCOL == PROTOCOL_MODBUS
  UpdateModbusRegisters();
#endif
  led1.toggle();
  led2.toggle();
  delay_ms(10);
}
