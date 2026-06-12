# AGENTS.md

## 构建命令

```bash
# 配置（首次或修改CMakeLists.txt后）
cmake --preset debug

# 编译
cmake --build --preset debug

# 烧录（DAP-Link）
cmake --build build/debug --target flash-daplink

# 烧录（J-Link）
cmake --build build/debug --target flash-jlink

# 启动调试服务器
cmake --build build/debug --target debug

# 连接GDB
cmake --build build/debug --target gdb

# 清理
cmake --build build/debug --target clean
```

## 测试

```bash
# 需要硬件连接，串口设备如 /dev/ttyACM0
python3 tests/test_modbus.py /dev/ttyACM0
python3 tests/test_string.py /dev/ttyACM0
```

## 架构分层

- `Firmware/` - GD32 SDK，只读不修改
- `interface/` - 硬件抽象层，**唯一允许调用GD32 SDK的地方**
- `device/` - 设备驱动（传感器、串口等）
- `protocol/` - 协议栈（Modbus、字符串指令、HEX）
- `app/` - 应用协议层
- `user/` - 主循环和初始化

## 关键规则

1. **SDK调用边界**：`gd32f4xx_*.h` 函数只能在 `interface/src/` 中调用
2. **GPIO抽象**：所有引脚操作必须通过 `io_ctrl` 对象
3. **ISR限制**：中断中禁止动态内存分配、printf、复杂操作
4. **命名**：文件名全小写+下划线（如 `inter_usart.cpp`）
5. **换行符**：源码文件强制LF（见 `.gitattributes`）

## 协议切换

在 `app/inc/protocol_conf.hpp` 中修改 `ACTIVE_PROTOCOL` 宏：
- `PROTOCOL_MODBUS` (1) - Modbus RTU
- `PROTOCOL_STRING_CMD` (2) - ASCII字符串指令（默认）

## 开发流程

修改代码前必须：
1. 读取相关文件和接口
2. 分析影响范围
3. 形成修改方案
4. 等待用户授权

修改后必须更新 `docs/changelog.md`。

## 环境要求

- **工具链**：arm-none-eabi-gcc 14.x+
- **CMake**：3.20+
- **OpenOCD**：0.12+（烧录/调试）
- **Python**：3.x + pyserial（测试）

## 串口配置

- **调试串口**：USART0 (PA9-TX, PA10-RX)
- **波特率**：115200-8N1
- **RS485**：USART1 (PA2-TX, PA3-RX, PG13-DE)

## 参考文档

- 引脚表：`docs/pinout.md`
- 开发规范：`.agents/skills/my-gd-skill/SKILL.md`
- 变更日志：`docs/changelog.md`
