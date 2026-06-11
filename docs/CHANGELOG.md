# Changelog

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
