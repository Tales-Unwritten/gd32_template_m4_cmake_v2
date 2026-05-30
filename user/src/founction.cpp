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
#include "spi_port.hpp"
#include "w25q128.hpp"
#include "timer_port.hpp"
#include "can_port.hpp"
#include "systick.h"
#include <cstring>
#include <stdio.h>

device_i2c_bus i2c_bus({GPIOE, GPIOE, GPIO_PIN_2, GPIO_PIN_3}, 1);
INA228 INA228_2(&i2c_bus, INA228::ADDR_0x40);

io_ctrl led1(GPIOC, GPIO_PIN_4);
io_ctrl led2(GPIOC, GPIO_PIN_5);
io_ctrl led3(GPIOA, GPIO_PIN_7);

spi_port g_flash_spi({
    spi_id::spi4, GPIOF, GPIO_PIN_7, GPIOF, GPIO_PIN_9,
    GPIOF, GPIO_PIN_8, GPIOF, GPIO_PIN_6,
    RESET, GPIO_AF_5, SPI_PSC_2, SPI_CK_PL_HIGH_PH_2EDGE,
    SPI_ENDIAN_MSB, SPI_FRAMESIZE_8BIT
});
w25q128 g_flash(g_flash_spi);

timer_port g_tick_timer({timer_id::timer2, 59999, 999, {}, 0});

void _on_timer_tick(void *) { led3.toggle(); }

// ════════════════════════════════════════════════════════════
void function_init(void) {
  DebugPort_Init();
  i2c_bus.i2c_init();
  INA228_2.init();

  led1.init(GPIO_MODE_OUTPUT, GPIO_PUPD_NONE, GPIO_OTYPE_PP, GPIO_OSPEED_50MHZ);
  led2.init(GPIO_MODE_OUTPUT, GPIO_PUPD_NONE, GPIO_OTYPE_PP, GPIO_OSPEED_50MHZ);
  led3.init(GPIO_MODE_OUTPUT, GPIO_PUPD_NONE, GPIO_OTYPE_PP, GPIO_OSPEED_50MHZ);

  Protocol_Init();

  g_flash_spi.init(); g_flash.init();
  uint32_t f_id = g_flash.jedec_id();
  debug_uart.send_data((const uint8_t *)"\r\n[BOOT] Flash ", 14);
  for (int8_t n=5; n>=0; n--) { uint8_t h=(f_id>>(n*4))&0xF; uint8_t c=h<10?'0'+h:'A'+(h-10); debug_uart.send_data(&c,1); }
  debug_uart.send_data((const uint8_t *)(f_id==0xEF7018 ? " OK" : " FAIL"), f_id==0xEF7018 ? 3 : 5);

  g_tick_timer.init(); g_tick_timer.on_timeout(_on_timer_tick, nullptr); g_tick_timer.start();

  // ── CAN 回环测试 ───────────────────────────────────────
  {
      can_port can({can_id::can1, GPIOB, GPIO_PIN_13, GPIOB, GPIO_PIN_12,
                    GPIO_AF_9, 500000, CAN_SILENT_LOOPBACK_MODE, 15});
      can.init();

      const uint8_t tx[8] = "CAN_OK!";
      bool sent = can.send(0x123, false, tx, 8);
      bool rdy  = can.rx_ready();
      uint32_t id=0; bool ext=false; uint8_t rx[8]={0}, len=0;
      bool ok = can.recv(id, ext, rx, len);

      debug_uart.send_data((const uint8_t *)"\r\n[CAN] s=", 9);
      uint8_t sc=sent?'1':'0'; debug_uart.send_data(&sc,1);
      debug_uart.send_data((const uint8_t *)" rdy=", 5);
      uint8_t rc=rdy?'1':'0'; debug_uart.send_data(&rc,1);

      can.deinit();
  }
}

// ════════════════════════════════════════════════════════════
#if ACTIVE_PROTOCOL == PROTOCOL_MODBUS
#include "modbus_map.hpp"
enum {
    REG_BUSV_HI=0,REG_BUSV_LO=1,REG_SHUNTV_HI=2,REG_SHUNTV_LO=3,
    REG_CUR_HI=4,REG_CUR_LO=5,REG_PWR_HI=6,REG_PWR_LO=7,
    REG_TEMP_HI=8,REG_TEMP_LO=9,
    REG_ENERGY_3=10,REG_ENERGY_2=11,REG_ENERGY_1=12,REG_ENERGY_0=13,
    REG_CHARGE_3=14,REG_CHARGE_2=15,REG_CHARGE_1=16,REG_CHARGE_0=17,
    REG_MFR_ID=18,REG_DEV_ID=19,REG_ALERT=20,
};
void UpdateModbusRegisters(void) {
    extern uint16_t StatusBlock[];
    int32_t vm=INA228_2.getBusVoltage_mV(),sv=INA228_2.getShuntVoltage_uV();
    int32_t im=INA228_2.getCurrent_mA(),pm=INA228_2.getPower_mW(),tm=INA228_2.getTemperature_mC();
    int64_t em=INA228_2.getEnergy_mJ(),cm=INA228_2.getCharge_mC(),vu=(int64_t)vm*1000;
    StatusBlock[REG_BUSV_HI]=(uint16_t)((uint64_t)vu>>16); StatusBlock[REG_BUSV_LO]=(uint16_t)(vu);
    StatusBlock[REG_SHUNTV_HI]=(uint16_t)((uint32_t)sv>>16); StatusBlock[REG_SHUNTV_LO]=(uint16_t)(sv);
    StatusBlock[REG_CUR_HI]=(uint16_t)((uint32_t)im>>16); StatusBlock[REG_CUR_LO]=(uint16_t)(im);
    StatusBlock[REG_PWR_HI]=(uint16_t)((uint32_t)pm>>16); StatusBlock[REG_PWR_LO]=(uint16_t)(pm);
    StatusBlock[REG_TEMP_HI]=(uint16_t)((uint32_t)tm>>16); StatusBlock[REG_TEMP_LO]=(uint16_t)(tm);
    StatusBlock[REG_ENERGY_3]=(uint16_t)((uint64_t)em>>48); StatusBlock[REG_ENERGY_2]=(uint16_t)((uint64_t)em>>32);
    StatusBlock[REG_ENERGY_1]=(uint16_t)((uint64_t)em>>16); StatusBlock[REG_ENERGY_0]=(uint16_t)(em);
    StatusBlock[REG_CHARGE_3]=(uint16_t)((uint64_t)cm>>48); StatusBlock[REG_CHARGE_2]=(uint16_t)((uint64_t)cm>>32);
    StatusBlock[REG_CHARGE_1]=(uint16_t)((uint64_t)cm>>16); StatusBlock[REG_CHARGE_0]=(uint16_t)(cm);
    StatusBlock[REG_MFR_ID]=INA228_2.getManufacturerID(); StatusBlock[REG_DEV_ID]=INA228_2.getDeviceID();
    StatusBlock[REG_ALERT]=INA228_2.readDiagAlertRaw();
}
#endif

void function_loop(void) {
  APP_PC_Task();
#if ACTIVE_PROTOCOL == PROTOCOL_MODBUS
  UpdateModbusRegisters();
#endif
  led1.toggle(); led2.toggle();
  delay_ms(10);
}
