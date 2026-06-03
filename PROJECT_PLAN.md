# GD32F470 项目开发计划

> **分支策略**：`planB` 开发 → 验证通过 → 合并到 `master`

---

## 一、项目全景

### 1.1 当前架构总览

```
┌────────────────────────────────────────────────────────────┐
│ main.c  ──► function_init()  ──► function_loop()          │  user/
├────────────────────────────────────────────────────────────┤
│ APP_PC_Task() → Protocol_Task() → StringCmd/Modbus        │  App/
├────────────────────────────────────────────────────────────┤
│ Modbus Frame/Link    │    CRC16/CRC8/CRC32                 │  middleware/
├────────────────────────────────────────────────────────────┤
│ INA228  │ INA226 │ Debug  │ RS485  │ RS232                │  bsp/
├────────────────────────────────────────────────────────────┤
│    usart_port    │ device_i2c_dev   │  device_i2c_bus      │  interface/
│         ↑        │        ↑         │       ↑              │
│      io_ctrl ────┴────────┴─────────┘   (所有引脚管理)     │
├────────────────────────────────────────────────────────────┤
│ GD32F4xx Standard Peripheral Library + CMSIS               │  Firmware/
└────────────────────────────────────────────────────────────┘
```

### 1.2 已完成模块（生产可用）

| 模块 | 文件 | 状态 |
|------|------|------|
| GPIO 封装 | `interface/io_ctrl.hpp/cpp` | ✅ 完成 |
| USART 封装 | `interface/usart_port.hpp/cpp` | ✅ 完成（含 `new[]`，待重构） |
| 软 I2C 总线 | `interface/device_i2c_bus.hpp/cpp` | ✅ 完成（含 `while(_busy)`，待重构） |
| I2C 设备 | `interface/device_i2c_dev.hpp/cpp` | ✅ 完成 |
| INA228 驱动 | `bsp/ina228.hpp/cpp` | ✅ 完成 |
| INA226 驱动 | `bsp/ina226.hpp/cpp` | ✅ 完成 |
| Debug 串口 | `bsp/debug_port.hpp/cpp` | ✅ 完成 |
| RS485 串口 | `bsp/rs485_port.hpp/cpp` | ✅ 完成 |
| RS232 串口 | `bsp/rs232_port.hpp/cpp` | ✅ 完成 |
| Modbus RTU | `middleware/modbus/` (4 文件) | ✅ 完成 |
| 字符串指令 | `App/protocol_string.cpp` | ✅ 完成 |
| 协议调度 | `App/protocol.cpp/hpp` | ✅ 完成 |
| EEPROM 持久化 | `App/private.cpp/hpp` + `Hardware/eeprom.c` | ✅ 完成 |
| 主循环 | `user/founction.cpp/hpp` | ✅ 完成 |
| 测试脚本 | `tests/test_modbus.py`, `test_string.py` | ✅ 完成 |

### 1.3 已知技术债务

| 问题 | 文件 | 严重度 | 计划 |
|------|------|--------|------|
| `usart_port` 使用 `new[]` 堆分配 | `interface/src/usart_port.cpp` | ~~中~~→ ✅ | Phase 1.1 已完成 |
| `device_i2c_bus` 用 `while(_busy)` 自旋锁 | `interface/src/device_i2c_bus.cpp` | 低 | Phase 1.2 |
| `Hardware/` 有 12 个旧 C 驱动未编译 | `Hardware/src/*.c` | 信息 | 逐步迁移或清理 |

---

## 二、分阶段计划

### Phase 1：基础重构（当前阶段 — de-risk）

| 序号 | 功能 | 目标文件 | 说明 |
|------|------|----------|------|
| **1.1** | 重构 `usart_port` | `interface/usart_port.hpp/cpp` → 新增 `interface/usart_async_port.hpp/cpp` | 消除 `new[]`，模板静态 buffer；旧 `usart_port` 作为同步阻塞版本保留 |
| **1.2** | 重构 `device_i2c_bus` 锁 | `interface/device_i2c_bus.hpp/cpp` | `while(_busy)` → `critical_spinlock`（Skill 七.1） |

**完成标志**：编译 0 error/0 warning，运行现有测试全部通过。

---

### Phase 2：核心外设封装

| 序号 | 功能 | 目标类名 | 依赖 | 测试方式 |
|------|------|----------|------|----------|
| **2.1** | SPI 主机 | `spi_port` | `io_ctrl` × 4 | W25Q128 JEDEC ID 回读 ✅ EF7018 PASS |
| **2.2** | 定时器/PWM | `timer_port` | 无（纯寄存器） | 示波器测量 PWM 输出 |

**2.1 — spi_port 详细设计**：

```
接口文件： interface/inc/spi_port.hpp
实现文件： interface/src/spi_port.cpp

配置结构体：
  struct SpiPortConfig {
      spi_id    periph;        // enum class: spi0/spi1/spi2
      uint32_t  sck_port;      // SCK 引脚端口
      uint32_t  sck_pin;       // SCK 引脚掩码
      uint32_t  mosi_port;
      uint32_t  mosi_pin;
      uint32_t  miso_port;
      uint32_t  miso_pin;
      uint32_t  cs_port;       // 可选，软件 CS
      uint32_t  cs_pin;
      uint32_t  af;            // AF5 for SPI0/1/2
      uint32_t  baudrate_presc; // SPI_PSK_2 ~ SPI_PSK_256
      uint32_t  cpol;          // SPI_CK_PL_LOW/HIGH
      uint32_t  cpha;          // SPI_CK_PH_1EDGE/2EDGE
      uint32_t  frame_format;  // MSB/LSB
      uint16_t  frame_size;    // 8 or 16
  };

公共方法：
  void     init();
  void     deinit();
  bool     is_initialized() const noexcept;

  // 阻塞传输（裸机标准模式）
  void     transfer(const uint8_t *tx, uint8_t *rx, uint16_t len);
  uint8_t  transfer_byte(uint8_t data);       // 单字节快捷方法

  // CS 手动控制（用于多设备共享 SPI）
  void     cs_low();
  void     cs_high();

  spi_id   periph() const noexcept;
```

**2.2 — timer_port 详细设计**：

```
接口文件： interface/inc/timer_port.hpp
实现文件： interface/src/timer_port.cpp

配置结构体：
  struct TimerPortConfig {
      timer_id  periph;         // enum class: timer0..timer7
      uint32_t  prescaler;      // 预分频值
      uint32_t  period;         // 自动重装载值
      // PWM 通道配置（可选）
      struct {
          uint32_t port;
          uint32_t pin;
          uint32_t af;
      } channels[4];
      uint8_t   channel_count;
  };

公共方法：
  void     init();
  void     deinit();

  // 基础定时
  void     start();
  void     stop();
  uint32_t counter() const;

  // PWM
  void     pwm_set_duty(uint8_t channel, uint32_t value); // 0 ~ period

  // 中断（预留回调插槽）
  using timeout_cb_t = void (*)(void *);
  void     on_timeout(timeout_cb_t cb, void *user_data = nullptr);
```

**完成标志**：编译通过，测试脚本在硬件上验证通过。

---

### Phase 3：通信外设

| 序号 | 功能 | 目标类名 | 说明 |
|------|------|----------|------|
| **3.1** | CAN 总线 | `can_port` | `io_ctrl` × 2，含 CAN 帧收发 |
| **3.2** | 硬件 I2C | `i2c_hw_port` | `io_ctrl` × 2，替代软 I2C |

---

### Phase 4：模拟与系统外设

| 序号 | 功能 | 目标类名 | 说明 |
|------|------|----------|------|
| **4.1** | ADC | `adc_port` | 多通道扫描 |
| **4.2** | DAC | `dac_port` | 单/双通道 |
| **4.3** | DMA | `dma_port` | 全局资源管理 |
| **4.4** | RTC | `rtc_port` | 日历+闹钟 |
| **4.5** | 外部中断 | `exti_port` | `io_ctrl` × 1 |
| **4.6** | 看门狗 | `wdt_port` | IWDG + WWDG |

---

## 三、开发流程（每次迭代）

```
┌──────────────────────────────────────────────────┐
│  1. 阅读开发计划 → 确认当前要做的功能            │
│  2. 检查 Skill 规范 → 确认设计约束               │
│  3. 先写 .hpp 接口草案 + 测试脚本                │
│  4. 评审接口 → 稳定后开始实现 .cpp               │
│  5. 编译 → cmake --build build/debug             │
│  6. 烧录 → 真实硬件测试                          │
│  7. 运行测试脚本 → 全部通过                      │
│  8. 更新 CMakeLists.txt + README.md              │
│  9. git commit → push planB                      │
│ 10. planB 验证通过 → merge master                │
└──────────────────────────────────────────────────┘
```

---

## 四、当前状态：准备开始 Phase 1

| 项目 | 状态 |
|------|------|
| 分支 | `planB`（当前工作分支） |
| 编译 | ✅ 零错误零警告 |
| 测试 | ✅ Modbus + String 全通过 |
| 待重构 | `usart_port`（Phase 1.1） |

**下一步**：开始 Phase 1.1 — 重构 `usart_port`，消除 `new[]`。

---

## 四、变更日志 (planB)

### 2026-06-03 — Modbus 安全性加固 + 布局重构

**modbus_t 布局重构**
- `modbus_t` 头部字段对齐 `uart_buffer_t` 顺序（`rx_buf* | rx_len | rx_flag | tx_data* | ...`）
- 实际存储数组移至 `rx_buf_arr[256]` / `tx_buf_arr[256]`
- `Modbus_Init()` 中设置 `rx_buf` / `tx_data` 指针指向实际数组
- 目的：`Modbus_FillData()` 可直接从串口 `uart_buffer_t` 对接字段

**安全性修复（6 项）**
| # | 位置 | 修复内容 |
|---|------|----------|
| 1 | `FrameProcess` | 恢复 `rx_len < 4` 检查，防止 CRC16 参数回绕越界 |
| 2 | `Function03/04` | 增加 `quantity` 上限 `MODBUS_MAX_REG_QUANTITY=125` |
| 3 | `Function16` | 增加帧长、数量、字节一致性三重校验 |
| 4 | `FrameProcess` | 所有退出路径 + 最终统一清除 `rx_flag` |
| 5 | `FillData` | `rx_len==0` 提前返回时清除 `buf->rx_flag`，防死循环 |
| 6 | `Function03/04/06` | 增加 `rx_len >= 8` 帧长校验 |

**接口统一**
- `modbus.hpp`: `pc_receive_buffer_t` → `uart_buffer_t`
- `modbus.hpp`: `#include "debug.hpp"` → `"debug_port.hpp"`
- `modbus.cpp`: 端口表 `&pc_receive_buffer` → `debug_uart.buffer()`
- `usart_port.cpp`: ISR 错误处理增加 `buf->rx_flag = 0`
- CMakeLists: 剔除 `usart_async_port.cpp`，启用 `usart_port.cpp`

**验证结果**
- 冒烟测试 ✅ 回环/二进制完整
- Modbus 0x06 ✅ 写1234→读回1234
- Modbus 0x04 ✅ INA228 Temp=31.9°C
- ACTIVE_PROTOCOL = PROTOCOL_MODBUS ✅

---

## 五、当前状态

| 项目 | 状态 |
|------|------|
| 分支 | `planB`（当前工作分支） |
| 编译 | ✅ 零错误 |
| Modbus 0x03/0x06 读写 | ✅ |
| Modbus 0x04 INA228 数据 | ✅ |
| 安全性审查 | ✅ 已加固 |
| usart_port 冒烟 | ✅ |
