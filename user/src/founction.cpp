#include "founction.hpp"
#include "debug_uart.h"
#include "device_i2c_bus.hpp"
#include "device_i2c_dev.hpp"
#include "gd32f4xx_gpio.h"
#include "i2c_test_simple.hpp"
#include "ina226.hpp"
#include "ina228.hpp"
#include "io_ctrl.hpp"
#include "systick.h"
#include <cstring>
#include <stdio.h>

char debug_buffer[256];

device_i2c_bus i2c_bus({GPIOE, GPIOE, GPIO_PIN_2, GPIO_PIN_3}, 1);
// device_i2c_bus i2c_bus1({GPIOG, GPIOG, GPIO_PIN_4, GPIO_PIN_5}, 2);


// INA228 INA228_1(&i2c_bus, INA228::ADDR_0x40);
INA228 INA228_2(&i2c_bus, INA228::ADDR_0x40);

// INA226 INA226_1(&i2c_bus, INA226::ADDR_0x40);

io_ctrl led1(GPIOC, GPIO_PIN_4);
io_ctrl led2(GPIOC, GPIO_PIN_5);
io_ctrl led3(GPIOA, GPIO_PIN_7);

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

  debug_init_config(DEBUG_BAUDRATE); // 初始化调试串口
}

void function_loop(void) {
  led1.toggle();
  led2.toggle();
  led3.toggle();

  // int32_t bus_voltage_mV = INA226_1.getBusVoltage_mV();
  // int32_t shunt_voltage_uV = INA226_1.getShuntVoltage_uV();
  // int32_t current_mA = INA226_1.getCurrent_mA();
  // int32_t power_mW = INA226_1.getPower_mW();

  // snprintf(debug_buffer, sizeof(debug_buffer), "bus_voltage_mV=%d, shunt_voltage_uV=%d, current_mA=%d, power_mW=%d\n", bus_voltage_mV, shunt_voltage_uV, current_mA, power_mW);

  debug_send_data((uint8_t*)debug_buffer, strlen(debug_buffer));

  delay_us(100);
  // INA226_1.dumpRegisters();
  // INA228_2.dumpRegisters();
  delay_ms(100);

  int32_t bus_voltage_mV = INA228_2.getBusVoltage_mV();
  int32_t shunt_voltage_uV = INA228_2.getShuntVoltage_uV();
  int32_t current_mA = INA228_2.getCurrent_mA();
  int32_t power_mW = INA228_2.getPower_mW();
  int64_t energy_mJ = INA228_2.getEnergy_mJ();
  int64_t charge_mC = INA228_2.getCharge_mC();

  snprintf(debug_buffer, sizeof(debug_buffer), "bus_voltage_mV=%d, shunt_voltage_uV=%d, current_mA=%d, power_mW=%d, energy_mJ=%ld, charge_mC=%ld\n", bus_voltage_mV, shunt_voltage_uV, current_mA, power_mW, energy_mJ, charge_mC);
  debug_send_data((uint8_t*)debug_buffer, strlen(debug_buffer));

  delay_ms(1000); // 延时1秒
}
