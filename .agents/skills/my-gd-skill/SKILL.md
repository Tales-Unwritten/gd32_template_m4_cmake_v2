---
name: my-gd-skill
description: >
  GD32F470 project-specific development skill. Defines architecture rules,
  peripheral wrapper conventions, ISR safety, buffer policies, and the
  mandatory change-control workflow. Applies to the gd32_template_m4_cmake_v2
  project. Complements the generic gd32-peripheral-wrapper specification.
disable-model-invocation: false
---

# GD32 外设 C++ 封装规范（项目定制版）

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
device/                           ← 设备驱动层
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

所有 GD32 SDK / 寄存器直接操作**只能存在于** `interface/src/`。

以下层级**严禁直接调用任何 `gd32f4xx_*.h` 中的函数**：

- `device/`、`app/`、`user/`
- `transport/`（未来）
- `middleware/`

```cpp
// ❌ 禁止（出现在 device/app/user 层的任何文件）
#include "gd32f4xx_usart.h"
usart_data_transmit(USART0, data);

// ✅ 正确：通过封装对象
debug_uart.send_data(data, len);
```

### 2.2 禁止隐藏的动态内存分配

**严禁**在以下位置使用 `new` / `delete` / `malloc` / `free`：

- ISR 中（绝对禁止）
- 高频数据路径
- 任何 transport 层代码

原因：内存碎片不可控，分配失败无法恢复，延迟不可预测。

> **已知例外**：`usart_port` 在 `init()` 中使用 `new[]` 分配接收缓冲区（1KB heap）。
> 此为历史遗留，短期内保留；新外设必须使用模板静态数组。

### 2.3 ISR 最小化原则

ISR 中**只允许**以下操作：

| 允许 | 禁止 |
|------|------|
| 读数据寄存器 | `printf` / 日志输出 |
| 清中断标志 | `malloc` / `free` |
| 写 buffer（lock-free） | STL 容器 |
| 递增计数器 | 自旋等待 |
| 设置 flag（volatile / atomic） | 复杂状态机 |
| 推送 event 到队列 | DMA 重配置 / 回调链 |

**ISR 必须是 < 5μs 的确定性代码**。

### 2.4 禁止 God Object

单类不能同时承担：硬件资源 + 中断调度 + 协议解析 + 设备逻辑。

本项目实际拆分：
```
interface/    usart_port, spi_port, inter_i2c_bus  ← 硬件抽象
device/          debug_port, rs485_port, ina228        ← 设备驱动
middleware/   modbus                                 ← 协议栈
```

### 2.5 io_ctrl 为唯一 GPIO 抽象

**所有 GPIO 引脚操作必须且只能通过 `io_ctrl` 对象完成。**

```cpp
// ❌ 禁止
gpio_bit_set(GPIOA, GPIO_PIN_5);
GPIO_BOP(GPIOA) = GPIO_PIN_5;

// ✅ 唯一正确方式
io_ctrl tx(GPIOA, GPIO_PIN_9);
tx.init(GPIO_MODE_AF, GPIO_PUPD_PULLUP);
tx.set_af(GPIO_AF_7);
```

---

## 三、分层职责（项目当前状态）

| 层级 | 目录 | 职责 | 
|------|------|------|
| **Firmware** | `Firmware/` | GD32 SDK + CMSIS，只读不修改 |
| **interface** | `interface/` | 外设资源抽象（时钟、GPIO、寄存器、IRQ） |
| **device** | `device/` | 设备驱动（INA228、debug/rs485/rs232 串口） |
| **middleware** | `middleware/` | Modbus RTU 协议栈 |
| **app** | `app/` | 多协议调度、PC 任务入口 |
| **user** | `user/` | 主循环、初始化 |

> `transport/`（DMA/Async）为规划层，当前未创建。

---

## 四、串口驱动体系（项目当前状态）

### 4.1 usart_port（已验证，当前生产中）

```cpp
usart_port debug_uart({ USART0, PA9, PA10, GPIO_AF_7, 115200, 256 });

// 阻塞发送
debug_uart.send_data(data, len);

// 中断发送（非阻塞）
debug_uart.send_data_it(data, len);

// 获取缓冲区指针（Modbus 桥接用）
uart_buffer_t *buf = debug_uart.buffer();
//  buf->rx_flag == 1  → 帧就绪
//  buf->rx_buf[]      → 数据指针
//  buf->rx_len        → 数据长度
```

### 4.2 uart_buffer_t（串口 ↔ Modbus 桥接）

```cpp
struct uart_buffer_t {
    uint8_t        *rx_buf;
    volatile uint16_t rx_len;
    volatile uint8_t  rx_flag;
    const uint8_t   *tx_data;
    volatile uint16_t tx_len;
    volatile uint16_t tx_idx;
    volatile uint8_t  tx_busy;
};
```

### 4.3 ISR 规范（项目实际）

- 所有 USART ISR 定义在 `interface/src/inter_usart.cpp` 内
- 使用全局端口映射表 `g_port_map[]` 分发
- `gd32f4xx_it.c` 中不包含任何 USART ISR

---

## 五、缓冲区规范（零堆分配）

### 5.1 新代码标准：模板静态缓冲

```cpp
template <size_t RX_SIZE, size_t TX_SIZE = 64>
class usart_async_port {
    uint8_t _rx_buf[RX_SIZE];   // 静态数组，编译时确定
    uint8_t _tx_buf[TX_SIZE];
};
```

### 5.2 当前实现：usart_port 使用 new[]

`usart_port` 当前使用 `new (std::nothrow)` 堆分配接收缓冲区（256 字节）。
这是历史遗留，短期保留。**新外设不得使用堆分配。**

### 5.3 Ringbuffer 必须 lock-free

- single producer / single consumer 模型
- `volatile` head/tail 指针（Cortex-M4 单核天然原子）
- 不关中断、不加锁

---

## 六、I2C 总线规范

### 软 I2C（inter_i2c_bus，当前使用）

- 基于 GPIO 位反转（io_ctrl 管理 SCL/SDA 引脚）
- 开漏输出 + 内部上拉
- `i2c_wait_ack()` 带有超时机制（1000 次循环）
- 互斥锁 `_busy` 标志位

### 硬件 I2C（i2c_hw_port，规划中）

- 替代软 I2C，降低 CPU 占用
- INA228 可迁移到硬件 I2C 提升稳定性

---

## 七、Modbus 协议栈（项目特有）

### 7.1 数据流

```
USART ISR → uart_buffer_t.rx_flag = 1 → Modbus_Task()
  → Modbus_FillData(buf, send_fn)
    → memcpy(Modbus.rx_buf_arr, buf->rx_buf, rx_len)
  → Modbus_FrameProcess()
    → CRC → address → function dispatch
    → Modbus_PortSend(tx_buf_arr)
      → debug_uart.send_data_it(buf, len)
```

### 7.2 modbus_t 布局

```
 rx_buf* | rx_len | rx_flag | tx_data* | tx_len | tx_idx | tx_busy
 [rx_buf_arr[256]] [tx_buf_arr[256]] [send]
```

头部字段对齐 `uart_buffer_t`，方便 `Modbus_FillData()` 直接对接。

### 7.3 端口表

```cpp
const modbus_port_t modbus_ports[] = {
    { debug_uart.buffer(), Debug_Send_IT },
};
```

### 7.4 功能码

| 码 | 名称 | 状态 |
|----|------|------|
| 0x03 | 读保持寄存器 | ✅ |
| 0x04 | 读输入寄存器 (INA228) | ✅ |
| 0x06 | 写单个寄存器 | ✅ |
| 0x10 | 写多个寄存器 | ✅ |

---

## 八、接口设计原则

### 8.1 公共接口必须平台中性

```cpp
// ❌ 禁止暴露平台类型
uint32_t periph;     // GD32 外设基址宏
IRQn_Type irq;       // GD32 中断枚举
FlagStatus status;   // GD32 状态枚举

// ✅ 推荐
enum class usart_id : uint8_t {
    usart0, usart1, usart2, uart3, uart4, usart5, uart6, uart7
};
```

### 8.2 平台映射隐藏在 .cpp

`interface/src/*.cpp` 内部做平台映射，`.hpp` 只暴露平台中性接口。

---

## 九、生命周期规范

- **构造函数**：只保存配置引用，不操作硬件、不分配内存
- **`init()`**：幂等（`if (_initialized) return;`），配置 GPIO、使能时钟、注册 ISR
- **`deinit()`**：安全可重复，可再次 `init()`

---

## 十、错误处理策略

| 级别 | 场景 | 处理方式 |
|------|------|----------|
| L1 可恢复 | 预期操作失败 | 返回 `bool` / 错误码 |
| L2 配置错误 | 不应发生但可继续 | Debug `assert()`；Release 安全 return |
| L3 硬件异常 | 不可恢复故障 | 清标志 + 丢帧 |

---

## 十一、命名与代码风格

| 元素 | 规则 | 示例 |
|------|------|------|
| 类名 | 全小写 + 下划线 | `usart_port` |
| 配置结构体 | 大驼峰 + Config | `UsartPortConfig` |
| 缓冲区结构体 | 全小写 + _t | `uart_buffer_t` |
| 方法名 | 全小写 + 下划线 | `send_data()` |
| 成员变量 | 下划线前缀 | `_cfg`, `_initialized` |
| 私有方法 | 下划线前缀 | `_enable_clock()` |
| 文件名 | 全小写 + 下划线 | `usart_port.hpp` |

### 文件夹与文件命名

所有文件夹和文件名必须使用**全小写 + 下划线**风格，严禁使用大写字母：

| 元素 | 规则 | 示例 |
|------|------|------|
| 文件夹名 | 全小写 | `interface/`、`hardware/`、`app/` |
| 文件名 | 全小写 + 下划线 | `inter_usart.hpp`、`device_debug.hpp` |
| 文档文件 | 全小写 | `changelog.md`、`readme.md` |

例外：`CMakeLists.txt`、`CMakePresets.json` 等构建系统标准文件名不受此限。

---

## 十二、当前外设封装状态

| 外设 | 类名 | 状态 |
|------|------|------|
| GPIO | `io_ctrl` | ✅ 生产 |
| USART | `usart_port` | ✅ 生产 |
| I2C 软 | `inter_i2c_bus` | ✅ 生产 |
| I2C 设备 | `inter_i2c_dev` | ✅ 生产 |
| INA228 | `INA228` | ✅ 生产 |
| SPI | `spi_port` | ✅ 生产 |
| Timer | `timer_port` | ✅ 生产 |
| CAN | `can_port` | ✅ 生产 (规划B) |
| 硬件 I2C | `i2c_hw_port` | ✅ 生产 (规划B) |
| ADC | `adc_port` | ✅ 生产 (规划B) |
| DAC | `dac_port` | ✅ 生产 (规划B) |
| DMA | `dma_port` | ✅ 生产 (规划B) |
| EXTI | `exti_port` | ✅ 生产 (规划B) |
| WDT | `wdt_port` | ✅ 生产 (规划B) |
| RTC | `rtc_port` | ✅ 生产 (规划B) |
| Flash | `flash_port` | ✅ 生产 (规划B) |
| W25Q128 | `w25q128` | ✅ 生产 (规划B) |
| Modbus | `modbus` | ✅ 生产 |
| RS485 串口 | `rs485_port` | ✅ 生产 |
| RS232 串口 | `rs232_port` | ✅ 生产 |

---

## 十三、参考实现

| 文件 | 核心模式 |
|------|----------|
| `interface/inc/inter_io_ctrl.hpp` + `.cpp` | 单引脚封装基类、RAII 生命周期 |
| `interface/inc/inter_usart.hpp` + `.cpp` | 外设封装 + ISR 内聚 + 全局映射表 |
| `interface/inc/inter_i2c_bus.hpp` + `.cpp` | io_ctrl 组合、软协议、锁模式 |
| `device/inc/device_ina228.hpp` + `device/src/device_ina228.cpp` | device 驱动，依赖 interface 对象 |
| `protocol/modbus/src/modbus.cpp` | Modbus 协议栈，纯 C/C++ 无硬件依赖 |

---

## 十四、开发工作流

### 编译 & 烧录

```bash
cmake --build --preset debug
openocd -f openocd/interface-daplink.cfg -f openocd/target-gd32f4xx.cfg \
  -c "program build/debug/gd32f470_template.elf verify reset exit"
```

Zed 内置 task: **Build + Flash DAPLink**。

### Modbus 验证

```bash
python3 tests/test_modbus.py /dev/ttyACM0
```

---

## 十五、变更控制与架构评审流程（最高优先级）

### 15.1 禁止未经分析直接修改代码

当用户提出：新需求、Bug 反馈、重构建议、性能优化需求、API 调整、架构修改时，**严禁直接生成修改后的代码**。

必须先执行：
1. 读取相关文件
2. 分析问题
3. 分析影响范围
4. 评估架构影响
5. 形成修改方案
6. **等待用户授权**

获得授权后方可实施修改。

### 15.2 必须先阅读相关代码

修改前必须读取：
- 所有将被修改的文件
- 所有依赖的接口头文件
- 关联的 device / app / middleware 代码
- `CHANGELOG.md`（了解变更历史）

### 15.3 先输出问题分析报告

修改前输出：
- 根因分析
- 影响范围（文件 / 接口 / 功能）
- 兼容性说明（是否破坏旧接口）

### 15.4 修改方案必须包含架构影响评估

方案中必须说明：
- 哪些文件需要修改
- 是否涉及接口变更
- 是否影响现有测试
- 是否需要新增测试

### 15.5 涉及架构变更必须暂停并请求授权

以下情况必须暂停，请求用户确认：
- 接口签名修改
- 新增/删除类
- 修改分层依赖关系
- 引入新依赖项

### 15.6 默认采用最小修改原则

优先选择：
- 改动最小
- 不修改接口
- 向后兼容的方案

### 15.7 修改后必须进行连锁影响检查

修改完成后检查：
- 所有调用方是否已适配
- 构建是否零错误
- 测试是否通过
- 接口文档是否需要更新

### 15.8 架构日志（Architecture Log）

涉及架构决策的变更必须在相关文件记录：
- **CHANGELOG.md** — 新增 / 修改 / 修复 / 重构 / 废弃
- 记录：变更原因、变更内容、影响范围、兼容性说明

### 15.9 项目变更日志（Change Log）

任何代码修改完成后必须更新 `CHANGELOG.md`。

记录：
- 新增
- 修改
- 修复
- 重构
- 废弃

格式：

```markdown
## [Unreleased]

### Fixed
- 修复 INA228 电流换算溢出问题

### Changed
- 优化 usart_port 接收缓冲区逻辑

### Refactored
- inter_i2c_bus 锁机制改为 critical_spinlock
```

### 15.10 过期日志清理规则

定期检查：

- `plan/`
- `docs/`
- `architecture_log.md`
- `CHANGELOG.md`

清理内容：
- 已经完成的临时计划
- 已废弃设计方案
- 无效 TODO
- 重复记录

要求：
- 保留有效历史
- 删除失效内容
- 避免日志无限膨胀

禁止：
- 删除仍具有追溯价值的架构记录
- 删除正式发布记录

### 15.11 最终执行流程（强制）

```
用户提出需求
  ↓
读取相关文件
  ↓
分析问题 + 影响范围
  ↓
形成修改方案
  ↓
等待用户授权
  ↓
获得授权
  ↓
实施修改
  ↓
自我测试检查（参照第十六章）
  ↓
连锁影响检查
  ↓
更新 CHANGELOG
  ↓
提交最终结果
```

**如果没有用户授权：禁止修改代码、禁止重构代码、禁止输出最终实现。**

本章节优先级高于所有代码生成规范。任何需求、Bug 修复、优化、重构、架构调整，均必须先完成分析与授权流程，未经授权不得实施修改。


## ## 十六、通用测试思维模型（Test Thinking Model）

开发新模块时必须按五维模型自查。Code Review 前确认各维度验收标准。

### 16.1 接口契约与边界

- **非法输入防御**：所有公共接口入口必须有参数校验；Debug 用 `assert()`，Release 返回错误码；严禁静默失败
- **幂等性**：`init()` 连续调用两次必须安全；`deinit()` 后调功能接口返回明确错误码
- **边界值**：空指针、零长度、超上限值、非法枚举必须拦截

### 16.2 内存与资源安全

- **零堆分配**：ISR 中**严禁** `new`/`malloc`；新外设必须使用模板静态数组
- **缓冲区**：`memcpy`/`memset` 长度严格校验；数组索引不越界
- **资源泄漏**：初始化失败时已申请的时钟/GPIO/中断必须回滚

### 16.3 实时性与确定性

- **ISR 极简**：< 5μs，**禁止**循环、等待、打印、复杂浮点运算
- **超时保护**：依赖外部状态的 `while` 等待必须有超时机制
- **复杂度**：主循环中**禁止** O(N²) 算法

### 16.4 并发与竞态

- **共享变量**：跨 ISR/主循环的变量必须声明 `volatile`
- **临界区**：多字节变量读写用临界区保护，关中断时间 < 10μs
- **状态机**：状态跳转互斥；函数可重入

### 16.5 故障恢复与鲁棒性

- I2C/SPI 超时后总线必须复位（Toggle SCL/SDA）
- USART 帧错误/溢出后接收状态机必须重置
- 单次通信失败**不得**导致系统挂死
- 错误后能自动恢复到初始可用状态

### 16.6 Code Review Checklist

| 检查项 | 说明 | 状态 |
|--------|------|------|
| 边界测试 | 已验证空指针、零长度、最大值、非法枚举 | ☐ |
| 内存安全 | 无堆分配、无栈溢出、无资源泄漏 | ☐ |
| 实时性 | ISR 无阻塞、WCET 已评估、等待有超时 | ☐ |
| 并发安全 | 共享变量已加 volatile、无竞态条件 | ☐ |
| 容错能力 | 通信超时已处理、错误后可恢复 | ☐ |
| 冒烟测试 | 开发板上基本功能无立即崩溃 | ☐ |
 |
