# GD32F470 嵌入式开发模板

基于 **GD32F470ZIT6** 的固件模板项目，使用 CMake + arm-none-eabi-gcc 构建系统，支持 **Modbus RTU** 和 **ASCII 字符串指令** 双协议栈。

---

## 一、硬件平台

| 项目 | 规格 |
|------|------|
| 主控 | GD32F470ZIT6 (ARM Cortex-M4, 240MHz) |
| 调试器 | DAP-Link / J-Link (SWD) |
| 传感器 | INA228 (电压/电流/功率/温度), INA226, BMP280 |
| 外设 | RS485, RS232, CAN, EEPROM (AT24C02), W25Q128 Flash, 拨码开关 |
| 调试串口 | USART0 (115200-8N1) |

---

## 二、环境搭建

### 2.1 macOS（推荐 Homebrew）

```bash
# ARM 交叉编译工具链
brew install arm-none-eabi-gcc

# 构建系统
brew install cmake

# 调试 / 烧录
brew install openocd
brew install gdb-multiarch    # GDB 调试（可选）

# 测试工具（Python）
pip3 install pyserial
```

### 2.2 Linux (Debian/Ubuntu)

```bash
sudo apt install gcc-arm-none-eabi binutils-arm-none-eabi libnewlib-arm-none-eabi
sudo apt install cmake make
sudo apt install openocd
sudo apt install gdb-multiarch
pip3 install pyserial
```

### 2.3 验证安装

```bash
arm-none-eabi-gcc --version   # 期望 14.x 或更高
cmake --version                # 期望 3.20+
openocd --version              # 期望 0.12+
```

---

## 三、项目结构

```
GD32_Template_M4_Cmake_v2/
├── CMakeLists.txt              # CMake 构建配置（所有编译逻辑在此）
├── CMakePresets.json           # CMake Presets（Debug/Release）
├── cmake/
│   └── arm-none-eabi-gcc.cmake # 交叉编译工具链文件
├── openocd/
│   ├── interface-daplink.cfg   # DAP-Link 接口配置
│   ├── interface-jlink.cfg     # J-Link 接口配置
│   └── target-gd32f4xx.cfg     # GD32F4xx 目标芯片配置
├── .zed/tasks.json             # Zed 编辑器任务（编译/烧录/测试）
├── code/                       # 核心应用代码
│   ├── src/  (main.c, gd32f4xx_it.c, systick.c, link.c)
│   └── inc/
├── Hardware/                   # 硬件驱动层 (C)
│   ├── src/  (led, eeprom, can, rs485, key, dip_switch, ...)
│   └── inc/
├── bsp/                        # 板级支持包 (C++)
│   ├── src/  (ina226.cpp, ina228.cpp, debug_port.cpp, rs485_port.cpp, rs232_port.cpp)
│   └── inc/
├── interface/                  # 接口抽象层 (C++)
│   ├── src/  (io_ctrl.cpp, usart_port.cpp, device_i2c_bus.cpp, device_i2c_dev.cpp)
│   └── inc/
├── middleware/                  # 中间件协议栈
│   └── modbus/
│       ├── inc/  (modbus.hpp, modbus_crc.hpp, modbus_map.hpp)
│       └── src/  (modbus.cpp, modbus_crc.cpp, modbus_map.cpp, modbus_port_cfg.cpp)
├── App/                        # 应用协议层
│   ├── inc/  (protocol.hpp, pc_task.hpp, private.hpp)
│   └── src/  (protocol.cpp, protocol_string.cpp, pc_task.cpp, private.cpp)
├── user/                       # 用户应用层
│   ├── src/founction.cpp       # 主循环 + 初始化
│   └── inc/
├── Firmware/                   # GD32 官方固件库
│   ├── CMSIS/                  # ARM CMSIS + 系统文件
│   └── library/                # GD32F4xx 标准外设库
├── tests/                      # 协议测试脚本
│   ├── test_modbus.py          # Modbus RTU 全功能测试
│   ├── test_string.py          # ASCII 字符串指令测试
│   ├── switch_protocol.sh      # 协议切换 + 编译 + 烧录
│   └── TEST_REPORT.md          # 测试报告
├── file/                       # 数据手册 / 参考资料
└── build/                      # 编译输出目录（git 忽略）
```

---

## 四、构建

### 4.1 配置（首次或修改 CMakeLists.txt 后）

```bash
# Debug 配置（带调试信息 -Og -g3）
cmake --preset debug

# Release 配置（优化 -O2）
cmake --preset release
```

> 配置文件 `CMakePresets.json` 中预设好了 Debug/Release 两种配置。
> CMake 配置时会生成 `compile_commands.json` 到 `build/debug/`（或 `build/release/`），供 clangd 代码补全使用。

### 4.2 编译

```bash
# Debug 编译
cmake --build --preset debug

# Release 编译
cmake --build --preset release

# 或直接编译 build 目录
cmake --build build/debug -j$(nproc)   # Linux
cmake --build build/debug -j16          # macOS 固定 16 线程
```

编译产物（在 `build/debug/` 或 `build/release/` 目录下）：

| 文件 | 用途 |
|------|------|
| `gd32f470_template.elf` | 可执行文件（含调试信息） |
| `gd32f470_template.hex` | Intel HEX 格式 |
| `gd32f470_template.bin` | 裸二进制 |
| `gd32f470_template.map` | 内存映射表 |
| `compile_commands.json` | clangd 代码补全数据库 |

### 4.3 清理

```bash
# 清理编译产物
cmake --build build/debug --target clean

# 完全重建
rm -rf build && cmake --preset debug && cmake --build build/debug
```

---

## 五、烧录

### 5.1 DAP-Link 烧录（CMSIS-DAP）

```bash
# 方式一：CMake 任务（推荐）
cmake --build build/debug --target flash-daplink

# 方式二：直接 OpenOCD
openocd -f openocd/interface-daplink.cfg \
        -f openocd/target-gd32f4xx.cfg   \
        -c "program build/debug/gd32f470_template.elf verify reset exit"
```

### 5.2 J-Link 烧录

```bash
# 方式一：CMake 任务
cmake --build build/debug --target flash-jlink

# 方式二：直接 OpenOCD
openocd -f openocd/interface-jlink.cfg \
        -f openocd/target-gd32f4xx.cfg \
        -c "program build/debug/gd32f470_template.elf verify reset exit"
```

### 5.3 烧录说明

| 参数 | 说明 |
|------|------|
| `program` | 烧录文件路径 |
| `verify` | 烧录后校验 |
| `reset exit` | 烧录完成后复位芯片并退出 OpenOCD |

---

## 六、调试

### 6.1 启动调试服务器（终端 1）

```bash
# DAP-Link
cmake --build build/debug --target debug

# J-Link
cmake --build build/debug --target debug-jlink
```

等待看到 `Info : Listening on port 3333 for gdb connections` 即可。

### 6.2 连接 GDB（终端 2）

```bash
# 方式一：CMake 任务（自动连接 + 加载 + 断点 main + 运行）
cmake --build build/debug --target gdb

# 方式二：手动 GDB
arm-none-eabi-gdb build/debug/gd32f470_template.elf
# 或 gdb-multiarch build/debug/gd32f470_template.elf

(gdb) target remote localhost:3333   # 连接 OpenOCD
(gdb) monitor reset halt             # 复位并暂停
(gdb) load                           # 下载程序到 Flash
(gdb) break main                     # 在 main 处设断点
(gdb) continue                       # 运行
```

### 6.3 常用 GDB 命令

| 命令 | 说明 |
|------|------|
| `break function_name` | 在函数入口设断点 |
| `break file.c:123` | 在文件指定行设断点 |
| `continue` / `c` | 继续运行 |
| `next` / `n` | 单步执行（不进入函数） |
| `step` / `s` | 单步执行（进入函数） |
| `print var` / `p var` | 打印变量值 |
| `info registers` | 查看所有寄存器 |
| `bt` | 查看调用栈 |
| `monitor reset` | 复位芯片 |

---

## 七、协议切换

项目支持两种通信协议，通过 `App/inc/protocol.hpp` 中的宏切换：

```c
// 选择 Modbus RTU 协议
#define ACTIVE_PROTOCOL  PROTOCOL_MODBUS

// 选择 ASCII 字符串指令协议（默认）
#define ACTIVE_PROTOCOL  PROTOCOL_STRING_CMD
```

### 7.1 快速切换 + 烧录（Linux/macOS）

```bash
# 切换到 Modbus 模式并烧录
./tests/switch_protocol.sh modbus

# 切换到字符串指令模式并烧录
./tests/switch_protocol.sh string
```

### 7.2 协议功能对比

| 特性 | Modbus RTU | ASCII 字符串指令 |
|------|------------|------------------|
| 功能码 | 0x03/0x04/0x06/0x10 | 文本命令 |
| 传感器读取 | ✅ 寄存器映射 | ✅ `ina228_xxx` 命令 |
| 写入控制 | ✅ 写寄存器 | ❌ (只读) |
| 全量查询 | ❌ 分批读取 | ✅ `ina228_all` |
| 帧格式 | 二进制 + CRC16 | 文本 + `\r\n` |
| 波特率 | 115200-8N1 | 115200-8N1 |

---

## 八、测试

### 8.1 安装测试依赖

```bash
pip3 install pyserial
```

### 8.2 查看串口

```bash
# macOS
ls /dev/tty.usbmodem* /dev/ttyACM*

# Linux
ls /dev/ttyACM* /dev/ttyUSB*
```

### 8.3 运行测试

```bash
# Modbus 协议全功能测试
python3 tests/test_modbus.py /dev/tty.usbmodem123456

# 字符串指令协议覆盖测试（11 项）
python3 tests/test_string.py /dev/tty.usbmodem123456
```

> 将串口号替换为实际设备。上电后等待 1~2 秒（INA228 初始化）再运行测试。

---

## 九、Zed 编辑器集成

项目已配置 `.zed/tasks.json`，在 Zed 中按 `Ctrl+Shift+B`（或 `Cmd+Shift+B`）可访问：

| 任务 | 快捷键提示 |
|------|-----------|
| Build Debug | 编译项目 |
| Flash DAPLink | 通过 DAP-Link 烧录 |
| Build + Flash | 编译并烧录 |
| Switch Modbus + Flash | 切到 Modbus 协议 → 编译 → 烧录 |
| Switch String + Flash | 切到字符串协议 → 编译 → 烧录 |
| Test Modbus | 运行 Modbus 测试（需调整串口号） |
| Test String | 运行字符串指令测试（需调整串口号） |

---

## 十、常见问题

### Q: `arm-none-eabi-gcc: command not found`

```bash
# macOS
export PATH="/opt/homebrew/bin:$PATH"

# Linux
sudo apt install gcc-arm-none-eabi
```

### Q: 烧录时报 `Error: unable to find a matching CMSIS-DAP device`

- 检查 USB 线是否接好
- DAP-Link 指示灯是否亮起
- 尝试更换 USB 口或数据线

### Q: 烧录成功但不运行

- 确保烧录时带 `reset exit` 参数
- 检查 `BOOT0` 引脚是否为低电平
- 拔掉 DAP-Link 后重新上电

### Q: 串口无输出

- 确认波特率设为 **115200**
- 检查 TX/RX 接线是否正确交叉
- 检查调试串口使用的 USART 端口（默认 USART0）

### Q: INA228 数据全为 0

- 上电后等待 1~2 秒（传感器初始化需要时间）
- 检查 I2C 引脚焊接和上拉电阻
- 检查 INA228 供电电压

---

## 十一、Git 约定

- 源码文件（`.c/.h/.cpp/.hpp/.s/.S`）强制 LF 换行（见 `.gitattributes`）
- `build/`、`.cache/`、`compile_commands.json`、编译产物均被 `.gitignore` 忽略
- IDE 配置文件（`.zed/`、`.idea/`）提交至仓库，团队共享一致性配置
- GitHub Actions / CI 暂未配置，可在此基础上添加

---

## 十二、系统架构

```
┌─────────────────────────────────────┐
│           user/founction            │  ← 主循环 / 初始化
└──────────────┬──────────────────────┘
               │
┌──────────────▼──────────────────────┐
│        App/protocol                 │  ← 协议调度
│   ┌─────────┐  ┌─────────────────┐  │
│   │ Modbus  │  │  StringCmd      │  │
│   └────┬────┘  └───────┬─────────┘  │
└────────┼───────────────┼────────────┘
         │               │
┌────────▼───────────────▼────────────┐
│      middleware/modbus              │  ← Modbus 协议栈
└────────┬────────────────────────────┘
         │
┌────────▼────────────────────────────┐
│     interface / bsp                 │  ← 接口 & 驱动
│  usart_port, i2c_bus, ina228, ...   │
└────────┬────────────────────────────┘
         │
┌────────▼────────────────────────────┐
│     Hardware / Firmware             │  ← 底层外设 / CMSIS
└─────────────────────────────────────┘
```
