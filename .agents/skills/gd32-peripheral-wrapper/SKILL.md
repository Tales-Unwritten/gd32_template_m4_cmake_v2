---
name: gd32-peripheral-wrapper
description: >
  Industrial-grade embedded development skill for wrapping GD32 firmware
  peripherals into deterministic, portable, realtime-safe C++ interface
  objects. Use when designing or modifying any interface-layer peripheral
  abstraction (GPIO, USART, SPI, I2C, CAN, ADC, DMA, TIM, RTC, etc.).
  Enforces strict hardware abstraction, zero hidden allocation, ISR-safety,
  ownership clarity, and portable interface contracts.
---

# GD32 外设 C++ 封装规范（长期稳定版）

## 一、核心理念

本项目不是"把桌面 C++ 搬到 MCU"。所有设计决策**优先考虑 MCU realtime 特性**，其次才是面向对象优雅性。

```
Firmware SDK / Registers       ← 只读，不修改
        ↓
interface/                     ← 硬件资源抽象层（当前唯一适配层）
  ├── src/*.cpp                ← 平台相关实现（GD32 SDK 调用只在此）
  └── inc/*.hpp                ← 平台中性接口契约
        ↓
transport/                     ← [规划中] DMA / Async / Ringbuffer 扩展层
        ↓                          当前不依赖此层，新代码在 interface/ 完成
bsp/                           ← 设备驱动层
        ↓
app/ + user/                   ← 业务逻辑 + 初始化
```

**长期目标**：

| 特性 | 目标 |
|------|------|
| realtime-safe | 中断延迟可预测，ISR 无阻塞 |
| deterministic | 无隐藏堆分配，无碎片 |
| portable | 接口稳定，平台差异集中在 .cpp |
| zero hidden cost | 不用的功能不占 Flash/RAM |
| ownership clear | 谁分配、谁释放、谁独占 |
| ISR minimal | 中断中只做硬实时操作 |
| long-term maintainable | 单一职责，易于测试和替换 |

---

## 二、核心铁律（不可妥协）

### 2.1 SDK 调用边界

所有 GD32 SDK / 寄存器直接操作**只能存在于**：

- `interface/src/`

以下层级**严禁直接调用任何 `gd32f4xx_*.h` 中的函数**：

- `bsp/`、`app/`、`user/`
- `transport/`（未来）
- `middleware/`

```cpp
// ❌ 禁止（出现在 bsp/app/user 层的任何文件）
#include "gd32f4xx_usart.h"
usart_data_transmit(USART0, data);

// ✅ 正确：通过封装对象
_debug_uart->send_data(data, len);
```

### 2.2 禁止隐藏的动态内存分配

**严禁**在以下位置使用 `new` / `delete` / `malloc` / `free`：

- ISR 中（绝对禁止）
- 高频数据路径
- 任何 transport 层代码

原因：内存碎片不可控，分配失败无法恢复，延迟不可预测。

**缓冲区策略（详见第八章）**：新代码一律使用模板静态缓冲或外部注入 buffer。

### 2.3 ISR 最小化原则

ISR 中**只允许**以下操作：

| 允许 | 禁止 |
|------|------|
| 读数据寄存器 | `printf` / 日志输出 |
| 清中断标志 | `malloc` / `free` |
| 写 ringbuffer（lock-free） | STL 容器 |
| 递增计数器 | 自旋等待 |
| 设置 flag（volatile / atomic） | 复杂状态机 |
| 推送 event 到队列 | DMA 重配置 |
| | 回调链 |

**ISR 必须是 < 5μs 的确定性代码**。

### 2.4 禁止 God Object

一个类不能同时承担：硬件资源 + DMA 传输 + 中断调度 + 协议解析 + 设备逻辑。

必须按职责拆分：

```
spi_port          ← 硬件寄存器操作（interface/）
spi_dma_port      ← DMA 扩展（transport/，规划中）
spi_device        ← 设备驱动（bsp/）
w25q128           ← 具体芯片驱动（bsp/）
```

### 2.5 io_ctrl 为唯一 GPIO 抽象

**所有 GPIO 引脚操作，必须且只能通过 `io_ctrl` 对象完成。**

这是本架构最底层、最核心的约束，原因：

- `io_ctrl` 封装了寄存器级原子操作（BOP/BC/TG），消除读写竞争
- `io_ctrl` 管理时钟使能、模式锁定，上层类无需关心 GPIO 细节
- 换芯片时只改 `io_ctrl.cpp` 中的 GPIO 寄存器映射，所有上层外设类自动适配

依赖链：

```
io_ctrl  ← 唯一的 GPIO 原子操作类
    ↑
    │  （所有 interface 外设类通过组合 io_ctrl 管理引脚）
    │
usart_port._tx / ._rx        （串口 TX/RX 引脚）
spi_port._sck / ._mosi / ._miso / ._cs   （SPI 四线引脚）
can_port._tx / ._rx            （CAN 收发引脚）
i2c_hw_port._scl / ._sda       （硬件 I2C 引脚）
device_i2c_bus._scl / ._sda    （软 I2C 引脚）
adc_port._ain                  （模拟输入引脚）
```

**铁律**：

```cpp
// ❌ 绝对禁止：在任何地方直接调用 gpio_* 函数
gpio_mode_set(GPIOA, GPIO_MODE_AF, GPIO_PUPD_PULLUP, GPIO_PIN_9);
gpio_bit_set(GPIOA, GPIO_PIN_5);
GPIO_BOP(GPIOA) = GPIO_PIN_5;

// ✅ 唯一正确方式：通过 io_ctrl
io_ctrl tx(GPIOA, GPIO_PIN_9);
tx.init(GPIO_MODE_AF, GPIO_PUPD_PULLUP);
tx.set_af(GPIO_AF_7);
```

> 即使在新外设封装类（如 `spi_port`）内部实现中，也必须组合 `io_ctrl` 对象来管理引脚，不得绕过它直接操作 GPIO 寄存器。

---

## 三、分层职责

| 层级 | 目录 | 职责 | 依赖 |
|------|------|------|------|
| **Firmware** | `Firmware/` | 官方 SDK / CMSIS，只读不修改 | 无 |
| **interface** | `interface/` | 外设资源抽象（时钟、GPIO AF、寄存器操作、IRQ） | Firmware |
| **transport** | `transport/` | [规划中] DMA/Async/Ringbuffer 扩展 | interface |
| **bsp** | `bsp/` | 设备驱动（INA228、W25Q128 等） | interface |
| **app** | `App/` | 多协议调度、任务入口 | bsp |
| **user** | `user/` | 主循环、初始化 | app + bsp |

> **注意**：`transport/` 层为规划中的扩展层，项目当前尚未创建该目录。新外设封装先在 `interface/` 中完成，后续按需拆分 DMA/Async 功能。

---

## 四、接口设计原则（平台中立）

### 4.1 公共接口必须平台中性

**禁止**在 `.hpp` 公共接口中暴露平台特定类型：

```cpp
// ❌ 禁止
uint32_t periph;       // GD32 外设基址宏
IRQn_Type irq;         // GD32 中断枚举
FlagStatus status;     // GD32 状态枚举

// ✅ 推荐
enum class usart_id : uint8_t {
    usart0, usart1, usart2, uart3, uart4, usart5, uart6, uart7
};
```

公共接口只允许使用：

- `bool`
- `enum class`（整型底层）
- `uint8_t` / `uint16_t` / `uint32_t` 等 stdint 类型
- 自定义 POD 结构体

### 4.2 平台映射隐藏在 cpp

```cpp
// usart_port.cpp（平台适配层）
static uint32_t _to_hw_periph(usart_id id) {
    switch (id) {
        case usart_id::usart0: return USART0;
        case usart_id::usart1: return USART1;
        // ...
    }
}
```

**禁止**在公共头文件暴露：SDK 寄存器类型、SDK 宏、SDK 状态枚举。

### 4.3 可移植性目标

移植到新 MCU（STM32 / ESP32 / RP2040 等）时：

```
  无需修改:     app/ + user/ + bsp/
  极少修改:     interface/inc/*.hpp      （仅 platform #include + enum class 定义）
  完整重写:     interface/src/*.cpp      （全部平台 API 调用）
  替换 SDK:     Firmware/ → 目标 SDK
```

**现实主义**：不机械追求"hpp 永不修改"。不同架构差异极大，必要时 hpp 做最小适配是务实的。但公共接口的**语义**（方法名、行为契约）必须保持一致。

---

## 五、生命周期规范

### 5.1 构造函数

构造函数**只允许**：

- 保存配置引用
- 初始化成员变量为默认值

**禁止**：

- 时钟操作
- GPIO 操作
- 外设初始化
- `malloc` / `new`

```cpp
usart_port::usart_port(const UsartPortConfig &cfg)
    : _cfg(cfg)
    , _tx(cfg.tx_port, cfg.tx_pin)     // io_ctrl 构造只保存参数
    , _rx(cfg.rx_port, cfg.rx_pin)
    , _initialized(false)
{}
```

### 5.2 init()

`init()` 负责**一次性硬件初始化**：

- 使能外设时钟
- 配置 GPIO 为 AF 模式
- 配置外设寄存器
- 注册 ISR
- 分配/绑定缓冲区

要求：

- **幂等**：`if (_initialized) return;`
- 失败可安全重试
- 不阻塞（不等待硬件就绪超过 deadline）

### 5.3 deinit()

`deinit()` 负责**安全释放硬件资源**：

- 注销 ISR
- 禁用外设
- GPIO 恢复为模拟输入（最低功耗）
- 解绑 DMA（如有）

要求：

- 安全可重复调用：`if (!_initialized) return;`
- 可再次 `init()`

### 5.4 析构函数策略

两种场景区别对待：

```cpp
// 场景 A：作为成员变量的资源对象（推荐 RAII）
// io_ctrl 被包含在 usart_port 中，usart_port 销毁时自动清理引脚
class usart_port {
    io_ctrl _tx;   // 析构时自动调用 _tx.deinit()
    io_ctrl _rx;
    ~usart_port() { deinit(); }  // 先清理自身，成员随后自动清理
};

// 场景 B：顶层 static 对象（MCU 常态）
// main() 中的全局对象依赖 static lifetime，不依赖析构
// 如：static usart_port g_debug_uart(cfg);
// 这不需要也不应该依赖析构函数释放资源
```

**规则**：成员资源对象使用 RAII；顶层对象使用 static lifetime。两者不矛盾。

---

## 六、缓冲区规范（零堆分配）

### 6.1 新代码标准：模板静态缓冲

```cpp
template <size_t RX_SIZE, size_t TX_SIZE = 64>
class usart_async_port {
    uint8_t _rx_buf[RX_SIZE];   // 静态数组，编译时确定
    uint8_t _tx_buf[TX_SIZE];
    // ...
};
```

优点：零碎片、零分配失败、确定性延迟、编译时大小检查。

### 6.2 备选方案：外部注入 buffer

当缓冲区大小在运行时才确定（如从 EEPROM 读取配置）时：

```cpp
struct buffer_view_t {
    uint8_t *data;
    uint16_t size;
};

class usart_port {
    void bind_buffer(const buffer_view_t &rx, const buffer_view_t &tx);
};
```

外部拥有 buffer 内存（如全局数组），类只持有视图。

### 6.3 Ringbuffer 必须 lock-free

ISR 与主循环共享的 ringbuffer：

- **single producer / single consumer** 模型
- `volatile` head/tail 指针（Cortex-M3/4 单核天然原子读写）
- 不关中断、不加锁

### 6.4 现有代码迁移

当前 `usart_port` 使用 `new[]` 堆分配，属于历史遗留。迁移路径：

| 阶段 | 操作 |
|------|------|
| 短期 | 保持现有代码不变（已验证可用） |
| 中期 | 新增 USART 实例使用 `usart_async_port<RX_SIZE>` 模板版本 |
| 长期 | 将 `usart_port` 模板化，移除 `new[]`，存量 bsp 代码适配 |

---

## 七、并发与锁规范

### 7.1 裸机 safe-spinlock（推荐）

禁止原始的 `while(_busy);`（非原子、死锁风险）。使用：

```cpp
class critical_spinlock {
    volatile uint32_t _lock;
public:
    bool try_lock() {
        uint32_t mask = __get_PRIMASK();
        __disable_irq();                  // 关中断保证原子性
        if (_lock == 0) {
            _lock = 1;
            __set_PRIMASK(mask);          // 恢复原中断状态
            return true;
        }
        __set_PRIMASK(mask);
        return false;                     // 获取失败，不阻塞
    }
    void unlock() { _lock = 0; }
};
```

使用约束：

- 临界区极短（< 几 μs）
- 调用者处理 `try_lock()` 返回 false 的情况
- 不在 ISR 中使用 lock（ISR 本身不可重入，无需锁）

### 7.2 RTOS 场景

```cpp
SemaphoreHandle_t _mutex;
xSemaphoreTake(_mutex, pdMS_TO_TICKS(timeout_ms));
```

### 7.3 不推荐

```cpp
while (_busy);           // ❌ 非原子 + 死锁风险
while (!try_lock());     // ❌ 等价于忙等
```

---

## 八、寄存器访问策略

根据调用频率和上下文选择合适的方式：

| 路径 | 频率 | 推荐方式 | 示例 |
|------|------|----------|------|
| 初始化 | 一次 | SDK API（可读性强） | `usart_baudrate_set()` |
| 主循环配置变更 | < 1 Hz | SDK API | `gpio_output_options_set()` |
| 主循环轮询检查 | < 1 kHz | SDK API | `usart_flag_get(USART_FLAG_TC)` |
| ISR / 高频路径 | > 10 kHz | 直接寄存器操作 | `USART_RDATA(periph)` |
| GPIO 翻转（任意频率） | 任意 | 直接 BOP/BC 寄存器 | `GPIO_BOP(port) = pin` |

**直接寄存器操作的条件**：

- 必须在 `interface/src/` 内
- 必须注释说明寄存器名和数据手册章节
- 不得绕过封装的 `io_ctrl` 对象

---

## 九、ISR 处理规范

### 9.1 ISR 必须集中在对应的 cpp 文件

```cpp
// ❌ 禁止：散落在 gd32f4xx_it.c
void USART0_IRQHandler(void) { ... }

// ✅ 正确：定义在 usart_port.cpp 中
extern "C" void USART0_IRQHandler(void) {
    _usart_isr(g_port_map[_port_index(usart_id::usart0)]);
}
```

### 9.2 全局端口映射表模式

```cpp
namespace {
constexpr uint8_t UART_COUNT = 8;
usart_port *g_port_map[UART_COUNT] = {};

uint8_t _port_index(usart_id id) {
    return static_cast<uint8_t>(id);
}
}  // namespace
```

### 9.3 ISR 与业务逻辑分离

ISR 只负责**硬实时数据搬运**，业务逻辑在主循环：

```cpp
// ISR 中
static void _usart_isr(usart_port *p) {
    uint8_t data = usart_data_receive(p->_hw_periph);
    p->_rx_ringbuf.push(data);        // lock-free ringbuffer
    if (usart_flag_get(p->_hw_periph, USART_FLAG_IDLE)) {
        usart_data_receive(p->_hw_periph);  // 清 IDLE
        p->_rx_flag = 1;                    // 通知主循环
    }
}

// 主循环中
void protocol_task() {
    if (_uart->rx_flag()) {
        _uart->process_frame();       // 帧解析
    }
}
```

---

## 十、错误处理策略（三级制）

| 级别 | 场景 | 处理方式 | 示例 |
|------|------|----------|------|
| **Level 1** — 可恢复 | 预期内的操作失败 | 返回 `bool` / 错误码 | `i2c_freedom_read()` 返回 false |
| **Level 2** — 配置错误 | 不应发生，但可安全继续 | Debug: `assert()`；Release: 安全 return | `io_ctrl::init()` 重复调用 |
| **Level 3** — 硬件异常 | 不可恢复的硬件故障 | 记录错误标志，触发故障处理 | USART 帧/噪声错误 → 清标志 + 丢帧 |

### 规则

- **禁止** Release 版本因 `assert()` 而 crash
- **禁止** 在 ISR 中进入错误处理的阻塞分支
- 每个返回 bool 的方法调用者必须检查返回值

---

## 十一、io_ctrl 设计边界

`io_ctrl` 是所有上层类的**引脚管理基础**，但不得演化为 God Object：

### 保留功能

- 模式设置（`init` / `reinit` / `deinit`）
- 输出操作（`high` / `low` / `toggle` / `write`）
- 输入读取（`read` / `read_output`）
- 复用功能（`set_af`）
- 锁定（`lock`）

### 拒绝功能（由专用模块负责）

- 外部中断（EXTI）→ 由 `exti_port` 负责
- 模拟配置（ADC/DAC）→ 由 `adc_port` / `dac_port` 负责
- 低功耗模式 → 由 `pmu_port` 负责
- 引脚组操作（port-wide）→ 由 `gpio_port` 负责

### 迁移说明

当前 `io_ctrl` 的 `init()` 接收 `mode`/`pull`/`otype`/`speed` 参数，功能完整但边界清晰。**不做**功能删减，但不在此类上增加与单引脚无关的逻辑。

---

## 十二、事件模型

轮询 `rx_flag` 是裸机标准模型。但所有 async 外设必须**预留回调插槽**：

```cpp
using rx_callback_t = void (*)(void *user_data, const uint8_t *data, uint16_t len);

class usart_async_port {
public:
    void on_rx_frame(rx_callback_t cb, void *user_data = nullptr) {
        _rx_cb = cb;
        _rx_cb_data = user_data;
    }
private:
    rx_callback_t _rx_cb = nullptr;
    void          *_rx_cb_data = nullptr;
};
```

约束：

- **不强制**依赖 RTOS
- **不使用** `std::function`（代码膨胀 + 堆分配风险）
- 回调在非 ISR 上下文中调用（主循环或软件定时器）

---

## 十三、命名与代码风格

| 元素 | 规则 | 示例 |
|------|------|------|
| 类名 | 全小写 + 下划线 | `usart_port`、`spi_port`、`can_port` |
| 配置结构体 | 大驼峰 + Config | `UsartPortConfig`、`SpiPortConfig` |
| 缓冲区结构体 | 全小写 + _t | `uart_buffer_t`、`ringbuf_t` |
| 方法名 | 全小写 + 下划线 | `send_data()`、`set_baudrate()` |
| 成员变量 | 下划线前缀 | `_cfg`、`_initialized`、`_tx` |
| 私有方法 | 下划线前缀 | `_enable_clock()`、`_to_hw_periph()` |
| 文件名 | 全小写 + 下划线 | `usart_port.hpp`、`usart_port.cpp` |
| 注释 | Doxygen `@brief` / `@param` / `@platform` | — |

---

## 十四、待封装外设（优先级排序）

| 优先级 | 外设 | 目标类名 | 复用 io_ctrl | 备注 |
|--------|------|----------|-------------|------|
| **P0** | SPI | `spi_port` | 4 个 (SCK/MOSI/MISO/CS) | 阻塞模式优先 |
| **P0** | Timer | `timer_port` | 0~4 个 (PWM/捕获) | PWM + 定时中断 |
| **P1** | CAN | `can_port` | 2 个 (TX/RX) | 含 CAN 帧封装 |
| **P1** | ADC | `adc_port` | 0~N 个 (模拟输入) | 多通道扫描 |
| **P1** | DAC | `dac_port` | 0~2 个 (模拟输出) | — |
| **P2** | DMA | `dma_port` | 0 个 | 全局资源管理 |
| **P2** | I2C HW | `i2c_hw_port` | 2 个 (SCL/SDA AF) | 替代软 I2C |
| **P2** | RTC | `rtc_port` | 0 个 | 日历 + 闹钟 |
| **P3** | EXTI | `exti_port` | 1 个 (输入引脚) | 外部中断 |
| **P3** | WDT | `wdt_port` | 0 个 | IWDG + WWDG |

---

## 十五、新增外设检查清单

创建新的外设封装时逐项确认：

- [ ] 配置结构体为 POD（纯 C 结构体，无构造/析构/虚函数）
- [ ] 公共接口使用平台中性类型（`enum class` / `bool` / `uint32_t`）
- [ ] 构造函数只保存配置，不操作硬件、不分配内存
- [ ] 所有引脚使用 `io_ctrl` 对象管理，不直接调用 `gpio_*`
- [ ] `init()` 幂等（`if (_initialized) return;`）
- [ ] `deinit()` 安全可重复、可再次 `init()`
- [ ] 缓冲区为模板静态数组或外部注入，**不使用** `new[]`
- [ ] 如外设有 ISR：全局端口映射表 + ISR 定义在本 `.cpp`
- [ ] Ringbuffer（如有）为 lock-free single-producer/single-consumer
- [ ] 多对象共享资源时提供 `try_lock()` 模式
- [ ] Debug 用 `assert()`，Release 用 `if (!cond) return;`
- [ ] `.hpp` 顶部有 `@platform` 注释，`.cpp` 顶部有 `@platform` + API 映射清单
- [ ] 错误返回值为 `bool` 或 error code，调用者必须检查
- [ ] 回调插槽预留（裸函数指针 + `void* user_data`）
- [ ] 在 `CMakeLists.txt` 添加 `.cpp` 文件
- [ ] 在 `CMakeLists.txt` 确认 `interface/inc` 在 include path

---

## 十六、反模式（绝对禁止）

### 16.1 跨越封装层调用 SDK

```cpp
// ❌ bsp/src/ina228.cpp
#include "gd32f4xx_i2c.h"
i2c_transfer(...);
```

### 16.2 ISR 中阻塞或分配内存

```cpp
// ❌ ISR 中
printf("RX: %02X\n", data);
uint8_t *buf = (uint8_t *)malloc(256);
while (!dma_complete);
```

### 16.3 构造函数中操作硬件

```cpp
// ❌
can_port::can_port(const CanPortConfig &cfg) {
    rcu_periph_clock_enable(RCU_CAN0);
    can_init(CAN0, ...);
}
```

### 16.4 单一 God Object

```cpp
// ❌ 一个类包含：寄存器操作 + 中断 + DMA + 协议解析 + 超时重试
class super_uart { ... };
```

### 16.5 绕过 io_ctrl 直接操作 GPIO

```cpp
// ❌ 任何 interface/ 以外的地方
gpio_bit_set(GPIOA, GPIO_PIN_5);
GPIO_BOP(GPIOA) = GPIO_PIN_5;

// ✅
io_ctrl _led(GPIOA, GPIO_PIN_5);
_led.high();
```

### 16.6 公共接口暴露平台特定类型

```cpp
// ❌ .hpp 中
IRQn_Type _get_irq() const;
FlagStatus read();

// ✅ .hpp 中
bool read();

// ✅ .cpp 中（私有或在匿名命名空间）
IRQn_Type _get_irq() const;    // private 方法，.hpp 中声明 OK
```

---

## 十七、推荐架构模型

### USART 体系

```
usart_port              ← 资源层，同步阻塞（当前已实现）
    ↓（模板化静态 buffer）
usart_async_port<256>   ← transport 层，中断驱动 + ringbuffer（重构目标）
    ↓（组合 + 回调）
debug_uart              ← bsp 层设备实例
modbus_uart             ← bsp 层设备实例
```

### SPI 体系

```
spi_port                ← 资源层（待实现）
    ↓
spi_dma_port            ← transport 层（规划中）
    ↓
spi_device              ← bsp 设备基类
    ↓
w25q128 / lcd_st7789    ← 具体芯片驱动
```

---

## 十八、参考实现

以下文件是本规范的权威参考。新增代码应尽可能与它们风格一致：

| 文件 | 展示的核心模式 |
|------|--------------|
| `interface/inc/io_ctrl.hpp` + `interface/src/io_ctrl.cpp` | 单引脚封装基类、RAII 生命周期、平台适配层 |
| `interface/inc/usart_port.hpp` + `interface/src/usart_port.cpp` | 外设封装 + ISR 内聚 + 全局映射表 + Config 结构体 |
| `interface/inc/device_i2c_bus.hpp` + `interface/src/device_i2c_bus.cpp` | io_ctrl 组合使用、软协议实现、锁模式 |
| `interface/inc/device_i2c_dev.hpp` + `interface/src/device_i2c_dev.cpp` | 设备层（持有总线指针、不含总线所有权） |
| `bsp/inc/ina228.hpp` + `bsp/src/ina228.cpp` | BSP 层设备驱动（依赖 interface 对象，不触碰 SDK） |

---

## 十九、现有代码重构规划

为对齐本规范长期目标，以下现有代码需要分阶段调整：

| 代码 | 当前问题 | 严重度 | 计划 |
|------|----------|--------|------|
| `usart_port` 使用 `new[]` | 违反零堆分配原则 | 中 | 下次重构时模板化，存量 bsp 适配 |
| `device_i2c_bus::_busy` 自旋锁 | 非原子，理论死锁风险 | 低 | 替换为 `critical_spinlock`（七.1） |
| `usart_port` 同时含 ISR + 缓冲区 | 不符合单一职责，但裸机可接受 | 无 | 模板化时拆分 async 层 |
| `io_ctrl` 包含 `assert()` Debug 检查 | 符合规范，无需修改 | 无 | — |

**重构原则**：任何修改优先保证不引入 bug；每个重构独立 PR，单独测试。

---

## 二十、迁移到新 MCU 的步骤

1. 替换 `Firmware/` → 目标 SDK
2. 修改各 `.hpp` 中的 `#include "gd32f4xx.h"` → 目标 SDK 头文件
3. 重写各 `.cpp` 中的平台 API 调用（时钟、GPIO、外设初始化、ISR 入口名）
4. 更新 `cmake/` 工具链文件和 CMake 宏定义
5. 更新 `openocd/` 配置
6. `bsp/`、`App/`、`user/` 完全不动
7. 编译 → 烧录 → 测试
