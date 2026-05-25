#include "founction.hpp"
#include "usart_port.hpp"
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
}

void function_loop(void) {
  APP_PC_Task();         // 协议轮询

#if ACTIVE_PROTOCOL == PROTOCOL_MODBUS
  UpdateModbusRegisters();
#endif

  led1.toggle();
  led2.toggle();
  led3.toggle();

  delay_ms(10);           // 保持主循环节拍，避免外设异常
}
