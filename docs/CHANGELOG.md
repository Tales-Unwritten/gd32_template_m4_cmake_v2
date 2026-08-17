# Changelog

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
