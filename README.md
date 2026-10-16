# GD32F470 嵌入式开发模板

基于 **GD32F470 / GD32F450** 的工业级固件模板项目，CMake + arm-none-eabi-gcc 构建，
支持 **Modbus RTU**、**ASCII 字符串指令** 和 **HEX 指令** 三协议栈，
可选集成 **FreeRTOS** 调度内核。

> 开发板：MickeyPlus V2.0.0 (GD32F450ZGT6 / GD32F470ZIT6, 144-pin LQFP)
> 调试/烧录：**probe-rs**（SWD，替代已弃用的 OpenOCD）
> 引脚表、变更日志、开发规范等文档位于本地 `docs/` 与 `.agents/` 目录（**不纳入 Git 版本管理**）

---

## 快速开始

```bash
# 1. 配置
cmake --preset debug
# 2. 编译
cmake --build --preset debug
# 3. 烧录（probe-rs / DAP-Link，SWD）
probe-rs download --speed 4000 --protocol swd --chip STM32F429ZI \
    --verify build/debug/gd32f470_template.elf
probe-rs reset --speed 4000 --protocol swd --chip STM32F429ZI
```

> 在 Zed 中直接运行任务 **Build + Flash (Debug)** 可一步完成编译 + 烧录 + 复位。

编译产物（位于 `build/debug/` 目录下）：

| 文件 | 用途 |
|------|------|
| `gd32f470_template.elf` | 可执行文件（含调试信息，probe-rs 烧录对象） |
| `gd32f470_template.hex` | Intel HEX 格式 |
| `gd32f470_template.bin` | 裸二进制 |
| `gd32f470_template.map` | 内存映射表 |
| `compile_commands.json` | clangd 代码补全数据库 |

> 每次链接会打印内存占用（`-Wl,--print-memory-usage`）：`FLASH` / `RAM` 使用量随
> 协议选择、外设启用、`USE_FREERTOS` 开关而变化，以构建输出为准。

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
| 调试器 | DAP-Link / J-Link（SWD，经 probe-rs 驱动） |
| 传感器 | INA228 (电压/电流/功率/温度), INA226, BMP280 等 |
| 外设 | RS485, RS232, CAN 2.0B, EEPROM (AT24C02), W25Q128 SPI Flash, 拨码开关, LED, 按键 |
| 调试串口 | USART0 (115200-8N1) |
| 开发板 | MickeyPlus V2.0.0 |

---

## 二、环境搭建（三系统）

### 2.1 通用工具

| 工具 | 用途 | 最低版本 |
|------|------|----------|
| `arm-none-eabi-gcc` | ARM 交叉编译工具链 | 14.x |
| `cmake` / `make` | 构建系统 | CMake 3.20+ |
| `probe-rs` | 烧录 + 调试（SWD） | 最新稳定版 |
| `python3` + `pyserial` | 协议测试脚本 | 3.x |

**安装 probe-rs**（烧录/调试核心，替代 OpenOCD）：

```bash
# 方式一：官方安装脚本 / 预编译包（无需 Rust 环境）
#   见 https://github.com/probe-rs/probe-rs/releases  （probe-rs-tools）

# 方式二：通过 Cargo 安装（需 Rust 工具链）
cargo install probe-rs-tools --locked
```

### 2.2 macOS（推荐 Homebrew）

```bash
# ── 1. ARM 交叉编译工具链 ──
brew install arm-none-eabi-gcc

# ── 2. CMake 构建系统 ──
brew install cmake

# ── 3. probe-rs 烧录/调试工具 ──
cargo install probe-rs-tools --locked
#   或从 https://github.com/probe-rs/probe-rs/releases 下载预编译包

# ── 4. GDB 调试器（可选，配合 probe-rs gdb 使用） ──
brew install gdb-multiarch

# ── 5. Python 测试依赖 ──
pip3 install pyserial
```

> **Apple Silicon (M1/M2/M3) 注意**：
> - `arm-none-eabi-gcc` 原生支持 ARM64，无需 Rosetta
> - 工具默认安装在 `/opt/homebrew/bin/`，确保该目录在 `PATH` 中：
>   ```bash
>   echo 'export PATH="/opt/homebrew/bin:$PATH"' >> ~/.zshrc
>   source ~/.zshrc
>   ```

### 2.3 Linux（Debian / Ubuntu）

```bash
# ── 1. ARM 交叉编译工具链 ──
sudo apt update
sudo apt install gcc-arm-none-eabi binutils-arm-none-eabi libnewlib-arm-none-eabi

# ── 2. CMake 构建系统 ──
sudo apt install cmake make

# ── 3. probe-rs 烧录/调试工具 ──
cargo install probe-rs-tools --locked
#   或从 https://github.com/probe-rs/probe-rs/releases 下载预编译包

# ── 4. GDB 调试器（可选） ──
sudo apt install gdb-multiarch

# ── 5. Python 测试依赖 ──
sudo apt install python3-pip
pip3 install pyserial --user
```

> **Arch Linux / Manjaro**：
> ```bash
> sudo pacman -S arm-none-eabi-gcc arm-none-eabi-binutils arm-none-eabi-newlib cmake
> cargo install probe-rs-tools --locked
> pip3 install pyserial --user
> ```

> **openSUSE**：
> ```bash
> sudo zypper install arm-none-eabi-gcc cmake cross-arm-none-eabi-gcc
> cargo install probe-rs-tools --locked
> pip3 install pyserial --user
> ```

### 2.4 Windows（三种方式可选）

#### 方式 A：MSYS2（推荐）

```bash
# 1. 下载安装 MSYS2: https://www.msys2.org/

# 2. 打开 MSYS2 UCRT64 终端，更新包管理器
pacman -Syu

# 3. 安装 ARM 交叉编译工具链
pacman -S mingw-w64-ucrt-x86_64-arm-none-eabi-gcc

# 4. 安装 CMake 和 Make
pacman -S mingw-w64-ucrt-x86_64-cmake make

# 5. 安装 Rust（用于构建 probe-rs）或直接下载预编译包
pacman -S mingw-w64-ucrt-x86_64-rust
cargo install probe-rs-tools --locked

# 6. 安装 Python
pacman -S mingw-w64-ucrt-x86_64-python python-pyserial

# 7.（可选）GDB
pacman -S mingw-w64-ucrt-x86_64-gdb
```

> 在 MSYS2 终端中进入项目目录后，使用 `cmake --preset debug` 配置，`cmake --build --preset debug` 编译。

#### 方式 B：xPack 工具链（手动安装，独立于 MSYS2）

```bash
# 1. 安装 xPack ARM GCC
#    从 https://github.com/xpack-dev-tools/arm-none-eabi-gcc-xpack/releases
#    下载最新 Windows x64 .zip，解压到 C:\toolchains\xpack-arm-none-eabi-gcc

# 2. 安装 CMake
#    从 https://cmake.org/download/ 下载 Windows x64 Installer (.msi)

# 3. 安装 probe-rs
#    从 https://github.com/probe-rs/probe-rs/releases 下载 Windows 预编译包，
#    解压到 C:\toolchains\probe-rs

# 4. 安装 Python
#    从 https://www.python.org/downloads/ 下载安装
pip install pyserial

# 5. 添加环境变量（系统 PATH）
#    C:\toolchains\xpack-arm-none-eabi-gcc\bin
#    C:\toolchains\probe-rs
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
> USB 调试器（DAP-Link/J-Link）需通过 `usbipd-win` 转发到 WSL 才能被 probe-rs 识别。

### 2.5 验证安装

在终端中运行以下命令，确认各工具安装成功且版本满足最低要求：

```bash
arm-none-eabi-gcc --version   # 期望 14.x 或更高
cmake --version                # 期望 3.20+
probe-rs --version             # 能列出子命令即可
probe-rs list                  # 应能列出已连接的调试探针
```

---

## 三、构建与烧录

### 3.1 配置（首次或修改 CMakeLists.txt 后）

项目使用 CMake Presets 管理配置，预设了 **Debug / Release / IAP** 三种构建。

```bash
# Debug 配置（带调试信息 -Og -g3）
cmake --preset debug

# Release 配置（优化 -O2）
cmake --preset release

# IAP 配置（bootloader + 双 App 镜像，可选）
cmake --preset iap
```

> CMake 配置时会生成 `compile_commands.json` 符号链接到项目根目录，供 clangd 代码补全使用。
>
> **FreeRTOS 开关**：默认开启（`USE_FREERTOS=ON`）。切换回裸机：
> `cmake --preset debug -DUSE_FREERTOS=OFF`。

### 3.2 编译

```bash
# Debug 编译
cmake --build --preset debug

# Release 编译
cmake --build --preset release

# IAP 编译
cmake --build --preset iap

# 自定义线程数
cmake --build build/debug -j$(nproc)   # Linux
cmake --build build/debug -j16          # macOS
cmake --build build/debug -j8           # Windows
```

### 3.3 烧录（probe-rs / SWD）

> **芯片选型说明**：GD32F470 不在 probe-rs 内置芯片库中，但其 FMC 寄存器与 STM32F4 兼容，
> 实测使用 `--chip STM32F429ZI`（2MB Flash）可正常烧录/调试。
> `--protocol swd` 必须显式指定（probe-rs 默认 JTAG）。换芯片时全局替换 `STM32F429ZI` 即可。

```bash
# 烧录 + 校验 + 复位（等价于 Zed 任务 Flash (Debug)）
probe-rs download --speed 4000 --protocol swd --chip STM32F429ZI \
    --verify build/debug/gd32f470_template.elf
probe-rs reset --speed 4000 --protocol swd --chip STM32F429ZI

# 仅复位
probe-rs reset --speed 4000 --protocol swd --chip STM32F429ZI

# 擦除整片（慎用）
probe-rs erase --connect-under-reset --protocol swd --chip STM32F429ZI
```

**参数说明：**

| 参数 | 说明 |
|------|------|
| `download <file>` | 烧录指定 ELF 到目标 Flash |
| `--verify` | 烧录后校验数据完整性 |
| `--speed 4000` | SWD 时钟 4000 kHz |
| `--protocol swd` | 强制使用 SWD 协议 |
| `reset` | 复位并运行目标程序 |

> probe-rs `download` 烧录后**不会自动运行**，需另执行 `probe-rs reset`，
> 或使用 `probe-rs run`（烧录 + 运行 + 挂载 RTT 日志，见 [四、调试](#四调试)）。

### 3.4 清理

```bash
# 清理 build 产物（保留 build 目录）
cmake --build build/debug --target clean

# 完全重建（推荐在重大变更后）
rm -rf build && cmake --preset debug && cmake --build --preset debug
```

---

## 四、调试（probe-rs）

> 项目已**弃用 OpenOCD**，统一使用 probe-rs（SWD）。

### 4.1 一键烧录并运行

```bash
# 烧录 + 运行 + 实时输出（推荐日常调试）
probe-rs run --speed 4000 --protocol swd --chip STM32F429ZI \
    build/debug/gd32f470_template.elf
```

### 4.2 使用 Zed 调试面板

`.zed/debug.json` 预置了 GDB 调试配置（DAP-Link Debug），在 Zed 中打开调试面板即可
启动 / 断点 / 单步。

### 4.3 GDB 命令行调试（可选）

```bash
# 终端 1：启动 probe-rs GDB server（默认端口 1337）
probe-rs gdb --speed 4000 --protocol swd --chip STM32F429ZI

# 终端 2：连接 GDB
arm-none-eabi-gdb build/debug/gd32f470_template.elf
# 或 gdb-multiarch build/debug/gd32f470_template.elf

(gdb) target extended-remote localhost:1337   # 连接 probe-rs GDB server
(gdb) monitor reset halt                      # 复位并暂停
(gdb) load                                    # 下载程序到 Flash
(gdb) break main                              # 在 main 处设断点
(gdb) continue                                # 运行
```

### 4.4 常用 GDB 命令

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

项目基于 **GD32 标准外设库**构建了一套 C++ 硬件抽象层，覆盖全部常用外设。
所有 GD32 SDK 调用仅限 `interface/src/`，上层（device / app / user）严禁直接调用 `gd32f4xx_*.h`。

| 类 | 头文件 | 功能 |
|----|--------|------|
| `io_ctrl` | `interface/inc/inter_io_ctrl.hpp` | GPIO 单引脚原子操作（RAII 生命周期） |
| `usart_port_base` | `interface/inc/inter_usart.hpp` | 串口抽象基类（硬/软串口统一接口，可运行时多态互换） |
| `usart_port<RX>` | `interface/inc/inter_usart.hpp` | 硬件 USART（阻塞/中断收发，模板静态缓冲，ISR 内聚） |
| `soft_uart_port<RX>` | `interface/inc/inter_soft_uart.hpp` | 软件串口（GPIO 位反转 + DWT 时序） |
| `spi_bus` | `interface/inc/inter_spi_bus.hpp` | SPI 总线抽象基类 |
| `spi_port` | `interface/inc/inter_spi.hpp` | SPI 主机全双工（阻塞） |
| `soft_spi_bus` | `interface/inc/inter_soft_spi.hpp` | 软件 SPI（GPIO 位操作，内部/外部 CS） |
| `timer_port` | `interface/inc/inter_timer.hpp` | 基本定时 + PWM + 中断 |
| `i2c_bus` | `interface/inc/inter_i2c.hpp` | I2C 总线抽象基类 |
| `inter_i2c_bus` | `interface/inc/inter_i2c_bus.hpp` | 软件 I2C 总线（GPIO 位反转） |
| `inter_i2c_dev` | `interface/inc/inter_i2c_dev.hpp` | I2C 设备基类 |
| `i2c_hw_bus` | `interface/inc/inter_i2c_hw.hpp` | 硬件 I2C 主机（阻塞读写） |
| `can_port` | `interface/inc/inter_can.hpp` | CAN 总线收发（阻塞） |
| `adc_port` | `interface/inc/inter_adc.hpp` | ADC 单通道 + 扫描模式 |
| `dac_port` | `interface/inc/inter_dac.hpp` | DAC 双通道电压输出 |
| `dma_channel` | `interface/inc/inter_dma.hpp` | DMA 通道（M2M / P2M / M2P） |
| `exti_port` | `interface/inc/inter_exti.hpp` | 外部中断（EXTI 线管理 + 回调） |
| `wdt_port` | `interface/inc/inter_wdt.hpp` | 看门狗（IWDG + WWDG） |
| `rtc_port` | `interface/inc/inter_rtc.hpp` | RTC 日历 + 闹钟 |
| `flash_port` | `interface/inc/inter_flash.hpp` | 内部 Flash 读/写/擦除 |

> **使用原则**：所有 GD32 SDK 调用仅限 `interface/src/`，上层（device / app / user）**严禁**直接调用 `gd32f4xx_*.h` 函数。

### 串口驱动体系

项目使用模板化的 `usart_port<RX_BUF_SIZE>` 作为串口驱动核心（硬/软串口均继承 `usart_port_base`）：

```cpp
// 全局串口对象声明于 device/inc/device_serial.hpp
extern usart_port<256> debug_uart;   // USART0 (PA9/PA10)
extern usart_port<256> rs232_uart;   // USART2
extern usart_port<256> rs485_uart;   // USART1 (PA2/PA3, DE=PG13)
extern io_ctrl         en_485;       // RS485 方向控制

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

> 接收缓冲区由模板参数 `RX_BUF_SIZE` 在编译期静态分配（零堆分配），ISR 统一内聚在
> `interface/src/inter_usart.cpp`，`code/src/gd32f4xx_it.c` 中不含任何 USART ISR。

---

## 六、项目结构

```
gd32_template_m4_cmake_v2/
├── CMakeLists.txt                  # CMake 构建配置（所有编译逻辑在此）
├── CMakePresets.json               # CMake Presets（Debug / Release / IAP）
├── .gitattributes                  # Git 换行符与二进制文件配置
├── .gitignore                      # 忽略规则（build/、docs/、.agents/ 等）
├── .zed/                           # Zed 编辑器配置（团队共享）
│   ├── tasks.json                  # 编译/烧录/擦除/复位任务（probe-rs）
│   ├── settings.json               # 编辑器设置（clangd）
│   └── debug.json                  # GDB 调试配置（probe-rs，端口 1337）
│
├── cmake/
│   └── arm-none-eabi-gcc.cmake     # 交叉编译工具链文件
│
├── openocd/                        # [已弃用] 旧 OpenOCD 配置（保留备查）
│   ├── interface-daplink.cfg
│   ├── interface-jlink.cfg
│   └── target-gd32f4xx.cfg
│
├── Firmware/                       # GD32 官方固件库（只读，不修改）
│   ├── CMSIS/                      # ARM CMSIS + 启动文件 + 链接脚本
│   │   └── GD/GD32F4xx/Source/GCC/
│   │       ├── startup_gd32f450_470.S   # 启动文件
│   │       ├── Ld/gd32f470xI_flash.ld   # 链接脚本
│   │       └── newlib/syscalls.c        # Newlib 系统调用桩
│   └── library/                    # GD32F4xx 标准外设库（Source / Include）
│
├── freertos/                       # FreeRTOS V11.3.1 内核 + GCC/ARM_CM4F 移植层
│   ├── include/                     # 内核公共头文件
│   ├── portable/                    # 移植层（GCC + MemMang/heap_4.c）
│   └── *.c                          # tasks / queue / list / timers / ...
│
├── code/                           # 核心系统代码（C）
│   ├── src/
│   │   ├── main.c                  # 主函数入口
│   │   ├── gd32f4xx_it.c           # 系统中断服务（非 USART 中断）
│   │   ├── systick.c               # SysTick 定时器（ms 基准）
│   │   ├── link.c                  # 全局对象链接
│   │   └── freertos_api_index.c    # FreeRTOS API 跳转索引（零代码量）
│   └── inc/
│       ├── main.h
│       ├── gd32f4xx_it.h
│       ├── gd32f4xx_libopt.h       # 外设库模块选择
│       ├── systick.h
│       ├── link.h                  # 全局对象声明
│       ├── rtos_conf.h             # 软件侧 FreeRTOS 开关
│       └── FreeRTOSConfig.h
│
├── interface/                      # 接口抽象层（C++，核心驱动，唯一调用 SDK 处）
│   ├── src/
│   │   ├── inter_io_ctrl.cpp            # GPIO 引脚管理
│   │   ├── inter_usart.cpp              # 硬件串口驱动（含 ISR 分发 + 端口表）
│   │   ├── inter_soft_uart.cpp          # 软件串口（位反转 + DWT）
│   │   ├── inter_spi.cpp                # SPI 主机驱动
│   │   ├── inter_soft_spi.cpp           # 软件 SPI 总线
│   │   ├── inter_timer.cpp              # 定时器驱动
│   │   ├── inter_adc.cpp                # ADC 驱动
│   │   ├── inter_dac.cpp                # DAC 驱动
│   │   ├── inter_can.cpp                # CAN 驱动
│   │   ├── inter_i2c_hw.cpp             # 硬件 I2C 驱动
│   │   ├── inter_i2c_bus.cpp            # 软件 I2C 总线
│   │   ├── inter_i2c_dev.cpp            # I2C 设备基类
│   │   ├── inter_dma.cpp                # DMA 通道驱动
│   │   ├── inter_exti.cpp               # 外部中断驱动
│   │   ├── inter_wdt.cpp                # 看门狗驱动
│   │   ├── inter_rtc.cpp                # RTC 驱动
│   │   └── inter_flash.cpp              # 内部 Flash 驱动
│   └── inc/                        # 对应 .hpp 头文件
│
├── device/                         # 设备驱动层（C++，约 46 个器件驱动，自动 glob 收录）
│   ├── src/
│   │   ├── device_serial.cpp            # 全局串口对象定义（debug/rs232/rs485）
│   │   ├── device_ina228.cpp            # INA228 高精度传感器
│   │   ├── device_ina226.cpp            # INA226 传感器
│   │   ├── device_ina219/260/3221.cpp   # INA 系列其他型号
│   │   ├── device_w25q128.cpp           # W25Q128 SPI Flash 驱动
│   │   ├── device_w25q128_flash.cpp     # W25Q128 通用 Flash 操作
│   │   ├── device_ads1115 / ds18b20 / sht31 / bh1750 / mcp23017 / pca9685 ...
│   │   └── ...（压力量/温度/ADC/DAC/IO 扩展/EEPROM 等系列驱动）
│   └── inc/                        # 对应 .hpp 头文件
│
├── hardware/                       # 旧 C 驱动（逐步迁移中，仅少量仍参与编译）
│   ├── src/
│   │   ├── hard_bsp_led.c               # LED 控制（在用）
│   │   ├── hard_eeprom.c                # AT24C02 EEPROM（在用）
│   │   ├── hard_adc.c / hard_can.c / hard_rs485.c / hard_key.c ...
│   │   └── hard_debug_uart.c            # 旧调试串口（已停用，被 interface 取代）
│   └── inc/                        # 对应 .h 头文件
│
├── protocol/                       # 协议栈层
│   ├── modbus/                     # Modbus RTU 协议栈
│   │   ├── inc/  (modbus.hpp / modbus_crc.hpp / modbus_map.hpp)
│   │   └── src/  (modbus.cpp / modbus_crc.cpp / modbus_map.cpp)
│   ├── string/                     # ASCII 字符串指令协议
│   │   ├── inc/  (string_pro.hpp / str_cmd.hpp)
│   │   └── src/  (string_pro.cpp / str_cmd.cpp)
│   ├── hex/                        # HEX 指令协议
│   │   ├── inc/hex.hpp
│   │   └── src/hex.cpp
│   └── iap/                        # IAP 升级协议（可选，-DBUILD_IAP=ON）
│       ├── boot/ application/ common/ ld/ tools/
│       └── CMakeLists.txt
│
├── app/                            # 应用协议层
│   ├── inc/  (protocol_conf.hpp / pc_task.hpp / private.hpp)
│   └── src/  (protocol_conf.cpp / pc_task.cpp / private.cpp)
│
├── user/                           # 用户应用层
│   ├── inc/function.hpp
│   └── src/function.cpp            # 主循环 + 硬件初始化
│
├── tests/                          # 协议测试脚本（Python）
│   ├── test_modbus.py              # Modbus RTU 全功能测试
│   ├── test_string.py              # 字符串指令协议测试
│   └── iap_test.py                 # IAP 升级流程测试
│
├── tools/                          # 辅助脚本
│   ├── gen_api_index.py            # 生成 FreeRTOS API 跳转索引
│   └── gen_api_links.py
│
├── docs/                           # [本地，不纳入 Git] 文档
│   ├── CHANGELOG.md / PINOUT.md / DEBUG_LOG.md / PROJECT_PLAN.md / README.md
│   └── tests/
│
├── file/                           # [部分本地] 数据手册 / 参考资料（*.pdf、*.eprj2、*.enet* 忽略）
│
└── build/                          # 编译输出目录（git 忽略）
    ├── debug/                      # Debug 构建产物
    ├── release/                    # Release 构建产物
    └── iap/                        # IAP 构建产物
```

> `docs/`、`.agents/`、`file/*.eprj2`、`file/*.enet*`、`file/*.pdf` 均**不纳入 Git**，仅保留在本地工作区。

---

## 七、系统架构

```
┌─────────────────────────────────────┐
│   user/function  (+ FreeRTOS 任务)  │  ← 主循环 / 硬件初始化
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
│    protocol/modbus, string, hex     │  ← 协议栈实现
└─────────┬──────────────────────────┘
          │
┌─────────▼──────────────────────────┐
│    interface / device / hardware      │  ← 硬件抽象 & 设备驱动
│  usart_port, soft_uart, i2c_bus,   │
│  ina228, w25q128, LED, ...         │
└─────────┬──────────────────────────┘
          │
┌─────────▼──────────────────────────┐
│   Firmware (CMSIS + Library)        │  ← GD32 标准外设库
│   freertos (可选调度内核)             │
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
  → Protocol_Task() → StringCmd_Task()
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

# IAP 升级流程测试
python3 tests/iap_test.py /dev/ttyACM0

# Windows 上串口为 COM 端口
python3 tests/test_modbus.py COM3
python3 tests/test_string.py COM3
```

> 将串口号替换为实际设备。上电后等待 1~2 秒（INA228 初始化）再运行测试。

---

## 十、Zed 编辑器集成

项目已配置 `.zed/tasks.json`（**probe-rs** 烧录/调试链路），在 Zed 中按
`Ctrl+Shift+B`（或 `Cmd+Shift+B`）可访问：

| 任务 | 说明 |
|------|------|
| **Build (Debug)** | 配置 + 编译 Debug 版本 |
| **Build (Release)** | 配置 + 编译 Release 版本 |
| **Flash (Debug)** | 烧录 Debug（download + verify + reset） |
| **Build + Flash (Debug)** | 编译并烧录 Debug（一步完成） |
| **Build + Flash (Release)** | 编译后烧录 Release |
| **Clean Build** | 删除 build 目录 |
| **Erase Chip** | 擦除整片 Flash（慎用） |
| **Reset Target** | 仅复位目标芯片 |

> 所有任务使用 `--protocol swd --chip STM32F429ZI`（GD32F470 兼容替代）。
> `.zed/debug.json` 提供 GDB 调试配置。

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

### Q: `probe-rs: command not found`

- 通过 `cargo install probe-rs-tools --locked` 安装，或从
  [probe-rs releases](https://github.com/probe-rs/probe-rs/releases) 下载预编译包
- 确认 `probe-rs` 可执行文件在 `PATH` 中：`probe-rs --version`

### Q: probe-rs 找不到探针 / 无法连接（`No probe was found`）

- 检查 USB 线是否接好，DAP-Link 指示灯是否亮起
- 用 `probe-rs list` 确认探针已被识别
- **必须显式指定 SWD**：`--protocol swd`（probe-rs 默认 JTAG）
- **Linux 权限**：为探针添加 udev 规则，或临时使用 `sudo`
- **Windows**：确保已安装 DAP-Link / J-Link 驱动

### Q: 提示 `chip STM32F429ZI` 不匹配

- GD32F470 不在 probe-rs 芯片库中，**必须**使用兼容芯片 `STM32F429ZI`
  （GD32F4 的 FMC 寄存器与 STM32F4 兼容，实测烧录/调试正常），并显式指定 `--protocol swd`

### Q: 烧录成功但不运行

- probe-rs `download` 烧录后**不会自动运行**，需 `probe-rs reset` 或改用 `probe-rs run`
- 检查 `BOOT0` 引脚是否为低电平（从 Flash 启动）
- 重新上电后再试
- 检查电源供电是否正常

### Q: 串口无输出

- 确认串口调试器波特率设为 **115200**
- 检查 TX/RX 接线是否正确交叉（TX ↔ RX）
- 确认使用的串口号正确（参考 [九、测试](#九测试) 中的查看命令）
- 检查调试串口使用的 USART 端口（默认 USART0，PA9-TX, PA10-RX）
- 注意 `user/src/function.cpp` 当前可能处于**软串口测试模式**（硬件 `debug_uart` 与软串口共用 PA9/PA10），
  如需硬件串口请恢复 `DebugPort_Init()` 调用

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
  - **Gitee**（主仓库）：`origin` → `git@gitee.com:addiction_28/gd32_-template_-m4_-cmake_v2.git`
  - **GitHub**（镜像）：`github` → `git@github.com:Tales-Unwritten/gd32_template_m4_cmake_v2.git`
    （<https://github.com/Tales-Unwritten/gd32_template_m4_cmake_v2>）
- **同步推送**：提交后同时推送主仓库与镜像：
  ```bash
  git push origin planB && git push github planB
  ```
- **当前开发分支**：`planB`
- **FreeRTOS 维护说明**：本分支（`planB`）以裸机 + 协议栈重构为主线，FreeRTOS 部分
  仅保留可编译集成；相关内核裁剪、集成与练习内容已迁移到 **`practice`** 分支维护。
- 源码文件（`.c/.h/.cpp/.hpp/.s/.S/.ld`）强制 LF 换行（见 [.gitattributes](.gitattributes)）
- IDE 配置（`.zed/`）提交至仓库，团队共享一致性配置
- **忽略（不纳入 Git）**：
  - `build/`、`__pycache__/`、`compile_commands.json`、各编译产物
  - `.agents/`（Agent 技能）、`docs/`（本地文档）
  - `file/*.pdf`（大体积数据手册）、`file/*.eprj2`、`file/*.enet*`（EasyEDA 工程/网表文件）
- 提交前确保编译通过，并（在本地 `docs/CHANGELOG.md` 中）记录变更

---

## 许可

本项目为内部嵌入式开发模板，基于 GD32 标准外设库构建，集成 FreeRTOS 内核（MIT）。
