# Changelog

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
| `modbus.hpp` | `pc_receive_buffer_t` → `uart_buffer_t`；`#include "debug.hpp"` → `"debug_port.hpp"` |
| `modbus.cpp` | 端口表 `&pc_receive_buffer` → `debug_uart.buffer()`；`tx_buf` → `tx_buf_arr` |
| `usart_port.cpp` | ISR 错误处理增加 `buf->rx_flag = 0`（配合 Bug 5 修复） |
| `CMakeLists.txt` | 剔除 `usart_async_port.cpp`，启用 `usart_port.cpp`；注释 `modbus_port_cfg.cpp` |
| `founction.cpp` | 头文件 `usart_async_port.hpp` → `usart_port.hpp` |
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
  middleware/modbus/inc/modbus.hpp       — 类型桥接 + 布局重构
  middleware/modbus/src/modbus.cpp       — 安全加固 + 接口适配
  interface/src/usart_port.cpp          — ISR bug 修复
  App/inc/protocol.hpp                  — 协议宏修正
  bsp/inc/debug_port.hpp                — 类型统一
  bsp/src/debug_port.cpp                — 使用 usart_port
  user/src/founction.cpp                — INA228 集成
  code/inc/link.h                       — 类型统一
  CMakeLists.txt                        — 清理构建目标

新增:
  CHANGELOG.md                          — 本文档
```
