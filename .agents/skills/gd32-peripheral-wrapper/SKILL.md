---
name: gd32-peripheral-wrapper
description: >
  Strict development skill for wrapping GD32 firmware library peripherals into
  C++ objects. Use this when creating or modifying any interface-layer class
  (GPIO, I2C, USART, SPI, CAN, ADC, DAC, DMA, TIM, RTC, etc.) in the
  GD32F470 template project. Enforces the project's layered OOP architecture
  where every low-level peripheral is abstracted into a single, reusable C++
  class and all user code works exclusively through these object interfaces.
---

# GD32 外设 C++ 对象封装规范

## 一、核心理念

```
Firmware/ 库（GD32 官方 C API）
    ↓ 封装
interface/ 层（C++ 对象）
    ↓ 组合
bsp/ 层（设备驱动，使用 interface 对象）
    ↓ 调用
App/ + user/ 层（应用逻辑）
```

**铁律**：
- 所有对 `Firmware/` 下 GD32 SDK 函数的直接调用，**必须且只能**出现在 `interface/` 层。
- `bsp/`、`App/`、`user/` 层**严禁**直接 `#include` GD32 外设头文件（如 `gd32f4xx_usart.h`），只能使用 `interface/` 提供的 C++ 对象。
- 每个 GD32 外设类型（GPIO / I2C / USART / SPI / CAN / ADC / DAC / DMA / TIM / RTC / ...）**只封装一个类**，通过实例化覆盖所有通道。

---

## 二、层级职责

| 层级 | 目录 | 职责 | 语言 |
|------|------|------|------|
| **Firmware** | `Firmware/` | GD32 官方 SDK，只读，不修改 | C |
| **Interface** | `interface/` | 外设核心抽象，含 ISR | C++ |
| **BSP** | `bsp/` | 设备驱动，组合 interface 对象 | C++ |
| **App** | `App/` | 协议/任务调度 | C++ |
| **User** | `user/` | 主循环/初始化 | C++ |

---

## 三、可移植性设计规范（核心原则）

### 3.0 移植边界：只有 `.cpp` 需要重写

本项目的分层架构天然支持跨 MCU 移植。移植到新平台（如 STM32 / ESP32 / AT32 / MM32 / HC32 等）时：

```
                   ┌───────────────────────────┐
  【无需修改】      │  App/  + user/  + bsp/    │  ← 纯业务逻辑
                   ├───────────────────────────┤
                   │  interface/inc/*.hpp      │  ← 接口契约（公共头文件）
                   ├───────────────────────────┤
  【仅此需重写】    │  interface/src/*.cpp      │  ← 平台适配层（.cpp 实现）
                   ├───────────────────────────┤
  【替换 SDK】      │  Firmware/ → STM32_HAL/   │  ← 官方 SDK
                   └───────────────────────────┘
```

**移植工作量 = 仅重写 `interface/src/*.cpp` + 替换 `Firmware/`**，`.hpp` 头文件和所有上层代码完全不变。

### 3.1 头文件（.hpp）可移植性规则

公共头文件是对外暴露的**接口契约**，必须做到：

```cpp
// ============================================================
// @platform
//   移植到新 MCU 时，本 .hpp 文件【无需任何修改】。
//   所有平台差异封装在对应的 .cpp 实现文件中。
//   需要替换的内容：
//     - 平台相关 include: 将 #include "gd32f4xx.h" 替换为目标 SDK 头文件
//     - 类型别名:         FlagStatus, IRQn_Type 等替换或 typedef
//     - 外设基址宏:       USART0, SPI0 等替换为目标芯片对应宏
// ============================================================

#pragma once

// ── 平台相关 include（移植时替换此行） ─────────────────
#include "gd32f4xx.h"       // [PORT] 替换为 stm32f4xx.h / esp32.h 等

// ── 标准 C++（跨平台，无需修改） ───────────────────────
#include <cstdint>

#ifdef __cplusplus
// ... 类声明，不包含任何平台特定逻辑 ...
#endif
```

规则：
- 所有平台相关 `#include` 集中在文件顶部，用 `[PORT]` 注释标注
- Config 结构体字段使用 `uint32_t` / `uint16_t` 等标准类型，不依赖 GD32 宏
- 公共方法签名不暴露 `FlagStatus`、`IRQn_Type` 等平台特定类型（如需暴露，typedef 为中性类型）
- 头文件顶部必须包含 `@platform` Doxygen 注释，说明移植时需要替换什么

### 3.2 实现文件（.cpp）平台适配规则

`.cpp` 是**平台适配层**，所有 GD32 SDK 调用集中于此：

```cpp
// ============================================================
// @platform GD32F4xx（当前平台）
//   移植到新 MCU 时，本文件需要完整重写，但【公共接口签名不变】。
//   需要替换的映射：
//     - 时钟使能:   rcu_periph_clock_enable() → 目标平台时钟 API
//     - 外设初始化:  usart_deinit/baudrate_set → 目标平台 USART API
//     - 中断管理:   nvic_irq_enable → 目标平台 NVIC API
//     - GPIO 复用:   gpio_af_set → 目标平台 AF 配置 API
//     - ISR 入口:    USART0_IRQHandler → 目标平台中断向量名
// ============================================================
```

规则：
- 文件顶部必须包含 `@platform` 注释，列出当前平台和需要替换的 API 映射清单
- 所有 `rcu_`, `gpio_`, `nvic_`, `usart_` 等 GD32 前缀的函数调用**只能出现在此处**
- 平台相关的硬件操作宏（如缓存清理 `__DSB()`）用 `[PORT]` 标注

### 3.3 平台差异隔离模式

当同一个逻辑在不同平台实现方式不同时，使用条件编译 + 清晰注释：

```cpp
// ── 时钟使能 [PORT] ───────────────────────────────────
// GD32: rcu_periph_clock_enable(periph_clock)
// STM32: __HAL_RCC_USART1_CLK_ENABLE()

#ifdef GD32F4xx
    void _enable_clock() {
        const auto p = _cfg.periph;
        if      (p == USART0) rcu_periph_clock_enable(RCU_USART0);
        else if (p == USART1) rcu_periph_clock_enable(RCU_USART1);
        // ...
    }
#elif defined(STM32F4xx)
    void _enable_clock() {
        const auto p = _cfg.periph;
        if      (p == USART1) __HAL_RCC_USART1_CLK_ENABLE();
        else if (p == USART2) __HAL_RCC_USART2_CLK_ENABLE();
        // ...
    }
#else
    #error "Unsupported platform. Add your MCU _enable_clock() implementation."
#endif
```

规则：
- 平台宏命名统一：`GD32F4xx`、`STM32F4xx`、`ESP32` 等，与 CMSIS 设备宏一致
- 每个 `#ifdef` 块用 `[PORT]` 标注
- 最后必须有 `#else #error` 防止未定义平台静默失败

### 3.4 平台相关类型隔离

GD32 SDK 中有一些特定类型（`FlagStatus`、`ErrStatus` 等），在公共接口中应隔离：

```cpp
// ── 方式一：typedef 中性类型 ──────────────────────────
// [PORT] 移植时根据目标 SDK 调整
#ifdef GD32F4xx
    using port_status_t = FlagStatus;     // GD32: SET/RESET
    #define PORT_SET  SET
    #define PORT_RESET RESET
#elif defined(STM32F4xx)
    using port_status_t = GPIO_PinState;  // STM32: GPIO_PIN_SET/GPIO_PIN_RESET
    #define PORT_SET  GPIO_PIN_SET
    #define PORT_RESET GPIO_PIN_RESET
#endif

// ── 方式二：使用标准类型（推荐） ──────────────────────
// 公共接口直接用 bool / int / uint32_t
[[nodiscard]] bool read();               // 返回 true/false，而非 FlagStatus
void write(bool value);                  // 接收 true/false，而非 bit_status
```

规则：
- **优先用标准 C++ 类型**（`bool`, `int`, `uint32_t`）暴露公共接口
- 如果确实需要返回平台状态枚举，typedef 为 `port_status_t` 并在 Config 头文件中定义

### 3.5 移植检查清单

将本项目移植到新 MCU 时，按以下步骤操作：

- [ ] 1. 替换 `Firmware/` 目录为新 MCU 的官方 SDK
- [ ] 2. 修改 `interface/inc/` 中各 `.hpp` 的平台 `#include`（如 `gd32f4xx.h` → `stm32f4xx.h`）
- [ ] 3. 重写 `interface/src/` 中各 `.cpp` 的平台 API 调用
- [ ] 4. 更新 `io_ctrl.cpp` 中的 GPIO 操作函数映射
- [ ] 5. 更新各 `_port.cpp` 中的 ISR 入口函数名（如 `USART0_IRQHandler` → `USART1_IRQHandler`）
- [ ] 6. 更新 `cmake/` 工具链文件和 `CMakeLists.txt` 中的芯片宏定义
- [ ] 7. 更新 `openocd/` 调试配置文件
- [ ] 8. `bsp/`、`App/`、`user/` **完全不用动**
- [ ] 9. 编译验证

---

## 四、类设计模板

每个外设封装类必须遵循以下模式：

### 4.1 文件结构

```
interface/
├── inc/
│   └── <periph>_port.hpp     ← 类声明 + 配置结构体 + 缓冲区结构体
└── src/
    └── <periph>_port.cpp     ← 类实现 + ISR 入口函数
```

命名规则：`<periph>_port`，如 `usart_port`、`can_port`、`adc_port`。

### 4.2 配置结构体（Config Struct）

所有依赖外部输入的参数集合在一起，构造函数只接收一个 const 引用：

```cpp
struct <Periph>PortConfig {
    uint32_t periph;          // 外设基址，如 SPI0, ADC0
    // ... 引脚、波特率、DMA 通道等配置
};
```

要求：
- 纯 C 结构体（POD 类型），无构造函数，可直接 `{}` 初始化
- 字段命名与 GD32 库宏名保持语义一致
- 放在类声明头文件顶部

### 4.3 类声明骨架

```cpp
class <periph>_port {
public:
    explicit <periph>_port(const <Periph>PortConfig &cfg);
    ~<periph>_port();

    // 禁止拷贝
    <periph>_port(const <periph>_port &) = delete;
    <periph>_port &operator=(const <periph>_port &) = delete;

    // 生命周期
    void init();
    void deinit();
    [[nodiscard]] bool is_initialized() const noexcept;

    // 功能方法
    // ...

    // 查询
    [[nodiscard]] uint32_t periph() const noexcept;

private:
    // 时钟使能
    void _enable_clock();

    // IRQ 映射
    IRQn_Type _get_irq() const;

    // 端口注册（如果有 ISR）
    void _register_port();
    void _unregister_port();

    // 成员变量
    <Periph>PortConfig _cfg;
    io_ctrl             _pin1;      // 使用 io_ctrl 管理所有引脚
    io_ctrl             _pin2;      // ...
    bool                _initialized;
    // 缓冲区（内部拥有，对外暴露视图指针）
};
```

### 4.4 生命周期契约

```
构造函数:  只保存配置，不初始化硬件（constexpr 友好）
   ↓
init():    使能时钟、配置 GPIO、配置外设、注册 ISR、分配缓冲区
   ↓          （幂等：多次调用只有第一次生效）
使用:      调用功能方法
   ↓
deinit():  注销 ISR、禁用外设、反初始化 GPIO、释放缓冲区
   ↓          （可再次 init()）
析构函数:  调用 deinit()
```

### 4.5 引脚管理

**所有引脚必须使用 `io_ctrl` 对象管理**，严禁直接调用 `gpio_mode_set()` 等函数：

```cpp
// 正确
class spi_port {
    io_ctrl _sck;
    io_ctrl _mosi;
    io_ctrl _miso;
    io_ctrl _cs;
};

// 错误
class spi_port {
    uint32_t _sck_port;
    uint32_t _sck_pin;
    void _init_gpio() { gpio_mode_set(...); }  // 禁止！
};
```

`io_ctrl` 提供的能力：
- `init(mode, pull, otype, speed)` — 初始化
- `set_af(alt_func_num)` — 复用功能
- `high()` / `low()` / `toggle()` — 输出（硬件原子操作）
- `read()` / `read_output()` — 输入
- `deinit()` — 恢复为模拟输入
- `lock()` — 锁定配置
- `is_initialized()` — 状态查询

### 4.6 时钟使能

封装一个 `_enable_clock()` 私有方法，内部通过 if-else 链完成外设到时钟的映射：

```cpp
void spi_port::_enable_clock() {
    const auto p = _cfg.periph;
    if      (p == SPI0) rcu_periph_clock_enable(RCU_SPI0);
    else if (p == SPI1) rcu_periph_clock_enable(RCU_SPI1);
    else if (p == SPI2) rcu_periph_clock_enable(RCU_SPI2);
    else { /* assert */ }
}
```

注意：`io_ctrl::init()` 内部已自动使能 GPIO 时钟，无需重复。

---

## 五、ISR 中断处理规范

### 5.1 全局端口映射表

如果外设有多个实例且共享同一个中断向量表入口，必须使用**全局端口映射表**：

```cpp
namespace {
constexpr uint8_t PERIPH_MAX = 8;  // 根据芯片实际通道数
<periph>_port *g_port_map[PERIPH_MAX] = {};

uint8_t _port_index(uint32_t periph) {
    switch (periph) {
        case USART0: return 0;
        case USART1: return 1;
        // ...
        default: return PERIPH_MAX;
    }
}
}
```

### 5.2 注册 / 注销

```cpp
void <periph>_port::_register_port() {
    uint8_t idx = _port_index(_cfg.periph);
    if (idx < PERIPH_MAX) g_port_map[idx] = this;
}

void <periph>_port::_unregister_port() {
    uint8_t idx = _port_index(_cfg.periph);
    if (idx < PERIPH_MAX) g_port_map[idx] = nullptr;
}
```

### 5.3 ISR 处理函数

**所有 ISR 入口函数必须写在对应的 `.cpp` 文件中**，不要散落在 `gd32f4xx_it.c`：

```cpp
// 静态辅助函数（文件作用域）
static void _<periph>_isr(<periph>_port *port) {
    if (!port || !port->is_initialized()) return;
    // ... 中断处理逻辑
}

// ISR 入口（extern "C"）
extern "C" void SPI0_IRQHandler(void) {
    _<periph>_isr(g_port_map[_port_index(SPI0)]);
}
```

### 5.4 缓冲区设计

如果外设涉及数据收发，使用内部堆分配 + 对外暴露视图指针的模式：

```cpp
// 类内部
uint8_t  *_rx_heap_buf;    // new[] 分配
uint16_t  _rx_buf_size;

// 对外暴露的结构体（不含所有权）
struct <periph>_buffer_t {
    uint8_t        *rx_buf;
    volatile uint16_t rx_len;
    volatile uint8_t  rx_flag;   // 用户轮询此标志
    // ...
};
<periph>_buffer_t _buf;

// 公共方法
[[nodiscard]] <periph>_buffer_t *buffer() noexcept { return &_buf; }
```

---

## 六、互斥与锁

如果外设被多个上层对象共享（如一总线多设备），提供简单的自旋锁：

```cpp
class bus_type {
    volatile uint8_t _busy;

    void lock()   { while (_busy); _busy = 1; }
    void unlock() { _busy = 0; }
    uint8_t is_busy() { return _busy; }
};
```

每个共享该外设的上层对象在操作前 lock()，操作后 unlock()。

---

## 七、代码风格约束

### 7.1 命名

| 类型 | 规则 | 示例 |
|------|------|------|
| 类名 | 全小写 + 下划线 | `usart_port`, `spi_port`, `can_port` |
| 配置结构体 | 大驼峰 + Config 后缀 | `UsartPortConfig`, `CanPortConfig` |
| 缓冲区结构体 | 全小写 + _t 后缀 | `uart_buffer_t` |
| 方法名 | 全小写 + 下划线 | `send_data()`, `set_baudrate()` |
| 成员变量 | 下划线前缀 | `_cfg`, `_initialized`, `_tx` |
| 私有方法 | 下划线前缀 | `_enable_clock()`, `_get_irq()` |
| 文件 | 全小写 + 下划线 | `usart_port.hpp`, `usart_port.cpp` |

### 7.2 注释

- 类/结构体上方使用 Doxygen 风格 `@brief`
- 方法上方使用 `@brief`，参数用 `@param`
- 返回用 `[[nodiscard]]` 标注
- 条件编译块用 `#ifdef` 包裹平台差异代码

### 7.3 安全

- **Debug**: 使用 `assert()` 断言参数合法性
- **Release**: 使用早期 `if (!condition) return;` 安全返回
- 严禁在构造函数中分配堆内存或操作硬件
- 析构函数必须安全（检查已初始化状态再 deinit）
- 禁止拷贝，允许移动（如果资源唯一）或禁止移动（如果有全局注册）

---

## 八、扩展清单：待封装外设

按优先级排序，每完成一个在 `CMakeLists.txt` 中添加对应源文件：

| 优先级 | 外设 | 目标类名 | 备注 |
|--------|------|----------|------|
| P0 | SPI | `spi_port` | 组合 4 个 io_ctrl (SCK/MOSI/MISO/CS) |
| P0 | Timer | `timer_port` | PWM + 定时中断，多通道 |
| P1 | CAN | `can_port` | 组合 2 个 io_ctrl，含 CAN 协议封装 |
| P1 | ADC | `adc_port` | 多通道扫描 + DMA |
| P1 | DAC | `dac_port` | 单/双通道输出 |
| P2 | DMA | `dma_port` | 通用 DMA 通道管理 |
| P2 | I2C Hardware | `i2c_hw_port` | 硬件 I2C（替代软 I2C） |
| P2 | RTC | `rtc_port` | 日历 + 闹钟 |
| P3 | EXTI | `exti_port` | 外部中断，组合 io_ctrl |
| P3 | WDT | `wdt_port` | 独立看门狗 + 窗口看门狗 |

---

## 九、新增外设封装检查清单

创建新的外设封装类时，逐一确认以下项：

- [ ] 配置结构体定义为 POD（纯 C 结构体，无构造/析构/虚函数）
- [ ] 构造函数只接受 `const Config &`，不分配内存，不操作硬件
- [ ] 所有引脚用 `io_ctrl` 对象管理（不直接调用 gpio_ 函数）
- [ ] `init()` 幂等（多次调用只有第一次生效）
- [ ] `deinit()` 安全（检查初始化状态，释放所有资源）
- [ ] 析构函数调用 `deinit()`
- [ ] 禁止拷贝（`= delete`）
- [ ] `_enable_clock()` 私有方法封装外设时钟映射
- [ ] 如果外设有 ISR：全局映射表 + 注册/注销 + ISR 写在本 .cpp 文件
- [ ] 如果涉及收发缓冲区：堆分配 + 对外暴露视图指针
- [ ] 如果多对象共享：提供 lock/unlock
- [ ] Debug 用 `assert()`，Release 用 `if return`
- [ ] 在 `CMakeLists.txt` 的 `target_sources` 中添加新 .cpp 文件
- [ ] 在 `CMakeLists.txt` 的 `target_include_directories` 中确认 interface/inc 已存在
- [ ] 可移植性：`.hpp` 顶部有 `@platform` 注释，`.cpp` 顶部有 `@platform` + API 映射清单
- [ ] 可移植性：公共接口使用标准 C++ 类型（`bool`/`uint32_t`），平台特定类型已 typedef 隔离

---

## 十、反模式（严禁）

### 在 bsp/ 层直接调用 GD32 SDK 函数

```cpp
// bsp/src/ina228.cpp — 错误！
#include "gd32f4xx_i2c.h"
i2c_transfer(...);  // 禁止直接调用 SDK

// 正确：使用已封装的 device_i2c_dev 对象
```

### 在类外部编写 ISR Handler

```c
// code/src/gd32f4xx_it.c — 错误！
void USART0_IRQHandler(void) {
    // 应统一在 usart_port.cpp 中定义
}
```

### 绕过 io_ctrl 直接操作 GPIO

```cpp
// 任何 interface/ 以外的地方 — 错误！
gpio_bit_set(GPIOA, GPIO_PIN_5);

// 正确：通过 io_ctrl 对象
io_ctrl led(GPIOA, GPIO_PIN_5);
led.init(GPIO_MODE_OUTPUT, GPIO_PUPD_NONE);
led.high();
```

### 在构造函数中操作硬件

```cpp
// 错误！
can_port::can_port(const CanPortConfig &cfg) {
    rcu_periph_clock_enable(RCU_CAN0);  // 禁止！
    can_init(...);                       // 禁止！
}

// 正确：构造函数只保存配置，init() 中操作硬件
```

### 堆缓冲区由外部管理

```cpp
// 错误！
class usart_port {
    uint8_t *_rx_buf;  // 指向外部 buffer，生命周期不明确
};

// 正确：类内部 new[] / delete[]
```

---

## 十一、参考实现

以下现有文件是本规范的权威参考实现，新增封装类应尽可能与它们风格一致：

| 文件 | 展示的模式 |
|------|-----------|
| `interface/inc/io_ctrl.hpp` + `interface/src/io_ctrl.cpp` | 单引脚封装基类，所有上级类的引脚管理基础 |
| `interface/inc/usart_port.hpp` + `interface/src/usart_port.cpp` | 外设 + ISR + 缓冲区 + 全局映射表（完整范例） |
| `interface/inc/device_i2c_bus.hpp` + `interface/src/device_i2c_bus.cpp` | 双引脚组合 + 互斥锁 + 软协议实现 |
| `interface/inc/device_i2c_dev.hpp` + `interface/src/device_i2c_dev.cpp` | 设备层封装（持有 bus 指针，不持有总线所有权） |
| `bsp/inc/ina228.hpp` + `bsp/src/ina228.cpp` | BSP 层设备驱动（使用 interface 对象） |
