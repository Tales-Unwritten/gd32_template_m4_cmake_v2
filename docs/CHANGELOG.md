# Changelog

## 2026-10-06 — FreeRTOS API 编辑器跳转索引 + 两项配置补全

### Added

- `code/src/freertos_api_index.c`：由脚本自动生成的 **API 索引**（199 个已启用 API），
  采用 `/// 名字()` + `/// 说明` 的**文档注释风格**，并在每条下附**代码引用行**作兜底：
  注释行靠 clangd 的“注释内跳转”，代码引用行是标准转到定义（任何 clangd 都能跳）。
  函数 → `.c` 实现，宏 → 宏定义处。文件仅含 `#include` + 注释 + 一个从不调用的
  `static inline` 函数（内含引用），**零代码/零数据**（`nm` 验证无 `T/D/B` 符号）；
  纳入构建以获得 clangd 正确的编译参数；CMake 中仅当 `USE_FREERTOS=1` 时编译。

### Changed

- `code/inc/FreeRTOSConfig.h`：新增 `configUSE_APPLICATION_TASK_TAG=1`、
  `configUSE_SB_COMPLETED_CALLBACK=1`（使 `vTask*ApplicationTaskTag`、
  `*CreateWithCallback` 系列真正可用；对应 README 条目标注为 ✅）。

### Verified

- `debug` 构建通过、无新增告警；`text` 仍 36500B（未用 API 被 `--gc-sections` 回收，
  故开这两个配置宏不增 Flash）；索引目标文件经 `nm` 校验无任何代码/数据符号。

## 2026-10-06 — FreeRTOS 常用 API / 功能补全（FreeRTOSConfig.h）

### Changed

- `code/inc/FreeRTOSConfig.h`：补全常用可选 API 与功能
  - 任务类 `INCLUDE_`：新增 `INCLUDE_xTaskGetHandle` / `INCLUDE_xTaskGetIdleTaskHandle` /
    `INCLUDE_uxTaskGetStackHighWaterMark2` / `INCLUDE_xTaskResumeFromISR`；
    `INCLUDE_vTaskDelayUntil` 改为 v11 新名 `INCLUDE_xTaskDelayUntil`
    （两者**不可同时定义**，否则内核 `#error`）。
  - 队列 / 互斥量：`configUSE_QUEUE_SETS` 0→1；`INCLUDE_xQueueGetMutexHolder` /
    `INCLUDE_xSemaphoreGetMutexHolder` 置 1。
  - 软件定时器：`INCLUDE_xTimerPendFunctionCall` 置 1。
  - 调试：`configUSE_TRACE_FACILITY` 0→1、`configUSE_STATS_FORMATTING_FUNCTIONS` 0→1
    （启用 `vTaskGetInfo` / `uxTaskGetSystemState` / `vTaskList` 等）。

### Verified

- `debug` 编译通过；Flash 36048→**36500B（+452B）**，BSS 不变（40512B）。
- 上板（DAP-Link / `/dev/ttyACM0`，`tests/test_string.py`）：**35/35 通过**，
  压力 100/100、丢包 0%、INA228 `MFR=0x5449 / DEV=0x2281`。

### Docs

- `freertos/README.md`：新增「常用 API 速查（按功能分类）」——分 10 类
  （任务 / 内核控制 / 队列 / 信号量与互斥量 / 软件定时器 / 事件组 / 流·消息缓冲 /
  任务通知 / 内存管理 / 列表）列出 **249 个公共 API**，每行 `函数名  /// 中文说明`，
  并标注当前配置下的可用性（✅ 已启用 / ⚙️ 需开宏 / ➖ 底层内部 / ⛔ 不适用）。
- 每个函数名均为**可点击链接**，点击直接跳到**代码实现处**（`文件#L行号`）：
  函数 → 对应 `.c` 实现；宏 → 递归解析到它最终调用的 `.c` 实现
  （如 `xQueueCreate` → `queue.c` 的 `xQueueGenericCreate`）；
  仅纯头文件宏（`taskYIELD`/`portYIELD`/`listGET_OWNER_OF_NEXT_ENTRY` 等 8 个）指向头文件。
  链接由脚本按「`.c` 实现 → 宏展开递归 → 头文件原型」优先级生成，
  排除 MPU 别名头与空体桩宏干扰，并优先指向本工程实际编译的 `heap_4.c` / GCC 端口。

### Notes

- 澄清：队列 / 二值·计数·递归信号量 / 互斥量 / 事件组 / 流缓冲**本就可用**，
  由 `configSUPPORT_DYNAMIC_ALLOCATION` 与 `configUSE_*` 控制，**不受 `INCLUDE_` 门控**。
- 仍未启用的常见项：`configCHECK_FOR_STACK_OVERFLOW`、`configUSE_MALLOC_FAILED_HOOK`
  （需配套实现 hook 函数）、`configGENERATE_RUN_TIME_STATS`（需运行时间计数器）。

## 2026-10-06 — 引入 FreeRTOS 内核（V11.3.1）+ 目录规范化

### Added

- `freertos/`：FreeRTOS Kernel **V11.3.1**（上游 tag，MIT）。收录内核源码、`include/`、
  **GCC** 移植层 `portable/GCC/ARM_CM4F`（本工程构建用）与 **RVDS** 移植层
  `portable/RVDS/ARM_CM4F`（Keil/ARMCC 备用）、`portable/MemMang/heap_1~5.c`，
  以及 `README.md`（版本/来源/双端口/Keil 用法说明）。
- `code/inc/FreeRTOSConfig.h`：应用配置（`configCPU_CLOCK_HZ=240MHz`、`configTICK_RATE_HZ=1000`、
  `configTOTAL_HEAP_SIZE=32KB`、`configPRIO_BITS=4`、`configMAX_SYSCALL_INTERRUPT_PRIORITY=5<<4`、
  `configUSE_TICK_HOOK=1`，并将 `vPortSVCHandler/xPortPendSVHandler/xPortSysTickHandler`
  映射到 CMSIS 的 `SVC_Handler/PendSV_Handler/SysTick_Handler`）。
- `code/inc/rtos_conf.h`：`USE_FREERTOS` 编译期总开关（默认 1）。
- `CMakeLists.txt`：新增 `option(USE_FREERTOS ... ON)`；为 RTOS 目标补充内核源文件、
  `heap_4.c`、`GCC/ARM_CM4F/port.c` 及 include 路径，并传递 `-DUSE_FREERTOS=1/0`。

### Changed

- 移除旧 `FreeRTOS/`（**V9.0.0，仅含 RVDS/Keil 移植层，arm-none-eabi-gcc 无法编译**），
  以 `freertos/`（小写，符合项目命名规范）替换为最新 V11.3.1。
- `App/` → `app/`：`App/` 为真实目录，`app` 原是指向它的**跟踪型符号链接**（非重复目录）。
  改为**真实小写目录 `app/`** 并删除符号链接，消除大小写并存缺陷（`git mv` 保留重命名历史）。
- `code/src/gd32f4xx_it.c`：`SVC_Handler` / `PendSV_Handler` / `SysTick_Handler` 用
  `#if !USE_FREERTOS` 屏蔽并注释原因（FreeRTOS 端口接管，避免重复定义）；裸机模式自动恢复。
- `code/src/main.c`：按 `USE_FREERTOS` 分支——RTOS 下 `function_init()` 后创建 `default_task`
  （忙轮询循环 `function_loop()`，**不加 vTaskDelay**）并 `vTaskStartScheduler()`，不再调用
  `systick_config()`；裸机分支保持原 `while(1)` 主循环。
- `code/src/systick.c`：新增 `vApplicationTickHook()`（`USE_FREERTOS` 下）以维持裸机
  `get_tick()` 兼容（1 tick=1ms）；`systick_config()` 增加「RTOS 下勿调用」说明。

### Verified

- `debug` + `USE_FREERTOS=ON`（默认）：编译链接通过（RAM 40640B / FLASH 36204B）。
- `debug` + `USE_FREERTOS=OFF`：编译链接通过（RAM 7392B / FLASH 29636B），验证裸机回退路径。
- `release` + `USE_FREERTOS=ON`（-O2）：编译链接通过，无 FreeRTOS 相关告警。
- **上板实测**（MickeyPlus V2.0.0 / DAP-Link / `/dev/ttyACM0`，`tests/test_string.py`）：
  - `USE_FREERTOS=ON`：**35/35 通过（100%）**，INA228 `MFR=0x5449 / DEV=0x2281`，
    压力测试 100/100、丢包 0%、平均响应 ~11.8ms。
  - `USE_FREERTOS=OFF`（裸机）：**35/35 通过（100%）**，行为与 RTOS 一致，回退路径未被破坏。

### Notes

- `get_tick()` 由 Tick Hook 维持，`delay_us/ms` 为忙等、与 SysTick 无关，故 device 层遗留驱动
  在 RTOS 下行为不变。
- 内存管理用 `heap_4.c`（可释放 + 相邻空闲块合并），堆为内部静态数组 `ucHeap[]`，**无需改链接脚本**。
- ⚠️ 后续若在 ISR 中调用 FreeRTOS API，该 NVIC 优先级必须 ≤ `configMAX_SYSCALL_INTERRUPT_PRIORITY`(5)。
- Keil 工程：AC5 用 `portable/RVDS/ARM_CM4F`；AC6(armclang) 官方说明直接用 GCC 端口。
- ⚠️ **软串口必须忙轮询**：`soft_uart_port` 是轮询式实现，`default_task` 中**不可加
  `vTaskDelay`/`taskYIELD`**（否则轮询率降为 1kHz，115200 下丢字节 → 接收乱码）。
  本次实测首版加 `vTaskDelay(1)` 导致乱码，改为忙轮询后 100% 通过。后续若改为
  中断/DMA 收串口，方可让出 CPU。

## 2026-10-06 — Release 位操作驱动回归（-O2 延时扫描 + 软 SPI 上板）

### Verified

- `arm-none-eabi-objdump` 确认公共 `delay_us`/`delay_ms` 在 **-O2 下为真循环**（非 `bx lr`）；全项目位操作驱动延时来源已分类，**未发现新的「空转循环被优化掉」隐患**。
- 实板（Release，DAP-Link + `/dev/ttyACM0`）：`inter_soft_spi` 驱动真实 W25Q128，`delay=0` 与 **`delay=1`（压 `delay_us` 路径）均 PASS**（JEDEC `EF 70 18`、擦写回读 MATCH）；`inter_i2c_bus` + INA228 `MFR=0x5449 / DEV=0x2281` OK。

### Docs

- `docs/tests/TEST_REPORT.md`：新增「七、2026-10-06 Release 位操作驱动回归」——延时来源分类表、上板结果、未覆盖项。

### Notes

- 未覆盖（板上无器件）：ds18b20/ds2401（1-Wire）、dhtnew、hc40xx、74hc*、shiftin/out、x9c10x、drv8825、ms5611、ads1115 —— 「优化掉」风险已排除，但**时序精度**需接器件 + 示波器实测。
- `delay_us`/`delay_ms` 系数（40/40000）估算偏短约 17%，属精度维度（可用 DWT 标定）。
- 验证用的临时改动（`function_init` 暂换软 SPI 自检、软 SPI `delay=1`）**均已还原**，工作区无残留。

## 2026-10-06 — 软 I2C 波特率化改造（单参配置 + DWT 自标定）

### Changed

- `interface/inc/inter_i2c_bus.hpp`：`I2C_Bus_Info_t` 新增 `clock_speed`（目标 SCL Hz，默认 400k）与 `sys_clk_hz`（系统主频 Hz，默认 240M，**不依赖 `SystemCoreClock` 符号**）；构造函数收敛为单参 `explicit inter_i2c_bus(const I2C_Bus_Info_t&)`（去掉 `delay_us`）；成员 `_delay_val` → `_delay_count`。
- `interface/src/inter_i2c_bus.cpp`：半周期延时改为**按波特率反算忙等次数** `_delay_count = sys_clk_hz / (clock_speed * 2 * 每循环周期)`；`_delay()` 用 `volatile` 忙等（防 -O2 优化为空）；「每循环周期」默认 **DWT 上电自标定**，并保留**固定值分支注释备用**（无 DWT 的平台）。
- 调用点 `user/src/founction.cpp`：`inter_i2c_bus i2c_bus({portE, portE, pin2, pin3});`（默认 400k@240MHz）。

### Verified

- `debug` / `release` / `iap` 三套构建+链接通过。
- 实板（DAP-Link + `/dev/ttyACM0`）：INA228 `MFR=0x5449 / DEV=0x2281`（正确）；W25Q128 硬件 SPI **PASS**。

### Notes

- 方案：按目标速率反算循环次数（`*2` = 半周期，每循环周期折算）；上半层不受影响（device 层持抽象 `i2c_bus*`；`I2C_Bus_Info_t` 仅 `founction.cpp` 一处构造）。
- `inter_i2c_test_simple.hpp`（死代码）中 `inter_i2c_bus _bus;` 本就不可编译，未处理。

## 2026-10-06 — 修复 delay_us/delay_ms 被 -O2 优化为空（软 I2C 读 INA228 错乱根因）

### Fixed

- `code/src/systick.c`：`delay_us()` / `delay_ms()` 的空转循环在 **-O2 下被优化成 `bx lr`（零延时）**。改用 `volatile` 计数循环恢复实际延时。
  - 影响面：所有依赖 `delay_us/ms` 的位操作驱动（软 I2C / 软 SPI、ds18b20 / dht / hc40xx / 74hc / shiftin/out / x9c10x 等）在 Release 下延时均为 0。
  - 现象：软 I2C 全速翻转（MHz 级）远超 INA228 400kHz 上限 → ACK 通过但读数据错乱（`MFR` 读成 `0x7FFF`，DEV `0x3FFF`）。
  - （定位过程：先 A/B 排除 `inter_i2c_dev`/软 I2C 逻辑；再对比旧版 `hard_soft_i2c.c`；最后 `objdump` 发现 `delay_us` 只剩 `bx lr`。）

### Verified

- 实板（DAP-Link + `/dev/ttyACM0`）：修复后 INA228 `MFR=0x5449 / DEV=0x2281`（**正确**），Vbus=0（悬空，符合预期）；W25Q128 硬件 SPI **PASS**。
- 说明：PE2/PE3 **无 I2C 复用功能**，硬 I2C 在该引脚对不可用（仅软 I2C）；此结论已记录。

## 2026-10-06 — 测试文档更新：固定测试接口 + 实板回归

### Docs

- `docs/tests/TEST_REPORT.md`：新增「固定测试接口（硬件连线速查）」表——DAP-Link 调试串口 PA9/10、INA228 软 I2C PE2/PE3（ALERT **PG14**）、AT24C02 **PB6/PB7**、W25Q128 SPI4 **PF6/7/8/9**、RS485/RS232/OLED/LED；补充当前测试固件状态、烧录命令；新增「2026-10-06 复测（架构重构后回归）」小节。

### Verified

- 实板（DAP-Link + `/dev/ttyACM0`，烧录后复位）：上电自检 **W25Q128 硬件 SPI PASS**（JEDEC `EF 70 18`，Erase/Write/Readback MATCH，32B verified）。
- **I2C（INA228）上板验证**：启用 `i2c_bus.init()` + `ina228_selftest()` 后自检全流程跑通，但读数异常（`MFR=0x7FFF`/`DEV=0x3FFF`，期望 `0x5449`/`0x2281`；Vbus/I=0）。**A/B 对比**（读路径临时改回 HEAD 内联写法）结果完全一致 → **排除 I2C 抽象化重构嫌疑**，属独立的芯片/接线现象，待硬件侧确认。

### Changed

- `user/src/founction.cpp`：`function_init()` 启用 `i2c_bus.init()` 并在启动时调用 `ina228_selftest()`（I2C 上板验证；W25Q128 自检保留）。

## 2026-10-06 — I2C 设备层抽象化（依赖倒置落地）

### Changed

- `interface/inc/inter_i2c_dev.hpp` + `interface/src/inter_i2c_dev.cpp`：总线依赖 `inter_i2c_bus*` → `i2c_bus*`（include 由 `inter_i2c_bus.hpp` → `inter_i2c.hpp`）。
- 31 个 device I2C 驱动（`device_24lc1025`/`abp2`/`ads1115`/`as5600`/`asdx`/`bh1750`/`cat24m01`/`cht8305`/`ds3232`/`eeprom`/`fram`/`gy521`/`hc4067`/`ina219/226/228/260/3221`/`lcd1602`/`mcp23017`/`mcp3424`/`mcp4725`/`mcp9808`/`mprls`/`pca9685`/`pcf85263`/`pcf8574`/`pcf8591`/`sht31`/`sht4x`/`tca9548`）的构造参数与成员类型 `inter_i2c_bus*`/`&` → `i2c_bus*`/`&`；include 同步改抽象头。
- 隐式上转（`inter_i2c_bus*` → `i2c_bus*`）保证现有调用点**源码兼容，零改动**；硬 I2C（`i2c_hw_bus`）现可注入设备层。
- 保持指针注入（非引用）；`i2c_bus` 基类未扩充（设备层实际只用基类方法）。

### Fixed

- `device/src/device_rotarydecoder.cpp`：原先经 `inter_i2c_bus.hpp` 传递获得 SDK 宏 `SET`，改抽象头后缺失；改用平台无关的 `!= 0u` 比较（`PCF8574::read` 返回 0/1）。

### Verified

- `cmake --build --preset debug` / `--preset release` / `--preset iap` 全部构建+链接通过。
- 静态检查：`device/` 与 `inter_i2c_dev` 层已无具体 `inter_i2c_bus` 类型引用，也无 `inter_i2c_bus.hpp` 包含。

## 2026-10-06 — 软 SPI 时序枚举（对齐 STM32 版）

### Changed

- `interface/inc/inter_soft_spi.hpp`：新增 `soft_mode_enum_t`（`soft_mode_0..3`）与 `soft_order_enum_t`（`MSB`/`LSB`）；`SoftSpiConfig.mode`/`bit_order` 由 `uint8_t` 改为上述枚举；`set_mode()`/`set_bit_order()` 形参同步改枚举；私有成员 `_cs_active_level` 由 `bool` 改回 `polarity`。
- `interface/src/inter_soft_spi.cpp`：构造 / `set_mode` / `set_bit_order` / `_transfer_byte_core` / `cs_select` / `cs_deselect` 适配枚举；CS 控制改用 `io_ctrl::write(polarity)`。
- 调用点：`user/src/founction.cpp` 的 `w25q_spi` 位置化初始化 `0, 0, 0` → `soft_mode_0, MSB, 0`。

### Verified

- `cmake --build --preset debug` / `--preset release` / `--preset iap` 全部构建+链接通过。

## 2026-10-06 — 接口层架构统一：I2C/UART 抽象基类 + 串口模板化

### Added

- `interface/inc/inter_i2c.hpp`：新增 I2C 抽象基类 `i2c_bus`（软/硬 I2C 统一入口），含 `read_bytes()` 默认实现（逐字节 + 末字节 NACK + STOP）。
- `interface/inc/inter_usart.hpp`：新增串口抽象基类 `usart_port_base`（init/deinit/is_initialized/send_data/send_data_it/buffer/periph/rx_buf_capacity）。

### Changed

- **I2C**：`inter_i2c_bus`（软）与 `i2c_hw_bus`（硬）改为 `: public i2c_bus` 并全量 `override`；软 `wait_ack` 返回 `uint8_t`→`bool`；`is_busy()` 加 `const`。
- 硬 I2C `i2c_hw_bus`：新增 `read_bytes()`（GD32 F4 时序：`len==1`/`i==len-2` 提前 NACK）与 `bus_recovery()`（关外设→GPIO 开漏 9 时钟 + STOP→重配外设）；抽出私有 `_configure_peripheral()` 供 init/recovery 复用。
- `inter_i2c_dev.cpp`：`read_16bit()`/`freedom_read()` 读取段改用 `_bus->read_bytes(...)`（`.hpp` 成员类型保持 `inter_i2c_bus*`，与 STM32 版一致）。
- **UART**：`usart_port`、`soft_uart_port` 模板化为 `template <uint16_t RX_BUF_SIZE = 256> : public usart_port_base`；RX 缓冲由堆 `new[]` 改为编译期静态数组（**零堆分配**）；模板成员定义内联于头文件，ISR/`g_port_map` 保留在 `.cpp`。
- `UsartPortConfig` 删除 `rx_buf_size` 字段（改由模板参数承载）。
- `g_port_map` 改为 `usart_port_base*`，ISR 经基类虚接口分发；注册/注销改为非模板自由函数 `usart_port_register/unregister`。
- 调用点更新：`device_serial.{hpp,cpp}`（`usart_port<256>`）、`device_mhzco2.{hpp,cpp}`（`usart_port_base&`）、`iap_boot.cpp`（`usart_port<512>`）、`modbus.cpp`/`string_pro.cpp`（`extern soft_uart_port<256>`）、`user/src/founction.cpp`（`soft_uart_port<256>`）。

### Verified

- `cmake --build --preset debug` / `--preset release` / `--preset iap` 全部构建+链接通过（IAP 四目标：boot/app_a/app_b/template）。

### Notes

- 保留 GD32 平台约定：`port_enum_t`/`pin_enum_t`/`af_enum_t`、`io_ctrl` 4 参 init、`mode`+`otype` 分离；未引入 STM32 的 `GPIO_TypeDef*`/`afio_enum_t`。
- 未做 DMA（后续单独立项）；未改软串口发送时序（另项）。
- 权衡（**已决策，暂不改**）：为支持模板化，`inter_usart.hpp`/`inter_soft_uart.hpp` 现直接 include GD32 SDK 头（设备/应用层会传递性引入）。经评估，嵌入式场景下这种耦合可接受，**不做"下沉为非模板自由函数"**（留作可选后续项）。

## 2026-10-06 — io_ctrl 枚举化迁移（类型安全，全项目适配）

### Changed

- `interface/inc/inter_io_ctrl.hpp`：GPIO 参数由 GD32 SDK 整型宏改为项目枚举
  （`port_enum_t`/`pin_enum_t`/`mode_enum_t`/`pull_enum_t`/`speed_enum_t`/`otype_enum_t`/`af_enum_t`），
  `read()`/`read_output()` 返回 `polarity`。
- 新增哨兵 `port_enum_t::port_none = 0`（与既有 `pin_none` 对应，表示"未绑定"）；
  修正枚举笔误 `Hig` → `High`（`active_high = High`）。
- 接口层 11 个 config 结构体字段类型改为枚举：`UsartPortConfig`、`SpiPortConfig`、`SoftSpiConfig`、
  `SoftUartConfig`、`I2C_Bus_Info_t`、`I2cHwConfig`、`AdcChannelConfig`、`CanPortConfig`、
  `DacPortConfig`、`ExtiConfig`、`TimerChannelConfig`（`cs_active_level`→`polarity`、`pull`→`pull_enum_t`）。
- 全项目调用点迁移（约 50 个文件）：
  - `interface/src/` 11 个（init/reinit/set_af 参数、`read()` 与 `High`/`Low` 比较）；
  - `device/inc/` 15 个（INA 系列 + 74hc138/154/590、hc4051~4067_gpio、shiftin/shiftout、x9c10x）；
  - `device/src/` 15 个（INA 系列 + dhtnew/ds18b20/ds2401/hx711/mcp_dac/drv8825/serial/w25q128_flash/pca9685 等）；
  - `user/src/founction.cpp`、`protocol/iap/boot/src/iap_boot.cpp` 的实例化/构造点。
- 未改动任何传给 GD32 SDK 函数的宏（`usart_*`/`spi_*`/`adc_*`/`syscfg_exti_line_config` 等仍用 SDK 宏）。

### Verified

- 逐编译单元检查（`build/release/compile_commands.json`，98 TU）：**0 失败**。
- `cmake --build --preset debug` / `--preset release` / `--preset iap` 均构建+链接通过
  （release：FLASH 19668B/2MB，RAM 6608B/704KB；IAP：boot/app_a/app_b + template 四目标）。
- 未被任何 TU 引用的 header-only 驱动（74hc138/154/590、hc4051/4052/4053/4067_gpio、shiftin/shiftout、x9c10x）
  用项目编译参数单独 `-fsyntax-only` 验证通过。

## 2026-10-06 — 构建配置审计 + clangd 配置接入

### Added

- 项目根新增 `.clangd`：压制 GD32 SDK 头文件的 clangd 假报错
  `pp_including_mainfile_in_preamble`。单独打开某个库头文件（如 `gd32f4xx_gpio.h`）时，
  包含链 `gd32f4xx_xxx.h -> gd32f4xx.h -> gd32f4xx_libopt.h -> gd32f4xx_xxx.h` 成环，
  clangd 构建 preamble 时误判为"主文件被递归包含"（include guard 无法拦截）。
  该配置仅影响 IDE 语言服务，不影响 GCC / CMake 实际构建。
- 注：`.clangd` 目前被 `.gitignore`（第 14 行，IDE 配置项）忽略；如需纳入版本控制需取消忽略。

### Verified

- `clangd --check=Firmware/.../gd32f4xx_gpio.h` 复现原报错，加 `.clangd` 后该诊断消失且无配置告警。
- `cmake --preset release && cmake --build --preset release` 全量构建通过
  （FLASH 19652B/2MB，RAM 6616B/704KB，TCMRAM 0B）。
- 构建配置审计（只读，未改动源码）：发现的隐患见会话记录，本文件仅登记已实施变更。

## 2026-08-18 — IAP 目录结构重构：boot / application / common 分层

### Changed

- `protocol/iap/` 按功能拆分（文件内容零改动，仅路径调整）：
  - `boot/`（inc+src）：bootloader 私有——`iap_boot.*`（状态机/跳转）、`iap_proto.*`（升级协议）、`boot_main.cpp`（入口+异常处理）；
  - `application/`（inc+src）：应用侧私有——`iap_app.*`（VTOR/上报/请求/状态提示）；
  - `common/`（inc+src）：boot/app 共享——`iap_conf`（分区/协议常量）、`iap_image`（镜像头）、`iap_param`（参数区日志）、`iap_crc`（校验）；
  - `ld/`、`tools/` 位置不变。
- `protocol/iap/CMakeLists.txt`：include 路径与源路径同步更新（boot/src、application/src、common/src）。

### Verified

- 三 target 重新构建通过，体积与重构前完全一致（boot 18684B / app 23848B），无逻辑变化；裸跑版编译零影响。

## 2026-08-18 — IAP 升级系统 M5：硬件实测通过（全链路验证 + 5 项缺陷修复）

### Verified（DAP-Link + /dev/ttyACM0 + GD32F470ZI 实板）

- Boot 启动 → 参数初始化（双扇区日志）→ 跳转 app 全链路正常；app 软串口（PA9/PA10）自检输出 PASS。
- 升级模式双触发验证：PC13 按键进入 ✓、REQ_UPGRADE 命令标志进入 ✓（QUERY 应答正常）。
- 完整升级流程：START（擦分区）→ 90×DATA 分包（逐包 ACK）→ END（全量 CRC32 校验）→ ACTIVATE，A↔B 双向升级均成功。
- 确认窗口状态机：ACTIVATE→attempt 计数→app 上报 boot_ok→连续 3 次成功固化（state=IDLE）✓。
- 严格回滚：注入"连续 2 次启动失败"场景，boot 自动将 active 回滚到旧槽（旧固件零写入）✓。

### Fixed（实测发现 5 项）

1. **boot 缺 SysTick/HardFault handler**：原在 app 文件 gd32f4xx_it.c，boot 未编入 → 向量表指向 Default_Handler，SysTick 中断死循环（PC 卡 0x08001230）。已在 `boot_main.cpp` 自行定义。
2. **出厂烧录格式**：flash-iap 烧裸 bin，boot 镜像校验失败（分区首字是 MSP 非 magic）→ 烧录 target 改为烧升级包 `iap_gd32f470_app_*.bin`；app 链接基址 = 分区基址+0x40（分区存储格式=头部+固件），跳转/VTOR 相应 +64B。
3. **USB CDC 拆包导致 IDLE 误判**：DAP-Link 虚拟串口每 64B 一个 USB 帧（帧间 ~1ms > 86us），长帧被 IDLE 拆成多段半帧全部丢弃（小帧正常、265B 帧必挂）。`Iap_Proto_Task` 改为**长度驱动判帧**（解析帧头 LEN，收满再处理，不依赖 IDLE）。
4. **DATA 末包长度公式错误**：`image_len-64-seq×256` 重复扣除头部（头部仅在包 0）→ 修正为 `image_len-seq×256`。
5. **ACTIVATE 后确认窗口被误清**：attempt_slot=0 不满足首启判定 `attempt_slot==active`，boot 误判"参数不一致"清除 PENDING → 修正为 ACTIVATE 时 attempt_slot 指向新槽。
6. **FMC 擦除粒度错配**：`flash_port::erase` 误用 `fmc_page_erase`（仅擦 4KB 页），与扇区表（16K/64K/128K）粒度不符 → 目标分区有旧固件时同扇区残留导致 DATA 写入失败（ERR_BUSY）。改用 `fmc_sector_erase`。
7. **FMC SN 编码非连续**：GD32F4xx 扇区号编码 bank1 = 0~11、bank2 = 16~27（扇区号+4，12~15 保留；依据 CTL_SECTOR_NUMBER_12=CTL_SN(16)）。传裸扇区号曾误擦 boot 区（9→SN1，已恢复），修复为 `sector_to_sn()` 映射。

### Changed（字符串协议接入 + 固化提示）

- `app/inc/protocol_conf.hpp`：`ACTIVE_PROTOCOL` 切回 `PROTOCOL_STRING_CMD`（ASCII 字符串指令）。
- `user/src/founction.cpp`：恢复 `Protocol_Init()` 调用（协议栈此前被测试模式注释，导致串口命令无人解析）；`APP_IAP` 下启动打印 IAP 状态行。
- `protocol/string/src/string_pro.cpp`：端口表改挂软串口 `suart`（硬件 debug_uart 同引脚冲突被禁用）。
- `iap_app.cpp/hpp`：新增 `Iap_AppGetStatus()`——启动提示 `IAP: NEW firmware on slot B v1.0.0, confirming (x/3)`（确认窗口）/ `IAP: active slot B v1.0.0 (confirmed)`（固化生效）。
- `interface/src/inter_flash.cpp`：新增 `diag_fail_addr/stage` 通用擦除诊断（0=无 1=查表 2=安全区 3=擦除 4=等待）。

### Verified（实测闭环）

- **字符串命令触发**：串口发 `iap_upgrade` → app 写 REQ_UPGRADE → 复位 → boot 升级模式（QUERY 确认）✓
- **升级全流程**（B 分区含旧固件残留场景）：START 扇区擦除 → 94×DATA → END 全量校验 → ACTIVATE 全通 ✓
- **确认窗口与固化提示**：`confirming (2/3)` → `(3/3)` → `(confirmed)` 串口实时可见 ✓
- 裸跑版（无 APP_IAP）编译零影响。

### Notes

- `tests/iap_test.py`：IAP 协议验证工具（query / upgrade / 单命令），升级包自动分包+停等+重试。
- 断电场景（传输中/擦除中/激活后掉电）逻辑已推演（S2/S13/S15/S20/S26），硬件断电实测留待用户环境操作。

## 2026-08-18 — IAP 升级系统 M4：App 侧接入（VTOR / 上报 / 升级命令）

### Added

- `protocol/iap/inc/iap_app.hpp` + `src/iap_app.cpp`：
  - `Iap_AppInit`：VTOR 重定位到分区基址（编译期宏 APP_SLOT_A/B）+ flash 安全区（仅参数区，app 无权碰固件区）+ 首次喂狗；
  - `Iap_AppFeedWdt`：周期喂狗（boot 跳转前已使能 IWDG，app 接管喂狗职责）；
  - `Iap_AppReportBootOk`：主循环首圈调用，仅 PENDING 确认窗口且本槽=active 时写 boot_ok（幂等）；
  - `Iap_RequestUpgrade`：写 REQ_UPGRADE 标志后软复位，boot 进入升级模式。
- `code/src/main.c`：`APP_IAP` 宏下 `Iap_AppInit()` 置于 main 第一行（先于 systick_config，VTOR 必须先于任何中断使能，否则中断落入 boot 向量表）。
- `user/src/founction.cpp`：`APP_IAP` 宏下 init/loop 各加喂狗 + 首圈上报。
- `protocol/modbus/src/modbus_map.cpp`：`Modbus_WriteRegister` 钩子——保持寄存器 0x000A（ControlBlock[0]，此前全块空闲）写 0x5AA5 触发升级。
- `protocol/string/src/str_cmd.cpp`：命令表新增 `iap_upgrade`（`APP_IAP` 宏保护）。
- IWDG 生命周期修正（硬件特性：系统复位不停止已使能的 IWDG）：boot 启动即喂狗，升级模式循环/双坏死等循环/分区擦除每扇区间歇喂狗，`Iap_Param_Write` 擦除前喂狗——避免 boot 自身升级流程（擦除 4-12s）被 6.5s IWDG 打断；跳转 app 后由 app 接管。
- boot/app 构建：boot 源不变；app 源新增 iap_crc/iap_param/iap_app + fwdgt 库；bin 输出名修正（去掉 .elf 后缀，与烧录 target 一致）。

### Verified

- 三 target 链接通过：boot 18.5K/128K；app_a/app_b 各 22.9K/512K（4.37%）。
- 裸跑版（debug，无 APP_IAP）编译通过，宏保护零影响。
- 升级包 `iap_app_a.bin`/`iap_app_b.bin` 生成成功（v1.0.0，slot A/B）。

### Pending（后续里程碑）

- M5：硬件实测（boot 跳转、升级全流程、回滚、断电场景）。

## 2026-08-18 — IAP 升级系统 M3：Boot 完整流程（协议 / 状态机 / 跳转）

### Added

- `protocol/iap/inc/iap_crc.hpp` + `src/iap_crc.cpp`：CRC16（Modbus 0xA001）+ CRC32 增量接口 `Iap_Crc32_Step`（与打包脚本 zlib.crc32 同源）；`iap_param.cpp` 改为复用该模块。
- `protocol/iap/inc/iap_boot.hpp` + `src/iap_boot.cpp`：
  - Boot 初始化：USART0(PA9/10, 115200-8N1)、PC13 按键（50ms 防抖、低有效）、flash 安全区 = AppA/AppB/Param0/Param1（不含 boot 自身，防误擦）；
  - 启动状态机：读参数 → PENDING 判定（连续 3 次成功上报→固化；连续 2 次失败→回滚，回滚=改 active 标志，旧固件全程零写入）→ 升级入口（按键/app 请求）→ active 分区校验（magic/platform/slot/len + 全量 CRC32）→ 失败试另一槽（自救）→ 双坏死等升级；
  - `Iap_JumpToApp`：关中断/停 SysTick/清 NVIC → 使能 IWDG（FWDGT_PSC_DIV64, 4095 ≈ 6.5s 兜底，跳转即死也能复位回 boot 触发回滚）→ 取分区向量表 MSP/PC 跳转。
- `protocol/iap/inc/iap_proto.hpp` + `src/iap_proto.cpp`：
  - 帧协议 `0xA5 0x5A + CMD + LEN + PAYLOAD + CRC16`，逐包停等 + 序号去重，坏帧静默丢弃；
  - 命令：QUERY（状态+三版本）、START（擦全分区→记录传输状态）、DATA（长度严格匹配、包 0 镜像头一致性校验防错槽、写后 ACK 进度）、END（全量 CRC32）、ACTIVATE（切 active→复位）、ABORT、REBOOT；
  - 擦写期间关中断 + 停等协议（擦 4-12s / 写包 ~ms 级，无数据流丢失风险）；`iap_ports[]` 端口表与 modbus/string 同构。
- `iap_conf.hpp`：状态机新增 `REQ_UPGRADE`；确认/回滚阈值常量（3/2）。
- `iap_param.hpp`：state 枚举调整（IDLE/REQ_UPGRADE/TRANSFERRING/PENDING_ACTIVATE）。
- boot target 源列表：新增 fwdgt 库、systick、inter_io_ctrl/usart/flash/wdt、iap 四源文件。

### Verified

- boot 编译链接通过：18472B / 128KB（14.09%）；向量表 MSP=0x20001190、Reset_Handler=0x080011E9 落在 boot 分区；关键符号（Iap_Boot_Run/Iap_JumpToApp/Iap_Param_Write/USART0_IRQHandler）全部存在。
- app_a/app_b 双镜像构建不受影响。
- 修复：IWDG 预分频宏 GD32 命名为 `FWDGT_PSC_DIV64`（非 STM32 的 IWDG_PRESCALER_64）。

### Pending（后续里程碑）

- M4：app 侧接入（VTOR/上报/请求命令/IWDG 喂狗）。
- M5：硬件实测（升级/回滚/断电场景）。

## 2026-08-18 — IAP 升级系统 M2：参数区双扇区日志 + 镜像打包脚本

### Added

- `protocol/iap/src/iap_param.cpp`：参数区双扇区日志实现。
  - 每扇区 128KB / 512B = 256 槽顺序写（seq 递增），写满/遇脏槽时擦另一扇区从槽 0 重写；
  - 读取扫描两扇区取「seq 最大且 CRC16 有效」记录，半写记录自动跳过；
  - 擦除频率降至每 30~40 次升级 1 次；写后读回验证；
  - 自带 Modbus CRC16（0xA001 逐位实现，标准向量 `"123456789"→0x4B37` 已验证）。
- `protocol/iap/inc/iap_param.hpp`：公开接口 `Iap_Param_Read()` / `Iap_Param_Write()`（写方自动递增 seq），注释更新为多槽描述。
- `protocol/iap/tools/iap_pack.py`：升级包打包脚本——64B 头部（magic/version/platform/image_len/crc32/slot）+ 负载 4 字节对齐 + CRC32（zlib 标准）。
- boot target 源列表加入 `iap_param.cpp`。

### Verified

- `iap_pack.py` 打包 `gd32f470_app_a.elf.bin` → `iap_app_a.bin`：解包校验 magic/version/platform/total/crc32/slot/对齐 全部通过（version 1.0.0，slot A，crc32=0x8EF3CFCD）。
- boot 重新编译链接通过。

### Pending（后续里程碑）

- M3：boot 完整流程（升级协议/状态机/跳转）。
- M4：app 侧接入（VTOR/上报/请求命令）。
- M5：硬件实测（升级/回滚/断电场景）。

## 2026-08-18 — IAP 升级系统 M1：分区配置 + 链接脚本 + 构建骨架

### Added

- `protocol/iap/` 新目录（inc/src/ld/tools），实现串口 IAP 升级（boot + A/B 双应用）。分区布局（GD32F470ZI 2MB）：
  - Boot `0x08000000` 128K（S0~S4，仅 SWD 更新）；AppA `0x08020000` 512K；AppB `0x080A0000` 512K；
  - Param0 `0x08120000` / Param1 `0x08140000` 各 128K（双扇区日志）；预留 `0x08160000` 起 640K。
- `protocol/iap/inc/iap_conf.hpp`：分区地址/大小宏、槽位定义（APP_SLOT_A/B 编译期注入）、版本/平台标识（"GDF4"）、升级协议常量（帧头/命令码/应答码/超时）。
- `protocol/iap/inc/iap_image.hpp`：64B 镜像头部结构（magic/版本/平台/总长/CRC32/slot/保留）。
- `protocol/iap/inc/iap_param.hpp`：512B 参数记录结构（双扇区日志：seq/state/active/pending/attempt/success/boot_ok/镜像信息/CRC16）。
- `protocol/iap/ld/`：boot.ld（0x08000000/128K）、app_a.ld（0x08020000/512K）、app_b.ld（0x080A0000/512K），自 Firmware 链接脚本复制修改，RAM 布局不变。
- `protocol/iap/CMakeLists.txt`：三个独立 target——`gd32f470_boot.elf`（-Os 固定优化）、`gd32f470_app_a.elf`/`gd32f470_app_b.elf`（同一源码两次链接，宏 `APP_IAP`+`APP_SLOT_A/B`）；POST_BUILD 生成 bin；烧录 target `flash-iap-boot`/`flash-iap-app-a`/`flash-iap-app-b`/`flash-iap`（整片 boot+app_a）。
- 根 `CMakeLists.txt`：新增 `option(BUILD_IAP OFF)`（默认关闭，不影响现有构建）+ `add_subdirectory(protocol/iap)`。
- `CMakePresets.json`：新增 `iap` preset（Release + BUILD_IAP=ON，binaryDir build/iap）。
- `protocol/iap/src/boot_main.cpp`：M1 占位入口（M3 填充完整 boot 流程）。

### Verified

- `cmake --preset iap` + 构建三个 target 通过：boot 1.3KB/128K、app_a/app_b 各 19.2KB/512K（3.67%）。
- 向量表验证：app_a Reset_Handler=0x080202E1、app_b=0x080A02E1、boot=0x08000235，均落在各自分区内。
- 现有 debug 构建重新配置 + 编译通过，行为零变化。

### Pending（后续里程碑）

- M2：参数区双扇区日志实现 + 镜像打包脚本 tools/iap_pack.py。
- M3：boot 完整流程（升级协议/状态机/跳转）。
- M4：app 侧接入（VTOR/上报/请求命令）。
- M5：硬件实测（升级/回滚/断电场景）。

## 2026-08-17 — inter_timer 易用性：频率自动计算 + 百分比占空比

### Added

- `interface/inc/inter_timer.hpp`：`TimerPortConfig` 新增 `frequency_hz` 字段——`>0` 时 init() 自动推导 prescaler/period（回写配置），`==0` 保持手动指定方式（向后兼容，聚合初始化顺序为 `periph, frequency_hz, prescaler, period, channels, channel_count`）。
- `interface/src/inter_timer.cpp`：新增 `_timer_clock_hz()`（按 GD32F4xx 的 TIMERSEL + APBxPSC 规则推导定时器时钟，区别于 STM32 固定 2×APB）与 `_auto_calc()`（O(1) 求最小分频，period 尽量大以保证分辨率，区分 16 位/32 位计数器）。
- `timer_port::pwm_set_duty_percent(channel, percent)`：百分比占空比 API（0.0~100.0，含边界钳位）。

### Notes（宿主机算法验证）

- 默认 240MHz 时钟下（AHB=240M, APB2=120M, APB1=60M）：APB1 定时器（TIMER1~6）时钟 = 2×60M = 120MHz，APB2 定时器（TIMER0/7）时钟 = AHB = 240MHz。
- `_auto_calc` 验证：1Hz~500kHz 全频段误差 < 0.0015%（多数精确 0.0000%）；16 位定时器 period 不溢出；32 位定时器（TIMER1/4）全分辨率；目标频率高于时钟时安全退化（psc=0, period=1）。

## 2026-08-17 — inter_timer 修复：timer5 IRQ 映射 / 高级定时器 MOE / DAC 共享 ISR

### Fixed

- `interface/src/inter_timer.cpp`：`_get_irq()` 的 timer5 映射原为 `#ifdef TIMER5_IRQn`，但 GD32F4xx 全系列无 `TIMER5_IRQn`（TIMER5 与 DAC 共享 `TIMER5_DAC_IRQn`），导致 `timer_id::timer5` 被错误指向 `TIMER6_IRQn`，且对应 `TIMER5_DAC_IRQHandler` 从未实现——一旦使用 timer5，中断落入默认 handler。
  - 映射改为 `TIMER5_DAC_IRQn`（保留 `#ifdef` 保护 + `[PORT]` 注释）。
  - 补实现 `TIMER5_DAC_IRQHandler`（DAC 欠载等共享事件通过 UP 标志判断后直接返回）。
- `interface/src/inter_timer.cpp`：`init()` 对高级定时器（timer0/timer7）补调 `timer_primary_output_config(ENABLE)`。GD32 高级定时器主输出（CCHP.POEN/MOE）默认关闭，不使能则 PWM 引脚无输出；通用定时器无 CCHP 寄存器，故仅对 timer0/timer7 调用。

## 2026-08-17 — Flash 布局重构（容量宏 + 扇区表）+ 板上实测确认

### Fixed

- `interface/src/inter_flash.cpp`：重新引入 `#include "gd32f4xx.h"`（位于 gd32f4xx_fmc.h 之前）。GD32 SDK 外设头无自带 `extern "C"` 保护，须借 gd32f4xx.h 的 extern "C" 块声明，否则 fmc_* 在 C++ 下被名字修饰导致链接失败（此前因 gc-sections 未暴露，首次实际调用即报错）。

### Changed

- `interface/inc/inter_flash.hpp`：新增 `FLASH_CAPACITY_KB` 容量宏（默认 2048，须与链接脚本 LENGTH 一致；支持 1024/2048/3072）。
- `interface/src/inter_flash.cpp`：扇区布局改为**编译期表驱动**（每 1MB bank：4×16K+1×64K+7×128K；3MB 尾部追加 4×256K），`erase()` 查表取真实扇区边界 + `static_assert` 校验表总大小。
  - 修复：旧公式把 bank1 起始（0x08100000~0x0811FFFF）的 16K/64K 扇区误判为 128K，导致安全区检查过度保守（合法擦除被拒）。
- 修正 inter_flash.hpp 文档示例：安全区需覆盖整扇区（init(0x08020000, 0x20000)）。

### Verified（板上实测，经用户同意在 bank1 0x08100000 做破坏性验证，验证后已擦除清理）

```
FLASH LAYOUT VERIFY (bank1 @ 0x08100000)
0x08100000=0xFFFFFFFF  0x08104000=0x22222222  0x08110000=0x33333333
VERDICT: 16K sector (per-bank layout: 4x16K+64K+7x128K)
[REFACTOR] 16K-only safe-area erase: OK (table-driven)
```

- 结论 1：标记法证实 bank1 起始为 16K 扇区（每 1MB bank = 4×16K+1×64K+7×128K，与 STM32F429 兼容布局一致，亦与师傅扇区表吻合）。
- 结论 2：重构后安全区仅覆盖单个 16K 扇区时擦除成功（旧公式会误判拒绝），表驱动修复生效。

## 2026-08-17 — SPI 软/硬接口统一 + W25Q128 硬件 SPI 板上验证

### Added

- 新增抽象基类 `interface/inc/inter_spi_bus.hpp`（`spi_bus`）：软 SPI（`soft_spi_bus`）与硬件 SPI（`spi_port`）统一接口（init/deinit/transfer/transfer_byte/cs_select/cs_deselect），设备驱动可透明切换软/硬 SPI。

### Changed

- `inter_spi.hpp` / `inter_soft_spi.hpp`：`spi_port`、`soft_spi_bus` 均继承 `spi_bus`，公共方法标记 `override`。
- SPI 设备驱动构造改为接收 `spi_bus&`（软/硬 SPI 通用）：w25q128 / max31855 / mcp23s17 / mcp_adc / mcp_dac / ms5611（`w25q128_flash` 因内部持有 `spi_port` 值成员保持不变）。
- `user/src/founction.cpp`：新增 `w25q128_hw_selftest()`（SPI4 硬件 SPI：CS=PF6, CLK=PF7, MISO=PF8, MOSI=PF9, AF5, MODE3, PSC2, MSB, 8bit），临时注释双板 CAN 会话测试代码（保留可恢复）。

### Verified（板上实测，DAP-Link 虚拟串口 /dev/ttyACM1，PA9/PA10 输出）

```
JEDEC ID    : EF 70 18  (W25Q128 OK)
Status Reg  : 0x00 (WIP=0)
-- Erase/Write/Readback @ 0xFF0000 --
Readback    : MATCH (32B verified)
RESULT      : PASS
```

注：/dev/ttyACM0 为另一块板（双板 CAN 测试对端）的串口，仍在输出旧 CAN 固件日志；本板串口为 /dev/ttyACM1。

## 2026-08-17 — device 设备驱动 STM32→GD32 移植（SPI / GPIO 时序 / 串口 / 构建）

### Changed

- SPI 驱动移植（7 个）：max31855 / mcp23s17 / mcp_adc / mcp_dac / ms5611 / w25q128 / w25q128_flash
  - `HAL_GetTick()` → `get_tick()`、`delay.hpp` → `systick.h`
  - `w25q128_flash` 的 `SpiPortConfig` 常量映射为 GD32（`SPI_BAUDRATEPRESCALER_*`→`SPI_PSC_*`、`SPI_POLARITY_*|SPI_PHASE_*`→`SPI_CK_PL_*_PH_*`、`SPI_FIRSTBIT_MSB`→`SPI_ENDIAN_MSB`、`SPI_DATASIZE_*`→`SPI_FRAMESIZE_*`、`GPIO_AF0_SPI1`→`GPIO_AF_5`）
- GPIO 位反转/时序驱动移植（15 个）：74hc138/74hc154/74hc590/hc4051/hc4052/hc4053/hc4067_gpio/shiftin/shiftout/x9c10x/dhtnew/drv8825/ds18b20/ds2401/hx711
  - `io_ctrl::init()/reinit()` 由 STM32 三参 `(mode, pull, speed)` 改为 GD32 四参 `(mode, pull, otype, speed)`
  - GPIO 常量映射：`GPIO_NOPULL/PULLUP/PULLDOWN`→`GPIO_PUPD_*`、`GPIO_SPEED_FREQ_*`→`GPIO_OSPEED_*`、`GPIO_PIN_RESET/SET`→`RESET/SET`
- 串口驱动移植：mhzco2（`usart_port<256>` 模板 → `usart_port`；`HAL_GetTick()` → `get_tick()`）

### Refactored

- `CMakeLists.txt`：device 源文件由显式逐条列出改为 `file(GLOB ... CONFIGURE_DEPENDS)` 自动收录 `device/src/*.cpp`，新增驱动无需手动登记。

## 2026-08-17 — device 设备驱动 STM32→GD32 移植（第二批 14 个器件）

### Changed

- 将 `#include "delay.hpp"` 改为 `#include "systick.h"`：ads1115 / as5600 / cht8305 / fram / gy521 / lcd1602
- 将 `HAL_GetTick()` 替换为 `get_tick()`：abp2 / as5600 / asdx / bh1750 / cht8305 / gy521 / lcd1602
- 新增 `#include "systick.h"`（原无 delay.hpp 但使用了 HAL_GetTick）：abp2 / asdx / bh1750
- `delay_us()` / `delay_ms()` 保持不变
- eeprom 的 WP 引脚初始化重写为 GD32 签名：
  `init(GPIO_MODE_OUTPUT_PP, GPIO_NOPULL, GPIO_SPEED_FREQ_LOW)`
  → `init(GPIO_MODE_OUTPUT, GPIO_PUPD_NONE, GPIO_OTYPE_PP, GPIO_OSPEED_2MHZ)`

### 涉及文件

- device/src/device_abp2.cpp / inc/device_abp2.hpp
- device/src/device_ads1115.cpp / inc/device_ads1115.hpp
- device/src/device_as5600.cpp / inc/device_as5600.hpp
- device/src/device_asdx.cpp / inc/device_asdx.hpp
- device/src/device_bh1750.cpp
- device/src/device_cht8305.cpp / inc/device_cht8305.hpp
- device/src/device_eeprom.cpp
- device/src/device_fram.cpp
- device/src/device_gy521.cpp / inc/device_gy521.hpp
- device/src/device_lcd1602.cpp

其余 4 个驱动（24lc1025 / cat24m01 / ds3232 / hc4067）经检查已无 STM32 HAL / GPIO 常量残留，无需改动。

## 2026-08-17 — device 设备驱动 STM32→GD32 移植（13 个 I2C 器件）

### Changed

- 将 `#include "delay.hpp"` 改为 `#include "systick.h"`（mcp4725 / mprls / sht31 / sht4x）
- 将 `HAL_GetTick()` 替换为 `get_tick()`（mprls / rotarydecoder / sht31 / sht4x）
- rotarydecoder 新增 `#include "systick.h"`（原无 delay.hpp 但使用了 HAL_GetTick）
- `delay_ms()` 保持不变

### 涉及文件

- device/src/device_mcp4725.cpp
- device/src/device_mprls.cpp
- device/src/device_rotarydecoder.cpp
- device/src/device_sht31.cpp
- device/src/device_sht4x.cpp

其余 8 个驱动（mcp23017 / mcp3424 / mcp9808 / pca9685 / pcf85263 / pcf8574 / pcf8591 / tca9548）经检查已无 STM32 HAL / GPIO 常量残留，无需改动。

## 2026-08-17 — ADC 接口完善：DMA 模式 + 轮询扫描确定性（参考官方例程与师傅 adc_bsp.c）

### 板载实测（GD32F470，ADC0 温度 CH16 + Vref CH17，全链路验证）

```
scan1-5: TEMP=1374~1376mV (37°C)  VREFINT=1186~1187mV   // 连续 5 次稳定
read_raw: CH16=1704, CH17=1473                           // 单通道正确
scan_raw: [0]=1705 (CH16), [1]=1471 (CH17)               // 硬件扫描交替正确
DMA: done=1 frame0=[1705,1472] frame9=[1706,1472]        // DMA 循环搬运持续刷新
```

### Fixed / Added（根因全部来自 GD32F470 与 STM32 的差异）

| # | 问题 | 修复 |
|---|------|------|
| 1 | **DMA 请求映射错误**：误用 GD32F407 的 ADC0→DMA0_CH0/SUBPERI4，DMA 配置全对但永远无请求 | GD32F450/470 映射为 **ADC0→DMA1_CH0/SUBPERI0**（官方例程 `ADC0_routine_sequence_with_DMA` + 师傅 DMA_BSP 双重印证）；ADC1→DMA1_CH2/SUBPERI1、ADC2→DMA1_CH1/SUBPERI2 |
| 2 | **EOCM 位语义与 STM32 相反**：默认 EOCM=0 只在序列结束时置位 EOC，轮询扫描只能读到序列最后一个通道 | 轮询路径显式 EOCM=1（每次转换置位 EOC）；DMA 路径保持 EOCM=0（官方例程默认，请求在序列末发出） |
| 3 | **ROVF 停滞**：EOCM=1/DMA 时启用溢出检测，残留转换无人读数据→ROVF 置位→ADC 转换停滞直到清除 | 排空逻辑：每次操作前清 ROVF + 排空在途 EOC |
| 4 | 单次模式（CTN=0）软件触发只转换一次，不支持多通道序列 | 轮询扫描改为：连续模式触发 + 读满 channel_count 次 EOC + 立即关闭 |
| 5 | 校准顺序（已有）：GD32F4xx 必须先 enable 再 calibrate | 保持 |
| 6 | rank 0-based（已有）：`adc_routine_channel_config` rank 从 0 起 | 保持 |

### 接口变更

- `AdcPortConfig`：DMA 默认改为 `dma1/CH0/SUBPERI0`（ADC0），注释标明三 ADC 映射
- `adc_port` 新增：`dma_start(buf, frames)` / `dma_done()` / `dma_stop()`（DMA 循环连续采集）
- `inter_dma` 新增：`set_subperipheral()` / `set_circular()`；`dma_start` 内先 subperipheral 选择再使能通道
- 轮询路径（read_raw/scan_raw）与 DMA 路径（dma_start/stop）自动切换 EOCM/CTN，互不干扰

---

## 2026-08-15 — CAN 1Mbps 极限速率双板实测

- ✅ 双板 CAN1 NORMAL **1Mbps**（60MHz/12TQ = prescaler 5，整除零误差）
- ✅ 20 秒双向各 40 帧，零丢失零错误（计数序列完整）
- 当前测试固件波特率配置为 1Mbps（`founction.cpp` 的 `can_bus.baudrate`）

---

## 2026-08-15 — CAN 双板实测（CAN1 NORMAL 500k，发现并修复过滤器时钟域 bug）

### Fixed（双板实测发现）

- 🔴 **CAN1 过滤器时钟域 bug**：过滤器寄存器位于 CAN0 外设地址空间，
  仅使能 RCU_CAN1 时过滤器配置写入无效（FW 恒 0，收不到任何帧）——
  使用 CAN1 时必须同时使能 RCU_CAN0 + RCU_CAN1

### 双板实测结果（两块 GD32F470 开发板，CAN1 @ PB12/RX PB13/TX，500kbps）

- ✅ 双向通信：A→B 与 B→A 各 40 帧/20s，**零丢失零错误**（计数序列完整连续）
- ✅ 总线 ACK 正常（NORMAL 模式发送全部 OK，无 FAIL）
- ✅ 标准帧 ID=0x100/8B 收发数据完整
- ✅ 两板独立计数互不干扰（验证帧来源正确）
- ℹ️ NORMAL 模式下 GD32 不自接收（每板 RX = 对方帧数），符合预期

### 说明

- 测试固件（founction.cpp）当前为 CAN 专注模式：跳过 INA228/W25Q/EXTI/波特率自检
  （`[[maybe_unused]]` 保留，恢复调用即可）；保留 init 后一行 DBG 诊断输出（FW/FCTL/FIFO）
- 双板烧录：probe-rs `--probe 2e3c:5745-0:xxxx` 区分两块板

---

## 2026-08-15 — CAN 接口调通（修复波特率/过滤器等 5 处 bug，参考王海涛 can_bsp.c）

### Fixed（调不通的根因）

| # | 问题 | 修复 |
|---|------|------|
| 1 | 🔴 16TQ 时 500k 需 prescaler=7.5 → 取整后实际 468.75k（-6.25% 超容限）| 默认 12TQ（BS1=8/BS2=3，采样点 75%），60MHz 下 125k/250k/500k/1M 全部整除；不能整除时 init 返回 false，可手动指定 prescaler（对齐师傅外部指定思路） |
| 2 | 🔴 `filter_bank` 无默认值（聚合初始化随机值，过滤器号可能越界）| 全字段默认值（C++20 指定初始化友好） |
| 3 | 🟡 `time_segment_1/2` 寄存器值 = TQ-1，直接强转会差 1TQ | 显式 `bs-1` 映射 |
| 4 | 🟡 发送等待完成前未清 MTF 标志（残留标志假成功）| 发送前清标志再等待 |
| 5 | 🟡 `auto_retrans=DISABLE`（师傅 ENABLE）| 改为 ENABLE |

### Changed

- `CanPortConfig` 增强：`bs1_tq/bs2_tq/prescaler`（0=自动）、全字段默认值
- `init()` → bool（整除失败/参数非法返回 false）
- GPIO 上拉 NONE（对齐 CAN 标准与参考实现）

### 板载验证（LOOPBACK 自发自收，无需外部节点）

- ✅ 500kbps 初始化成功（prescaler=10 整除）
- ✅ 标准帧 0x123/8B：收发 ID/长度/数据全匹配
- ✅ 扩展帧 0x1FFFFFFF/3B：收发匹配

---

## 2026-08-15 — EXTI 全 23 线支持（对齐参考实现 exti_bsp.c）

### Changed（inter_exti 能力对齐）

参考王海涛 exti_bsp.c（纯 C）对齐并增强：

- **全 23 线覆盖**：GPIO 线 0~15 + 系统线 LVD(16) / USB_FS(18) / 以太网(19) / USB_HS(20) /
  RTC 时间戳(21) / RTC 唤醒(22)，全部 ISR 内聚；RTC 闹钟(17) 由 inter_rtc 管理（避免重复定义）
- **线路互斥**：`init()` 返回 bool，同线已有实例时失败（PA0/PB0 同为线 0 防覆盖）
- **优先级可配**：`ExtiConfig.priority`（默认 5，0 为最高抢占级）
- **软件触发**：`software_trigger()`（调试用，等价参考实现 Soft_Interrupt）
- **上/下拉可配**：`ExtiConfig.pull`（按键场景）
- **线类型**：`exti_line_id` 枚举（gpio/lvd/usb_fs/ethernet/usb_hs/rtc_timestamp/rtc_wakeup）
- 非法端口 assert、`line()` 查询、init/enable 去重

### 板载自测（软件触发，无需外部信号）

- ✅ 线路互斥：PA0 注册成功、PB0（同线 0）注册失败
- ✅ ISR 链路：software_trigger ×3 → 回调执行 3 次（标志检查/清除/分发正确）
- ✅ LVD 系统线：配置成功

### 兼容性

- `init()` 返回类型 void → bool（无现有调用方，零适配成本）
- 构造函数/回调/enable/disable 接口不变

---

## 2026-08-15 — 软串口适配 usart_port 接口 + Modbus 板载验证

### Changed（inter_soft_uart 接口对齐 usart_port 标准）

- 新增 `send_data/send_data_it`（bool 阻塞发送，send_data_it 软串口退化为同步）、
  `buffer()`（复用 `uart_buffer_t` 类型，协议层零适配）、`periph()`（恒 0）、
  `rx_buf_capacity()`、`poll()`（主循环轮询接收 + 空闲判帧）
- 接收缓冲区零堆分配静态数组 256B（Modbus ADU 上限）

### 板载验证（PA9/PA10 软串口 115200，Modbus RTU 从站）

- ✅ 软串口 TX：自检输出全部经位反转发出，PC 正常接收
- ✅ 软串口 RX + 判帧 + Modbus 桥接：0x03/0x04 请求完整收发，CRC 校验通过，
  INA228 数据真实（BusV=5.242V, T=33.55°C）

### 轮询式软串口两个关键设计结论（踩坑记录）

1. **判帧阈值必须 > 主循环周期**：3.5 字符（304µs）< 主循环周期时会提前断帧，
   阈值取 20 字符（`FRAME_IDLE_CHARS`）
2. **主循环周期必须 < 字节时长**（87µs @115200）：`UpdateModbusRegisters` 软 I2C
   读取改为每 100ms 刷新（原每循环刷新导致主循环 ~1ms，poll 错过起始位）

### 测试模式说明（当前固件状态）

- 软串口占用 PA9/PA10（与硬件 debug_uart 冲突），`DebugPort_Init()` 临时禁用
- `ACTIVE_PROTOCOL` = PROTOCOL_MODBUS；Modbus 端口表挂软串口
- 恢复硬件模式：`founction.cpp` 恢复 `DebugPort_Init()`、`protocol_conf.hpp` 恢复协议宏、
  `modbus.cpp` 端口表改回 debug_uart

---

## 2026-08-15 — 新增软件串口（inter_soft_uart）

### Added

- `interface/inc/inter_soft_uart.hpp` + `interface/src/inter_soft_uart.cpp`：
  软件串口（GPIO 位反转 + DWT 周期计数精确时序），由用户提供的纯 C 版改写为 C++ inter 风格

**改造要点**：

- GPIO 寄存器直操作（`GPIO_BOP/BC/ISTAT`）→ `io_ctrl` 组合（铁律 2.5）
- C 函数 → `soft_uart_port` 类：Config POD、init/deinit 生命周期、禁拷贝、零堆分配
- 主频硬编码 240MHz → `SystemCoreClock` 变量（改时钟配置自动适配）
- `getc()` 超时从 NOP 忙等 → `get_tick()` 毫秒精度
- 保留核心算法：下降沿检测起始位、毛刺过滤、中点采样、溢出安全的 DWT 忙等

**API**：`putc/puts/write`（阻塞发送）、`try_getc`（非阻塞轮询）、`getc(byte, timeout_ms)`、
`set_baud`、`init/deinit`；波特率常量 `BAUD_9600 ~ BAUD_115200`

### 说明

- 原目录 `interface/soft_uart/`（含 `__MACOSX` 解压垃圾）待用户确认后删除
- 板载验证需用户提供 2 个空闲引脚（TX/RX），或 TX-RX 短接回环测试

---

## 2026-08-15 — W25Q 软 SPI 板载实测（inter_soft_spi 硬件验证）

### 板载实测结果（W25Q128JV-IM @ PF6/7/8/9，MODE0，delay=0）

- ✅ `inter_soft_spi` 全部功能实测通过：JEDEC ID 读取、状态寄存器、扇区擦除、页编程、读回比对（32B MATCH）
- ✅ 零延时（delay_us=0）下时序正确，无位错误
- ℹ️ 板载芯片实际为 **W25Q128JV-IM**（JEDEC `EF 70 18`，16MB），非 W25Q64（`EF 40 17`）；
  测试 ID 校验已按实际芯片更新
- ⚠️ 测试擦除了 0xFF0000 扇区（4KB）用于擦写回环验证

### Fixed

- `code/inc/systick.h`：添加 `extern "C"` 保护（C++ 文件直接 include 时链接失败的根治；
  此前仅通过 link.h 间接声明才可用）

---

## 2026-08-15 — 新增软件 SPI 总线（inter_soft_spi）

### Added

- `interface/inc/inter_soft_spi.hpp` + `interface/src/inter_soft_spi.cpp`：
  软件 SPI 总线（GPIO 位操作模拟，阻塞全双工）

**架构**（对齐 `inter_i2c_bus` 总线模式）：

- SCK/MOSI/MISO 共享总线由单对象独占管理（含 IRQ-safe 互斥锁）
- CS 双模式：内部单 CS（`cs_select/cs_deselect`）或外部 CS（多设备共享总线）
- 零堆分配：4 个 `io_ctrl` 成员对象

**API**：`init/deinit`、`transfer(tx,rx,len)`（tx/rx 可空可同址）、`transfer_byte`、
`cs_select/cs_deselect`、`set_mode(0..3)`、`set_bit_order`、`set_speed(delay_us)`、`lock/unlock`

**实现要点**：

- 位循环统一 4 种 SPI 模式（CPOL/CPHA 一张逻辑覆盖，无分支复制）
- `delay_us=0` 为空操作（`while(us--)` 先判后减），速率由 io_ctrl 位操作开销决定（约 1MHz 级）
- 设计决策：不增设设备层（SPI 设备协议差异大，无统一寄存器抽象，transfer 原语即最优抽象）

**参考**：Rob Tillaart SWSPI（mode/bitOrder 语义）、项目 inter_i2c_bus（互斥/延时/io_ctrl 组合）

### 验证

- 编译通过零警告 ✅
- 4 种模式时序已在实现注释中逐模式验证（采样沿/数据稳定顺序）
- 硬件回环测试（MOSI↔MISO 短接）待用户按需执行

---

## 2026-08-15 — Zed 任务二次优化（参考 STM32F407 工程风格）

### Changed

- `.zed/tasks.json` 按用户提供的 STM32F407 参考工程风格重构：
  - 新增 `Build (Release)` / `Build + Flash (Release)`（验证 release preset 可用）
  - 烧录命令增加 `--speed 4000`（实测 1.84s，比默认快约一倍）与 `--verify`
  - `Erase Chip` 使用 `--connect-under-reset` 增强擦除成功率
  - 并行编译 `-j$(nproc 2>/dev/null || echo 4)`（build preset 已含 jobs:16，双保险）
  - 全部任务 `reveal: always` + emoji 标签，chip 参数内联（换芯片搜索替换 `STM32F429ZI`）
  - 保留本项目特有任务：`Serial Monitor` / `GDB Server`(1337) / `GDB Client` / `Reset Target`

### Fixed（参考文件本身的笔误，未照搬）

- 参考中 `release + Flash` 误用 Debug 目录的 elf → 已修正为 `build/release/gd32f470_template.elf`
- 参考中 download/reset 芯片名不一致（ZETx/VGTx 混用）→ 统一为 STM32F429ZI

### 验证

- JSONC 解析通过（11 个任务）
- `--speed 4000` 实测烧录 1.84s verify OK ✅
- Release preset 全量编译通过 ✅（FLASH 21576B，比 Debug 小约 1.9KB）

---

## 2026-08-15 — Zed 任务优化：OpenOCD → probe-rs

### Changed

- `.zed/tasks.json` 全部重写，烧录/调试链路从 OpenOCD 迁移到 **probe-rs**：
  - 8 个 OpenOCD/JLink 任务 → 10 个任务（Build / Clean+Rebuild / Flash+Run / Flash Only /
    Build+Flash+Run / Reset / Erase / GDB Server / GDB Client / Serial Monitor）
  - 统一环境变量 `PROBE_RS_CHIP` / `PROBE_RS_PROTOCOL=swd` / `PROBE_RS_NON_INTERACTIVE`，
    换芯片/调试器只需改 env
  - 新增 `Serial Monitor`（python 直读 /dev/ttyACM1 @115200）
  - 任务按 `tags` 分类（build / flash / debug / monitor / danger）

### 关键发现（实测验证）

- probe-rs 0.31 芯片库**无 GD32F470**，但 GD32F4 的 FMC 与 STM32F4 寄存器兼容，
  实测 `--chip STM32F429ZI --protocol swd` 可正常连接、烧录（3.23s）、verify 通过、复位运行
- probe-rs `download` 烧录后**不会自动运行**，需 `probe-rs run`（烧录+运行）或 `probe-rs reset`
- probe-rs GDB server 默认端口 **1337**（非 OpenOCD 的 3333）

### 验证

- probe-rs download + verify + reset 全流程实测通过 ✅
- 板载程序运行正常（Vbus=3.29V 采样正常，5V 过压阈值正确未触发）

---

## 2026-08-15 — 构建升级 C++20（designated initializers）

### Changed

- `CMakeLists.txt`：`CMAKE_CXX_STANDARD` 17 → **20**，使能指定初始化（designated initializers），
  Config 结构体初始化可只写需要修改的字段，其余用默认值（与字段类型、字段位置无关）
- `user/src/founction.cpp`：`ina228_cfg` 从全字段 20 行聚合初始化简化为
  `{.alert_port = GPIOG, .alert_pin = GPIO_PIN_14}` 两行

### 影响范围

- 其他 INA 家族（219/226/260/3221）的 `Config` 同样受益：目前工程中无其他 Config 实例化点，无需额外修改
- 枚举 class 与数值型字段均不受影响（聚合初始化规则与字段类型无关）

### 验证

- 重新配置 + 全量编译零警告 ✅ FLASH 23496B (1.12%) / RAM 5456B (0.76%)
- DAP-Link 烧录 Verify OK ✅

---

## 2026-08-15 — INA228 板载验证测试（开发板实测）

### Added

- `user/src/founction.cpp`：INA228 注册报警引脚 **PG14**（`Config::alert_port=GPIOG, alert_pin=GPIO_PIN_14`），
  新增 `ina228_selftest()` 全功能自检（连接/ID → 校准 → 测量 → 阈值写入 → 报警触发/清除 → 寄存器 dump → 软复位恢复）
- 主循环每秒周期采样打印 `[INA228] Vbus/I/P/T/ALERT`

### Fixed

- `device_serial.cpp`：debug_uart 波特率 460800 → **115200**（对齐 AGENTS.md 规范）
- 自检输出适配 newlib-nano：`%lld` → `%ld` 分段打印（nano printf 不支持 64 位格式）
- 自检 BUVL 恢复值 100mV → 0（VBUS=0 时任何正阈值都会持续触发欠压）

### 板载实测结果（DAP-Link 烧录 + debug 串口 115200）

| 项目 | 结果 |
|------|------|
| I2C 连接 / MFR=0x5449 / DEV=0x2281 | ✅ |
| 校准 2A@1mΩ（err=0, LSB=3814nA） | ✅ 理论值 3814.7nA 一致 |
| 温度测量 33.6°C | ✅ 真实读数 |
| 欠压报警（BUVL=20V 触发→清除） | ✅ pin=LOW→HIGH |
| 温度过限报警（0°C 触发→清除） | ✅ pin=LOW→HIGH |
| 分流过压报警（SOVL=1mA） | ✅ 触发 |
| 过压/过功率报警 | ✅ 逻辑自洽（VBUS=0 时正确不触发） |
| 寄存器 dump / 软复位后恢复 | ✅ connected=1 |

⚠️ **硬件提示**：VBUS 恒为 0（芯片 I2C 正常、温度真实），疑似 INA228 的 IN+/IN- 未接入被测电源，需检查板级接线。

---

## 2026-08-15 — INA 家族驱动统一移植（INA219/228/260/3221）

### Refactored

按 INA226 已批准模式统一移植 4 款新增 INA 器件驱动，消除 STM32 HAL / Arduino 残留：

| 文件 | 清除的违规代码 |
|------|----------------|
| `device_ina219.cpp` | `HAL_GPIO_Init`/`HAL_GPIO_ReadPin`/`HAL_GetTick`/`delay.hpp`(不存在) |
| `device_ina228.cpp` | `HAL_GPIO_Init`/`HAL_GPIO_ReadPin` |
| `device_ina260.cpp` | `HAL_GPIO_Init`/`HAL_GPIO_ReadPin`/`HAL_GetTick`/`delay.hpp`(不存在) |
| `device_ina3221.cpp` | `HAL_GPIO_Init`/`HAL_GPIO_ReadPin`/`HAL_GetTick`/`delay.hpp`(不存在) |

- 4 个头文件私有成员 `_alertPort`/`_alertPinMask`/`_alertCfg` → `io_ctrl _alertPinAuto{0, 0}`（零堆分配）
- `init()` 报警引脚自动配置改用 `io_ctrl` 移动赋值 + `init(GPIO_MODE_INPUT, GPIO_PUPD_PULLUP)`
- `isAlertAsserted()` 改用 `_alertPinAuto.read()`，未配置时安全返回 false
- `waitConversionReady()`（219/260/3221）`HAL_GetTick` → `get_tick()`
- `#include "delay.hpp"` → `#include "systick.h"`（delay.hpp 不存在，原代码无法编译）

### Fixed

- `CMakeLists.txt`：删除 5 个不存在的 device 源文件引用（`device_debug/rs485/rs232/w25q128.cpp`），
  加入 `device_serial.cpp` 与 4 个 INA 文件，恢复可构建
- `modbus.hpp` / `modbus.cpp` / `string_pro.cpp` / `founction.hpp` / `founction.cpp`：
  `#include "device_debug.hpp"` → `"device_serial.hpp"`（文件已合并）
- `founction.cpp`：`i2c_bus.i2c_init()` → `i2c_bus.init()`（旧 API 残留）

### 兼容性

- 公开接口零变化：构造、`Config::alert_port/alert_pin`、`bindAlertPin(io_ctrl*)` 均不变
- 行为改善：报警引脚随 `io_ctrl` RAII 释放；重复 `init()` 换引脚自动释放旧引脚

### 验证

- 全量编译链接通过 ✅ FLASH 17712B (0.84%) / RAM 5448B (0.76%)，零警告（仅链接器 RWX 段提示，与本次无关）

---

## 2026-08-15 — INA226 去 STM32 HAL 残留 + SysTick 毫秒 tick 服务

### Refactored

- `device_ina226.cpp` 报警引脚自动配置改为使用 `io_ctrl` 成员对象（`_alertPinAuto`），
  替换直接调用 STM32 HAL GPIO（`HAL_GPIO_Init` / `HAL_GPIO_ReadPin`）的移植残留代码，
  符合铁律 2.1/2.5（SDK 调用仅限 `interface/src/`，GPIO 唯一抽象为 `io_ctrl`）
- 私有成员 `_alertPort` / `_alertPinMask` / `_alertCfg` 合并为 `io_ctrl _alertPinAuto{0, 0}`（零堆分配）
- `isAlertAsserted()` 优先外部 `bindAlertPin` 引脚，其次自动配置引脚（`is_initialized()` 判有效）
- 行为差异：INA226 析构时自动配置引脚随 `io_ctrl` RAII 释放（deinit 为模拟输入）；
  重复 `init()`/`setConfig()` 换引脚时移动赋值先释放旧引脚

### Added

- `systick.c`：新增 `get_tick()` 毫秒 tick 服务（SysTick 1kHz 递增计数），
  替代 STM32 `HAL_GetTick`；原 `delay_decrement()` 更名为 `tick_increment()`（语义与行为一致）

### Fixed

- `device_ina226.cpp` 补 `#include "device_serial.hpp"`（`dumpRawRegisters` 等使用 `debug_uart`）

### 涉及文件

- `device/inc/device_ina226.hpp` — 成员替换
- `device/src/device_ina226.cpp` — GPIO 重构 + `get_tick()` + include 修复
- `code/inc/systick.h` / `code/src/systick.c` — tick 服务
- `code/src/gd32f4xx_it.c` — `SysTick_Handler` 调用 `tick_increment()`

### 验证

- 相关 4 个源文件编译通过 ✅
- 全量链接因云端提交 `4ac82bf` 自身不完整（CMakeLists 引用 6 个不存在的 device 源文件）而失败，待修复

---

## 2026-06-13 — AT24Cxx 统一 EEPROM 驱动框架

### Added

实现 AT24Cxx 全系列 EEPROM 统一 C++ 驱动（`device_eeprom`），替代旧 C 驱动 `hard_eeprom`：

**覆盖型号**：24C02 / 24C04 / 24C08 / 24C16 / 24C32 / 24C64

**三层 API**：

| 层级 | 功能 | 典型 API |
|------|------|----------|
| L1 协议层 | 随机读写、字节操作、填充、ACK 探测、总线恢复 | `read()` / `write()` / `probe()` |
| L2 便利层 | 全片 dump、写入回读校验、空片检查、连续读取、WP 引脚 | `dump()` / `write_verified()` / `is_blank()` / `wp_enable()` |
| L3 结构化层 | 带 magic + version + CRC16 的配置块存取（模板） | `read_config<T>()` / `write_config<T>()` |

**架构要点**：

- 直接依赖 `inter_i2c_bus`（不依赖 `inter_i2c_dev`），因为 EEPROM 是"内存地址+数据"协议
- 24C04/08/16 的页寻址（页位嵌入 I2C 器件地址）自动处理
- 24C32/64 的双字节内存地址自动处理
- 跨页写入自动切分
- ACK 轮询替代固定延时（更快更可靠）
- 零堆分配，所有缓冲区栈上或调用方提供
- CRC-16-CCITT (0x1021) 用于配置块校验
- `detect_model()` 非破坏性自动型号检测

**数据安全性**：

- `_begin_write` / `_begin_read` 每个 ACK 超时均有返回检查
- `_wait_write_cycle` ACK 轮询 + 可配置超时
- `write_verified` 写入后逐字节回读比对
- `read_config` / `write_config` 含 magic(0xA5E5) + CRC16 双重校验
- I2C 通信失败自动调用 `bus_recovery()` 恢复总线
- WP 引脚支持：默认高电平写保护，写入时拉低

**设计决策**：

| # | 决策 | 理由 |
|---|------|------|
| 1 | 直接依赖 `inter_i2c_bus` | EEPROM 协议是"内存地址+数据"非"寄存器+数据" |
| 2 | `read_config` 开销 7 字节/块 | magic(2)+version(1)+len(2)+CRC16(2)，嵌入式开销可接受 |
| 3 | 构造时调用 `init()` | 对齐 INA226 模式，简化使用 |
| 4 | 页寻址型号：_begin_write 拼接 I2C 地址 | 最内层完成协议适配，上层 API 透明 |
| 5 | `detect_model` 非破坏性 | 保存+恢复地址 0 的原数据 |

### 涉及文件

- `device/inc/device_eeprom.hpp` — 重写：型号枚举、配置结构体、类声明、L3 模板
- `device/src/device_eeprom.cpp` — 重写：约 560 行完整实现

### 兼容性

- 旧 C 驱动 `hardware/src/hard_eeprom.c` / `hardware/inc/hard_eeprom.h` 不变，两套可共存
- `app/inc/private.hpp` 仍引用旧 `hard_eeprom.h`，不影响

### 验证

- 编译通过 ✅ 零错误零警告
- 资源占用：FLASH 20768B/2MB (0.99%), RAM 6272B/704KB (0.87%)

---

## 2026-06-12 — Modbus 协议栈安全性修复

### Fixed

修复 Modbus RTU 协议栈的安全性和合规性问题：

| # | 严重度 | 问题 | 修复 |
|---|--------|------|------|
| 1 | 🔴 | 功能码 0x01/0x02/0x05/0x0F 帧长校验失败时不返回异常响应，导致主机超时 | 统一添加异常响应，符合 Modbus 规范 |
| 2 | 🟡 | `Modbus_FillData()` 接收超长帧时截断处理，可能导致数据解析错误 | 改为丢弃超长帧，Modbus RTU ADU 最大 256 字节 |
| 3 | 🟡 | `Modbus_Init()` 未显式初始化 `send` 函数指针 | 添加 `Modbus.send = NULL` 显式初始化 |
| 4 | 🔴 | `device_debug.cpp` 波特率配置为 460800，与文档 115200 不一致 | 修正为 115200，与 `readme.md` 和测试脚本一致 |

### 涉及文件

- `protocol/modbus/src/modbus.cpp` — 异常响应 + 超长帧丢弃 + send 指针初始化
- `device/src/device_debug.cpp` — 波特率修正

### 验证

- 编译通过 ✅
- 资源占用：FLASH 17.7KB/2MB (0.84%)，RAM 5.8KB/704KB (0.81%)

---

## 2026-06-11 — 文件命名规范化重构

### Refactored

统一三个核心目录的文件命名风格，消除 `_port` / `_bsp` 等不一致后缀：

| 目录 | 规则 | 示例 |
|------|------|------|
| `interface/` | 去掉 `_port` 后缀，加 `inter_` 前缀 | `adc_port.hpp` → `inter_adc.hpp` |
| `bsp/` → `device/` | 去掉 `_port` 后缀，加 `device_` 前缀；目录重命名 | `debug_port.hpp` → `device_debug.hpp` |
| `hardware/` | 去掉 `_bsp` 后缀（如有），加 `hard_` 前缀 | `adc_bsp.h` → `hard_adc.h` |

涉及文件：
- `interface/inc/` 16 个头文件
- `interface/src/` 15 个源文件
- `device/inc/` 7 个头文件
- `device/src/` 7 个源文件
- `hardware/inc/` 13 个头文件
- `hardware/src/` 14 个源文件

连带更新：
- `CMakeLists.txt` 构建路径
- `readme.md` 文档路径
- `SKILL.md` 规范文档
- `changelog.md` 历史记录
- `app/`、`protocol/`、`user/` 中所有 `#include` 引用

## 2026-06-05 — 字符串协议重构（多端口 + 命令框架）

### 重构内容

将旧版 `string_pro.cpp`（硬编码 debug_uart，命令名含 \r\n）拆分为分层架构：

| 文件 | 职责 |
|------|------|
| `protocol/string/inc/str_cmd.h` | 命令框架头文件（新增） |
| `protocol/string/src/str_cmd.c` | 框架实现 + 命令表 + handler（新增） |
| `protocol/string/inc/string_pro.hpp` | 多端口桥接层接口（重写） |
| `protocol/string/src/string_pro.cpp` | 桥接层实现，遍历端口表、数据路由（重写） |

### 关键改进

| 改进 | 旧版 | 新版 |
|------|------|------|
| 多端口 | 硬编码 `debug_uart` | `string_ports[]` 端口表，与 Modbus 模式一致 |
| 数据路由 | 固定走 debug | 从哪个口来，从哪个口回 |
| 命令名 | 含 `\r\n` | 解析时自动去除 `\r\n` |
| 命令增删 | 改多处代码 | 只改 `str_cmd.c` 一个文件 |
| 参数支持 | 无 | 支持空格分隔的参数 |
| 错误提示 | 静默失败 | 返回 `Error: unknown command` |

### 关联修复

- `app/inc/protocol_conf.hpp`：修复 `PROTOCOL_MODBUS` 重复定义（第32行和第33行），添加 `PROTOCOL_STRING_CMD=2`

### usart_port 接口变更

修改 `send_data()` 和 `send_data_it()` 的返回类型从 `void` → `bool`，调用方可感知操作是否成功。

### 严重风险修复

| # | 严重度 | 问题 | 修复 |
|---|--------|------|------|
| 1 | 🔴 | `send_data()` 阻塞等待无超时保护，硬件异常时永久挂死 | 添加 `0xFFFF` 超时计数，超时返回 `false` |
| 2 | 🔴 | `send_data_it()` 保存用户指针，用户释放后 ISR 访问野指针 | 改为内部 `_tx_buf[TX_BUF_SIZE]` 静态数组 + `memcpy` 拷贝，ISR 只访问内部缓冲区 |

### 结构变更

| 维度 | 变更 |
|------|------|
| RAM 增加 | +256 字节（`_tx_buf[256]` 内部发送缓冲区） |
| ROM 增加 | 超时检查 + memcpy，约 +200 字节 |
| 接口 | `send_data()` 和 `send_data_it()` 返回 `void` → `bool` |

### 调用方适配

| 文件 | 调用处 |
|------|--------|
| `device/src/device_ina226.cpp` | 14 处 send_data 调用添加 `(void)` |
| `device/src/device_ina228.cpp` | 28 处 send_data 调用添加 `(void)` |
| `protocol/modbus/src/modbus.cpp` | 1 处 send_data_it 调用添加 `(void)` |
| `protocol/string/src/string_pro.cpp` | 1 处 send_data 调用添加 `(void)` |
| `user/src/founction.cpp` | 1 处 send_data 调用添加 `(void)` |

---

## 2026-06-04 — link.h 解耦 + interface 安全性修复

### link.h 解耦

移除 `link.h` 中的 C++ 专用驱动层（`#include "inter_usart.hpp"` 和 extern 声明），避免循环依赖。各 device 文件已自行包含 `inter_usart.hpp`。

### 安全性修复（3 项）

| # | 严重度 | 位置 | 问题 | 修复 |
|---|--------|------|------|------|
| 1 | 🟡 | `inter_i2c_bus.hpp:11` | 重复的 `#ifdef __cplusplus` | 移除重复的预处理指令 |
| 2 | 🟡 | `inter_timer.cpp:71` | `channel_count` 无边界检查，可能越界访问 `channels[4+]` | 添加 `count = min(channel_count, 4)` 限制 |
| 3 | 🟡 | `inter_usart.cpp:115` | 使用 `new[]` 堆分配缓冲区（历史遗留） | 添加 `[LEGACY]` 标记注释，新外设应使用 `usart_async_port` |

### 已确认无需修复

- `inter_inter_i2c_dev.cpp:101` — 已有空指针检查 `if (!Data || length == 0 || length > 8)`

---

## 2026-06-03 — Modbus 安全性加固 + 布局重构

### modbus_t 布局重构

`modbus_t` 头部字段对齐 `uart_buffer_t` 顺序，方便 `Modbus_FillData()` 直接对接：

```
modbus_t: [rx_buf*] [rx_len] [rx_flag] [tx_data*] [tx_len] [tx_idx] [tx_busy]
          [rx_buf_arr[256]] [tx_buf_arr[256]] [send]
```

- `Modbus_Init()` 中设置 `rx_buf` / `tx_data` 指针指向实际数组
- 函数处理器中 `tx_buf` → `tx_buf_arr` 适配新字段名

### 安全性修复（6 项）

| # | 严重度 | 位置 | 问题 | 修复 |
|---|--------|------|------|------|
| 1 | 🔴 | `FrameProcess:89` | `rx_len < 4` 检查被注释。`rx_len=0` 时 `CRC16(buf, 65534)` 越界 65KB | 恢复检查，同时清除 rx_flag |
| 2 | 🔴 | `Function03/04:130` | `quantity` 无上限，攻击者可传入 255 导致 `tx_buf_arr[3+510]` 越界写 | 增加 `MODBUS_MAX_REG_QUANTITY=125` 上限检查 |
| 3 | 🔴 | `Function16:218` | 无帧长校验。`quantity=100, rx_len=20` 时循环读超出 rx_buf | 增加帧长 + 数量 + 字节数三重校验 |
| 4 | 🟡 | `FrameProcess` | 成功后只清 `rx_len` 不清 `rx_flag`，后续每帧都重入 | 所有退出路径统一清除 rx_flag |
| 5 | 🟡 | `FillData:63` | `rx_len==0` 提前返回但不清 `buf->rx_flag`，可死循环 | 提前返回前清除 buf->rx_flag |
| 6 | 🟡 | `Function03/04/06` | 无帧长校验，短帧可越过 CRC 后访问越界 | 增加 `rx_len >= 8` 校验 |

### 接口统一

| 文件 | 改动 |
|------|------|
| `modbus.hpp` | `pc_receive_buffer_t` → `uart_buffer_t`；`#include "debug.hpp"` → `"device_debug.hpp"` |
| `modbus.cpp` | 端口表 `&pc_receive_buffer` → `debug_uart.buffer()`；`tx_buf` → `tx_buf_arr` |
| `inter_usart.cpp` | ISR 错误处理增加 `buf->rx_flag = 0`（配合 Bug 5 修复） |
| `CMakeLists.txt` | 剔除 `usart_async_port.cpp`，启用 `inter_usart.cpp`；注释 `modbus_port_cfg.cpp` |
| `founction.cpp` | 头文件 `usart_async_port.hpp` → `inter_usart.hpp` |
| `protocol.hpp` | `ACTIVE_PROTOCOL` = `PROTOCOL_MODBUS` |

### 验证

- 冒烟测试 ✅ 回环/二进制数据完整
- Modbus 0x06 ✅ 写1234 → 读回1234
- Modbus 0x04 ✅ INA228 温度数据 (31.9°C)
- 编译 FLASH: ~19KB, RAM: ~5KB

---

## 文件清单

```
修改:
  protocol/modbus/inc/modbus.hpp       — 类型桥接 + 布局重构
  protocol/modbus/src/modbus.cpp       — 安全加固 + 接口适配
  interface/src/inter_usart.cpp          — ISR bug 修复
  app/inc/protocol.hpp                  — 协议宏修正
  device/inc/device_debug.hpp                — 类型统一
  device/src/device_debug.cpp                — 使用 inter_usart
  user/src/founction.cpp                — INA228 集成
  code/inc/link.h                       — 类型统一
  CMakeLists.txt                        — 清理构建目标

新增:
  changelog.md                          — 本文档
```
