# GD32F470 嵌入式开发模板

基于 **GD32F470 / GD32F450** 的工业级固件模板项目，CMake + arm-none-eabi-gcc 构建，
支持 **Modbus RTU**、**ASCII 字符串指令** 和 **HEX 指令** 三协议栈。

> 开发板：MickeyPlus V2.0.0 (GD32F450ZGT6 / GD32F470ZIT6, 144-pin LQFP)
> 引脚表：[docs/pinout.md](docs/pinout.md) | 开发规范：[.agents/skills/my-gd-skill/SKILL.md](.agents/skills/my-gd-skill/SKILL.md)
> 变更日志：[docs/changelog.md](docs/changelog.md)

---

## 快速开始

```bash
# 1. 配置
cmake --preset debug
# 2. 编译（Linux）
cmake --build build/debug -j$(nproc)
# 2. 编译（macOS）
cmake --build build/debug -j16
# 3. 烧录（DAP-Link）
openocd -f openocd/interface-daplink.cfg -f openocd/target-gd32f4xx.cfg \
        -c "program build/debug/gd32f470_template.elf verify reset exit"
```

编译产物（位于 `build/debug/` 目录下）：

| 文件 | 用途 |
|------|------|
| `gd32f470_template.elf` | 可执行文件（含调试信息） |
| `gd32f470_template.hex` | Intel HEX 格式 |
| `gd32f470_template.bin` | 裸二进制 |
| `gd32f470_template.map` | 内存映射表 |
| `compile_commands.json` | clangd 代码补全数据库 |

> 当前资源占用：**FLASH ~22KB / 2MB (1.1%)，RAM ~6KB / 704KB (0.8%)**
> （实际随协议选择和外设启用有所变化）

---

## 目录

1. [硬件平台](#一硬件平台)
2. [环境搭建（三系统）](#二环境搭建三系统)
3. [构建与烧录](#三构建与烧录)
4. [调试](#四调试)
5. [C++ Interface 封装层](#五c-interface-封装层)
6. [项目结构](#六项目结构)
7. [系统架构](#七系统架构)
8. [协议配置与切换](#八协议配置与切换)
9. [测试](#九测试)
10. [Zed 编辑器集成](#十zed-编辑器集成)
11. [常见问题](#十一常见问题)
12. [Git 约定](#十二git-约定)

---

## 一、硬件平台

| 项目 | 规格 |
|------|------|
| 主控 | GD32F470ZIT6 (ARM Cortex-M4, 240MHz, FPv4-SP-D16) |
| Flash | 2MB 内部 + 16MB 外部 W25Q128 |
| SRAM | 704KB (512KB + 192KB) |
| 调试器 | DAP-Link / J-Link (SWD) |
| 传感器 | INA228 (电压/电流/功率/温度), INA226, BMP280 |
| 外设 | RS485, RS232, CAN 2.0B, EEPROM (AT24C02), W25Q128 SPI Flash, 拨码开关, LED, 按键 |
| 调试串口 | USART0 (115200-8N1) |
| 开发板 | MickeyPlus V2.0.0 |

---

## 二、环境搭建（三系统）

### 2.1 macOS（推荐 Homebrew）

```bash
# ── 1. 安装 ARM 交叉编译工具链 ──
brew install arm-none-eabi-gcc

# ── 2. 安装 CMake 构建系统 ──
brew install cmake

# ── 3. 安装 OpenOCD 调试/烧录工具 ──
brew install openocd

# ── 4. 安装 GDB 调试器（可选） ──
brew install gdb-multiarch

# ── 5. 安装 Python 测试依赖 ──
pip3 install pyserial
```

> **Apple Silicon (M1/M2/M3) 注意**：
> - `arm-none-eabi-gcc` 和 `openocd` 均原生支持 ARM64，无需 Rosetta
> - 工具默认安装在 `/opt/homebrew/bin/`，确保该目录在 `PATH` 中：
>   ```bash
>   echo 'export PATH="/opt/homebrew/bin:$PATH"' >> ~/.zshrc
>   source ~/.zshrc
>   ```

### 2.2 Linux（Debian / Ubuntu）

```bash
# ── 1. 安装 ARM 交叉编译工具链 ──
sudo apt update
sudo apt install gcc-arm-none-eabi binutils-arm-none-eabi libnewlib-arm-none-eabi

# ── 2. 安装 CMake 构建系统 ──
sudo apt install cmake make

# ── 3. 安装 OpenOCD 调试/烧录工具 ──
sudo apt install openocd

# ── 4. 安装 GDB 调试器（可选） ──
sudo apt install gdb-multiarch

# ── 5. 安装 Python 测试依赖 ──
sudo apt install python3-pip
pip3 install pyserial --user
```

> **Arch Linux / Manjaro**：
> ```bash
> sudo pacman -S arm-none-eabi-gcc arm-none-eabi-binutils arm-none-eabi-newlib cmake openocd
> pip3 install pyserial --user
> ```

> **openSUSE**：
> ```bash
> sudo zypper install arm-none-eabi-gcc cmake openocd cross-arm-none-eabi-gcc
> pip3 install pyserial --user
> ```

### 2.3 Windows（三种方式可选）

#### 方式 A：MSYS2（推荐）

```bash
# 1. 下载安装 MSYS2: https://www.msys2.org/

# 2. 打开 MSYS2 UCRT64 终端，更新包管理器
pacman -Syu

# 3. 安装 ARM 交叉编译工具链
pacman -S mingw-w64-ucrt-x86_64-arm-none-eabi-gcc

# 4. 安装 CMake 和 Make
pacman -S mingw-w64-ucrt-x86_64-cmake make

# 5. 安装 OpenOCD
pacman -S mingw-w64-ucrt-x86_64-openocd

# 6. 安装 Python
pacman -S mingw-w64-ucrt-x86_64-python python-pyserial

# 7.（可选）GDB
pacman -S mingw-w64-ucrt-x86_64-gdb
```

> 在 MSYS2 终端中进入项目目录后，使用 `cmake --preset debug` 配置，`make -j$(nproc)` 编译。

#### 方式 B：xPack 工具链（手动安装，独立于 MSYS2）

```bash
# 1. 安装 xPack ARM GCC
#    从 https://github.com/xpack-dev-tools/arm-none-eabi-gcc-xpack/releases
#    下载最新 Windows x64 .zip，解压到 C:\toolchains\xpack-arm-none-eabi-gcc

# 2. 安装 CMake
#    从 https://cmake.org/download/ 下载 Windows x64 Installer (.msi)

# 3. 安装 OpenOCD
#    从 https://github.com/xpack-dev-tools/openocd-xpack/releases
#    下载最新 Windows x64 .zip，解压到 C:\toolchains\xpack-openocd

# 4. 安装 Python
#    从 https://www.python.org/downloads/ 下载安装
pip install pyserial

# 5. 添加环境变量（系统 PATH）
#    C:\toolchains\xpack-arm-none-eabi-gcc\bin
#    C:\toolchains\xpack-openocd\bin
#    C:\Program Files\CMake\bin
```

> 在 **PowerShell** 或 **CMD** 中使用：
> ```powershell
> # 配置
> cd gd32_template_m4_cmake_v2
> cmake --preset debug -G "MinGW Makefiles"
> # 编译
> cmake --build build/debug -j8
> ```
> 注意：Windows 上需要 `-G "MinGW Makefiles"` 指定生成器，或者使用 `-G "Ninja"`（需安装 Ninja）。

#### 方式 C：Windows Subsystem for Linux (WSL2)

```bash
# 1. 安装 WSL2: https://learn.microsoft.com/zh-cn/windows/wsl/install
#    推荐 Ubuntu 24.04 LTS

# 2. 在 WSL 终端中执行 Linux (Ubuntu) 的安装命令
```

> 将项目放在 WSL 文件系统内（如 `~/projects/`）可获得最佳性能。
> 通过 `ls /dev/ttyACM*` 等访问 Windows 串口（WSL2 原生支持 USB 串口穿透）。

### 2.4 验证安装

在终端中运行以下命令，确认各工具安装成功且版本满足最低要求：

```bash
arm-none-eabi-gcc --version   # 期望 14.x 或更高
cmake --version                # 期望 3.20+
openocd --version              # 期望 0.12+
```

---

## 三、构建与烧录

### 3.1 配置（首次或修改 CMakeLists.txt 后）

项目使用 CMake Presets 管理配置，预设了 Debug / Release 两种构建类型。

```bash
# Debug 配置（带调试信息 -Og -g3）
cmake --preset debug

# Release 配置（优化 -O2）
cmake --preset release
```

> CMake 配置时会生成 `compile_commands.json` 到对应的 `build/` 目录下，供 clangd 代码补全使用。

### 3.2 编译

```bash
# Debug 编译
cmake --build --preset debug

# Release 编译
cmake --build --preset release

# 自定义线程数
cmake --build build/debug -j$(nproc)   # Linux
cmake --build build/debug -j16          # macOS
cmake --build build/debug -j8           # Windows
```

### 3.3 烧录

项目内置 CMake 烧录任务，支持 DAP-Link 和 J-Link 两种调试器。

**DAP-Link（CMSIS-DAP）烧录：**

```bash
# 方式一：CMake 任务（推荐）
cmake --build build/debug --target flash-daplink

# 方式二：直接 OpenOCD
openocd -f openocd/interface-daplink.cfg \
        -f openocd/target-gd32f4xx.cfg   \
        -c "program build/debug/gd32f470_template.elf verify reset exit"
```

**J-Link 烧录：**

```bash
# 方式一：CMake 任务
cmake --build build/debug --target flash-jlink

# 方式二：直接 OpenOCD
openocd -f openocd/interface-jlink.cfg \
        -f openocd/target-gd32f4xx.cfg \
        -c "program build/debug/gd32f470_template.elf verify reset exit"
```

**烧录参数说明：**

| 参数 | 说明 |
|------|------|
| `program <file>` | 烧录指定的 ELF 文件 |
| `verify` | 烧录后校验数据完整性 |
| `reset exit` | 烧录完成后复位芯片并退出 OpenOCD |

### 3.4 清理

```bash
# 清理 build 产物（保留 build 目录）
cmake --build build/debug --target clean

# 完全重建（推荐在重大变更后）
rm -rf build && cmake --preset debug && cmake --build build/debug
```

---

## 四、调试

### 4.1 启动 OpenOCD 调试服务器（终端 1）

```bash
# DAP-Link
cmake --build build/debug --target debug

# J-Link
cmake --build build/debug --target debug-jlink
```

等待看到 `Info : Listening on port 3333 for gdb connections` 即可。

### 4.2 连接 GDB（终端 2）

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

### 4.3 常用 GDB 命令

| 命令 | 说明 |
|------|------|
| `break function_name` | 在函数入口设断点 |
| `break file.c:123` | 在文件指定行设断点 |
| `continue` / `c` | 继续运行 |
| `next` / `n` | 单步执行（不进入函数） |
| `step` / `s` | 单步执行（进入函数） |
| `print var` / `p var` | 打印变量值 |
| `info registers` | 查看所有寄存器 |
| `bt` | 查看调用栈（backtrace） |
| `monitor reset` | 复位芯片 |

---

## 五、C++ Interface 封装层

项目基于 **GD32 标准外设库**构建了一套 C++ 硬件抽象层，共 **15 个封装类**，覆盖全部常用外设。

| 类 | 头文件 | 功能 | 状态 |
|----|--------|------|------|
| `io_ctrl` | `interface/inc/inter_io_ctrl.hpp` | GPIO 单引脚原子操作（RAII 生命周期） | ✅ 正式 |
| `usart_port` | `interface/inc/inter_usart.hpp` | USART 串口（阻塞/中断收发，含 ISR 内聚） | ✅ 正式 |
| `spi_port` | `interface/inc/inter_spi.hpp` | SPI 主机全双工（阻塞） | ✅ 正式 |
| `timer_port` | `interface/inc/inter_timer.hpp` | 基本定时 + PWM + 中断 | ✅ 正式 |
| `inter_i2c_bus` | `interface/inc/inter_i2c_bus.hpp` | 软件 I2C 总线（GPIO 位反转） | ✅ 正式 |
| `inter_i2c_dev` | `interface/inc/inter_i2c_dev.hpp` | I2C 设备基类 | ✅ 正式 |
| `i2c_hw_bus` | `interface/inc/inter_i2c_hw.hpp` | 硬件 I2C 主机（阻塞读写） | ✅ 正式 |
| `can_port` | `interface/inc/inter_can.hpp` | CAN 总线收发（阻塞） | ✅ 正式 |
| `adc_port` | `interface/inc/inter_adc.hpp` | ADC 单通道 + 扫描模式 | ✅ 正式 |
| `dac_port` | `interface/inc/inter_dac.hpp` | DAC 双通道电压输出 | ✅ 正式 |
| `dma_channel` | `interface/inc/inter_dma.hpp` | DMA 通道（M2M / P2M / M2P） | ✅ 正式 |
| `exti_port` | `interface/inc/inter_exti.hpp` | 外部中断（EXTI 线管理） | ✅ 正式 |
| `wdt_port` | `interface/inc/inter_wdt.hpp` | 看门狗（IWDG + WWDG） | ✅ 正式 |
| `rtc_port` | `interface/inc/inter_rtc.hpp` | RTC 日历 + 闹钟 | ✅ 正式 |
| `flash_port` | `interface/inc/inter_flash.hpp` | 内部 Flash 读/写/擦除 | ✅ 正式 |

> **使用原则**：所有 GD32 SDK 调用仅限 `interface/src/`，上层（device / app / user）**严禁**直接调用 `gd32f4xx_*.h` 函数。

### 串口驱动体系

项目使用 `usart_port` 作为串口驱动核心：

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

---

## 六、项目结构

```
gd32_template_m4_cmake_v2/
├── CMakeLists.txt                  # CMake 构建配置（所有编译逻辑在此）
├── CMakePresets.json               # CMake Presets（Debug/Release）
├── .gitattributes                  # Git 换行符与二进制文件配置
├── .gitignore                      # 忽略规则（build/、__pycache__/ 等）
├── .clangd                         # clangd 配置（编译数据库 + 系统头文件路径）

├── cmake/
│   └── arm-none-eabi-gcc.cmake     # 交叉编译工具链文件

├── openocd/                        # OpenOCD 调试器配置
│   ├── interface-daplink.cfg       # DAP-Link 接口配置
│   ├── interface-jlink.cfg         # J-Link 接口配置
│   └── target-gd32f4xx.cfg         # GD32F4xx 目标芯片配置

├── Firmware/                       # GD32 官方固件库（只读，不修改）
│   ├── CMSIS/                      # ARM CMSIS + 启动文件 + 链接脚本
│   │   └── GD/GD32F4xx/Source/GCC/
│   │       ├── startup_gd32f450_470.S   # 启动文件
│   │       ├── Ld/gd32f470xI_flash.ld   # 链接脚本
│   │       └── newlib/syscalls.c        # Newlib 系统调用桩
│   └── library/                    # GD32F4xx 标准外设库
│       ├── Source/                 # 外设驱动源文件
│       └── Include/                # 外设驱动头文件

├── code/                          # 核心系统代码（C）
│   ├── src/
│   │   ├── main.c                 # 主函数入口
│   │   ├── gd32f4xx_it.c          # 系统中断服务（非 USART 中断）
│   │   ├── systick.c              # SysTick 定时器（ms 基准）
│   │   └── link.c                 # 全局对象链接
│   └── inc/
│       ├── main.h
│       ├── gd32f4xx_it.h
│       ├── gd32f4xx_libopt.h      # 外设库模块选择
│       ├── systick.h
│       └── link.h                 # 全局对象声明

├── interface/                     # 接口抽象层（C++，核心驱动）
│   ├── src/                       # 15 个外设驱动实现
│   │   ├── inter_io_ctrl.cpp            # GPIO 引脚管理
│   │   ├── inter_usart.cpp         # 串口驱动（含 ISR 分发）
│   │   ├── inter_spi.cpp           # SPI 主机驱动
│   │   ├── inter_timer.cpp         # 定时器驱动
│   │   ├── inter_adc.cpp           # ADC 驱动
│   │   ├── inter_dac.cpp           # DAC 驱动
│   │   ├── inter_can.cpp           # CAN 驱动
│   │   ├── inter_i2c_hw.cpp        # 硬件 I2C 驱动
│   │   ├── inter_device_i2c_bus.cpp     # 软件 I2C 总线
│   │   ├── inter_device_i2c_dev.cpp     # I2C 设备基类
│   │   ├── inter_dma.cpp           # DMA 通道驱动
│   │   ├── inter_exti.cpp          # 外部中断驱动
│   │   ├── inter_wdt.cpp           # 看门狗驱动
│   │   ├── inter_rtc.cpp           # RTC 驱动
│   │   └── inter_flash.cpp         # 内部 Flash 驱动
│   └── inc/                       # 对应 .hpp 头文件

├── device/                          # 板级支持包（C++，设备驱动）
│   ├── src/
│   │   ├── device_debug.cpp         # 调试串口 (USART0)
│   │   ├── device_rs485.cpp         # RS485 串口 (USART1, DE=PG13)
│   │   ├── device_rs232.cpp         # RS232 串口 (USART2)
│   │   ├── device_ina228.cpp             # INA228 高精度传感器
│   │   ├── device_ina226.cpp             # INA226 传感器
│   │   └── device_w25q128.cpp            # W25Q128 SPI Flash 驱动
│   │   └── device_w25q128_flash.cpp      # W25Q128 通用 Flash 操作
│   └── inc/                       # 对应 .hpp 头文件

├── hardware/                     # 硬件驱动层（旧 C 驱动，逐步迁移中）
│   ├── src/
│   │   ├── hard_bsp_led.c             # LED 控制
│   │   ├── key.c                 # 按键检测
│   │   ├── dip_switch.c          # 拨码开关
│   │   ├── hard_eeprom.c              # AT24C02 EEPROM
│   │   ├── bmp280.c              # BMP280 气压传感器
│   │   ├── soft_i2c.c            # 旧版软件 I2C
│   │   ├── rs485.c               # 旧版 RS485
│   │   ├── can.c                 # 旧版 CAN
│   │   ├── hard_debug_uart.c          # 旧版调试串口（已停用）
│   │   ├── usart_text.c          # 旧版串口文本协议
│   │   ├── w25q128.c             # 旧版 SPI Flash
│   │   ├── hard_adc.c             # 旧版 ADC
│   │   ├── hard_adc_test.c            # ADC 测试
│   │   └── bitband.c             # Bit-band 操作
│   └── inc/                      # 对应 .h 头文件

├── protocol/                     # 协议栈层
│   ├── modbus/                   # Modbus RTU 协议栈
│   │   ├── inc/
│   │   │   ├── modbus.hpp        # Modbus 核心定义
│   │   │   ├── modbus_crc.hpp    # CRC16 校验
│   │   │   └── modbus_map.hpp    # 寄存器映射表
│   │   └── src/
│   │       ├── modbus.cpp        # 协议核心（帧解析/功能码）
│   │       ├── modbus_crc.cpp    # CRC 实现
│   │       └── modbus_map.cpp    # 寄存器映射实现
│   ├── string/                   # ASCII 字符串指令协议
│   │   ├── inc/
│   │   │   ├── string_pro.hpp    # 字符串协议桥接层
│   │   │   └── str_cmd.hpp       # 命令框架
│   │   └── src/
│   │       ├── string_pro.cpp    # 桥接层实现
│   │       └── str_cmd.cpp       # 命令表 + handler
│   └── hex/                      # HEX 指令协议
│       ├── inc/hex.hpp
│       └── src/hex.cpp

├── app/                          # 应用协议层
│   ├── inc/
│   │   ├── protocol_conf.hpp     # 协议 ID 定义 + 公共接口
│   │   ├── pc_task.hpp           # PC 通信任务
│   │   └── private.hpp           # 波特率存储管理
│   └── src/
│       ├── protocol_conf.cpp     # 多协议调度
│       ├── pc_task.cpp           # 任务入口
│       └── private.cpp           # 参数存储

├── user/                         # 用户应用层
│   ├── inc/founction.hpp
│   └── src/founction.cpp         # 主循环 + 硬件初始化

├── tests/                        # 协议测试脚本
│   ├── test_modbus.py            # Modbus RTU 全功能测试
│   └── test_string.py            # 字符串指令协议测试

├── docs/                         # 文档
│   ├── changelog.md              # 变更日志
│   ├── pinout.md                 # 引脚表
│   ├── debug_log.md              # 调试日志
│   ├── project_plan.md           # 项目规划
│   ├── readme.md                 # docs 索引
│   └── tests/
│       ├── test_report.md        # Modbus 测试报告
│       └── test_string_report.md # 字符串协议测试报告

├── file/                         # 数据手册 / 参考资料
│   ├── GD32F470ZIT6_规格书.PDF
│   ├── GD32F4xx_User_Manual_cn_Rev3.2.pdf
│   ├── INA228_规格书.pdf
│   ├── INA226AIDGSR_规格书_WJ778166.PDF
│   ├── AT24C02.pdf
│   ├── BMP280_压力传感器英文规格书.PDF
│   ├── FLASH_W25Q128JVSIQ_规格书_WJ1546901.PDF
│   ├── MickeyPlusV2.0.0-sch.pdf
│   └── ...

├── .zed/                         # Zed 编辑器配置
│   ├── tasks.json                # 编译/烧录/测试任务
│   ├── settings.json             # 编辑器设置
│   └── debug.json                # 调试配置

├── .agents/                      # Agent 技能指令
│   └── skills/my-gd-skill/
│       └── SKILL.md              # 项目开发规范

└── build/                        # 编译输出目录（git 忽略）
    ├── debug/                    # Debug 构建产物
    └── release/                  # Release 构建产物
```

---

## 七、系统架构

```
┌─────────────────────────────────────┐
│         user/founction              │  ← 主循环 / 硬件初始化
└──────────────┬──────────────────────┘
               │
┌──────────────▼──────────────────────┐
│        app/protocol_conf            │  ← 多协议调度层
│   ┌──────────┐  ┌──────────────┐   │
│   │  Modbus  │  │ StringCmd    │   │
│   └─────┬────┘  └──────┬───────┘   │
└─────────┼──────────────┼───────────┘
          │              │
┌─────────▼──────────────▼───────────┐
│      protocol/modbus, string, hex   │  ← 协议栈实现
└─────────┬──────────────────────────┘
          │
┌─────────▼──────────────────────────┐
│     interface / device / hardware     │  ← 硬件抽象 & 设备驱动
│  usart_port, i2c_bus, ina228, ...  │
│  LED, EEPROM, KEY, CAN, ...        │
└─────────┬──────────────────────────┘
          │
┌─────────▼──────────────────────────┐
│      Firmware (CMSIS + Library)    │  ← GD32 标准外设库
└────────────────────────────────────┘
```

### 数据流（Modbus 示例）

```
USART ISR → uart_buffer_t.rx_flag = 1
  → Protocol_Task() → Modbus_Task()
    → Modbus_FillData(buf, send_fn)     # 从串口 buffer 拷贝到 Modbus 内部 buffer
    → Modbus_FrameProcess()              # CRC校验 → 地址匹配 → 功能码分发
      → Function03/04/06/10()              # 读/写寄存器
      → Modbus_PortSend(tx_buf_arr)        # 通过串口发送响应
```

### 数据流（字符串指令示例）

```
USART ISR → uart_buffer_t.rx_flag = 1
  → Protocol_Task() → String_Task()
    → 按 \r\n 分割命令
    → 查命令表 → 执行 handler
    → send_data() 返回结果
```

---

## 八、协议配置与切换

项目支持三种通信协议，通过 `app/inc/protocol_conf.hpp` 中的宏切换：

```c
// ── 协议选择（修改此宏后重新编译即可切换） ──
#define PROTOCOL_NONE         0   // 无协议（仅内部处理）
#define PROTOCOL_MODBUS       1   // Modbus RTU
#define PROTOCOL_STRING_CMD   2   // ASCII 字符串指令（默认）

#define ACTIVE_PROTOCOL  PROTOCOL_STRING_CMD
```

### 协议功能对比

| 特性 | Modbus RTU | ASCII 字符串指令 | HEX 指令 |
|------|------------|------------------|----------|
| 功能码/命令 | 0x03/0x04/0x06/0x10 | 文本命令 (ina228_xxx) | HEX 帧 |
| 传感器读取 | ✅ 寄存器映射 | ✅ 命令查询 | ✅ |
| 写入控制 | ✅ 写寄存器 | ❌ (只读) | ✅ |
| 全量查询 | ❌ 分批读取 | ✅ `ina228_all` | ❌ |
| 帧格式 | 二进制 + CRC16 | 文本 + `\r\n` | 二进制 |
| 波特率 | 115200-8N1 | 115200-8N1 | 115200-8N1 |

---

## 九、测试

### 9.1 安装测试依赖

```bash
pip3 install pyserial
```

### 9.2 查看串口设备

```bash
# macOS
ls /dev/tty.usbmodem* /dev/ttyACM*

# Linux
ls /dev/ttyACM* /dev/ttyUSB*

# Windows（在 PowerShell 中）
[System.IO.Ports.SerialPort]::getportnames()
# 或使用设备管理器查看 COM 端口
```

### 9.3 运行测试

```bash
# Modbus 协议全功能测试
python3 tests/test_modbus.py /dev/ttyACM0

# 字符串指令协议测试
python3 tests/test_string.py /dev/ttyACM0

# Windows 上串口为 COM 端口
python3 tests/test_modbus.py COM3
python3 tests/test_string.py COM3
```

> 将串口号替换为实际设备。上电后等待 1~2 秒（INA228 初始化）再运行测试。

---

## 十、Zed 编辑器集成

项目已配置 `.zed/tasks.json`，在 Zed 中按 `Ctrl+Shift+B`（或 `Cmd+Shift+B`）可访问：

| 任务 | 说明 |
|------|------|
| **Build** | 编译 Debug 版本 |
| **Flash DAPLink** | 通过 DAP-Link 烧录 |
| **Build + Flash DAPLink** | 编译并烧录（一步完成） |
| **Flash JLink** | 通过 J-Link 烧录 |
| **Build + Flash JLink** | 编译后通过 J-Link 烧录 |
| **Clean Build** | 清理并重新配置 |
| **OpenOCD Server** | 启动 GDB 调试服务器 |
| **GDB** | 连接 GDB 调试器 |

---

## 十一、常见问题

### Q: `arm-none-eabi-gcc: command not found`

**macOS**：
```bash
export PATH="/opt/homebrew/bin:$PATH"
```
或确认已安装：`brew install arm-none-eabi-gcc`

**Linux (Ubuntu/Debian)**：
```bash
sudo apt install gcc-arm-none-eabi binutils-arm-none-eabi libnewlib-arm-none-eabi
```

**Windows (MSYS2)**：
```bash
pacman -S mingw-w64-ucrt-x86_64-arm-none-eabi-gcc
```
或将工具链 bin 目录添加到系统 PATH。

### Q: 烧录时报 `Error: unable to find a matching CMSIS-DAP device`

- 检查 USB 线是否接好
- DAP-Link 指示灯是否亮起
- 尝试更换 USB 口或数据线
- **Windows 注意**：确保已安装 DAP-Link 驱动（若使用 MSYS2，需通过 `lsusb` 确认设备可见）

### Q: 烧录成功但不运行

- 确保烧录时带 `reset exit` 参数
- 检查 `BOOT0` 引脚是否为低电平（从 Flash 启动）
- 拔掉 DAP-Link 后重新上电
- 检查电源供电是否正常

### Q: 串口无输出

- 确认串口调试器波特率设为 **115200**
- 检查 TX/RX 接线是否正确交叉（TX ↔ RX）
- 确认使用的串口号正确（参考 [九、测试](#九测试) 中的查看命令）
- 检查调试串口使用的 USART 端口（默认 USART0，PA9-TX, PA10-RX）

### Q: INA228 数据全为 0

- 上电后等待 1~2 秒（传感器初始化需要时间）
- 检查 I2C 引脚焊接和上拉电阻
- 检查 INA228 供电电压（3.3V）
- 确认 I2C 地址配置正确

### Q: `cmake --preset debug` 提示 Unknown preset

- 确认 CMake 版本 ≥ 3.20：`cmake --version`
- **Windows 注意**：CMake Presets 在 3.20+ 中可用，需确保使用新版本

### Q: Windows 上编译报错 `Cannot specify generators with multiple builds`

- 在 Windows 上使用 CMake Presets 时需指定生成器：
  ```powershell
  cmake --preset debug -G "MinGW Makefiles"
  # 或安装 Ninja 后：
  cmake --preset debug -G "Ninja"
  ```

---

## 十二、Git 约定

- **远程仓库**：
  - **Gitee**（主仓库）：`origin` / `gitee` → `https://gitee.com/addiction_28/gd32_-template_-m4_-cmake_v2.git`
  - **GitHub**（镜像）：`github` → `git@github.com:Tales-Unwritten/gd32_template_m4_cmake_v2.git`
- **当前开发分支**：`planB`
- 源码文件（`.c/.h/.cpp/.hpp/.s/.S`）强制 LF 换行（见 [.gitattributes](.gitattributes)）
- `build/`、`__pycache__/`、`compile_commands.json`、编译产物均被 [.gitignore](.gitignore) 忽略
- IDE 配置文件（`.zed/`）提交至仓库，团队共享一致性配置
- 提交前确保编译通过，遵循 `docs/changelog.md` 记录变更

---

## 许可

本项目为内部嵌入式开发模板，基于 GD32 标准外设库构建。
