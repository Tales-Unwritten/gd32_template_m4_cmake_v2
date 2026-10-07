# GD32 Modbus / 字符串指令 双协议测试报告

**测试日期**: 2026-05-27（原始协议测试）｜**最近更新**: 2026-10-06（补充固定测试接口表）  
**测试目标**: GD32F470 + INA228 传感器，验证 Modbus RTU 及 ASCII 字符串双协议数据流  
**测试环境**: Linux + OpenOCD (CMSIS-DAP) + Python (pyserial)

---

## 零、固定测试接口（硬件连线速查）

> 板载 DAP-Link 已接好，可直接烧录/调试。引脚来源：`docs/PINOUT.md` 网表 + 代码实例化处。
> **这些测试引脚固定不变**，换测试固件时按此表接线。

| 器件 / 功能 | 接口 | 引脚 | 参数 | 代码位置 |
|-------------|------|------|------|----------|
| **DAP-Link 调试串口** | USART0 | **PA9=TX, PA10=RX** | 115200-8N1 | `device/src/device_serial.cpp`（`debug_uart`） |
| **INA228**（电压/电流） | 软 I2C | **PE2=SCL, PE3=SDA** | addr `0x40`；**ALERT=PG14** | `user/src/function.cpp`（`i2c_bus` / `INA228_2` / `ina228_cfg`） |
| **AT24C02** EEPROM | I2C | **PB6=SCL, PB7=SDA** | 7-bit addr `0x50` | `docs/PINOUT.md`；驱动 `hardware/src/hard_eeprom.c`(`eeprom_config`) / `device/inc/device_eeprom.hpp` |
| **W25Q128** SPI Flash | SPI4 | **PF6=CS, PF7=CLK, PF8=MISO, PF9=MOSI** | AF5；硬 SPI MODE3/PSC2；软 SPI MODE0 | `user/src/function.cpp`（`w25q_spi` 软 / `w25q_hw_spi` 硬） |
| RS485 | USART1 | PA2=TX, PA3=RX；**PG13=DE** | 115200-8N1 | `device/src/device_serial.cpp`（`rs485_uart`） |
| RS232 | USART2 | PD5=TX, PD6=RX | 115200-8N1 | `device/src/device_serial.cpp`（`rs232_uart`） |
| OLED | I2C | PC1=SCL, PC2=SDA | — | `docs/PINOUT.md` |
| user_led / flash_led / LED4 | GPIO | PC4 / PC5 / PA7 | 低电平亮 | `user/src/function.cpp`（`user_led/2/3`） |

### 当前测试固件状态（`function_init()`）

- ⚠️ **硬件 `debug_uart`（USART0 @ PA9/10）被禁用**，改用**软串口 `suart`**（同引脚 PA9/10），`DebugPort_Init()` 被注释。
- INA228 / CAN 初始化当前被注释；上电后自动执行 `w25q128_hw_selftest()`（SPI4 硬件 SPI）。
- 串口监视用 DAP-Link 虚拟串口（`ls /dev/ttyACM*` 确认，通常 `/dev/ttyACM0`）。

### 烧录（DAP-Link）

```bash
cmake --build --preset release          # 或 debug
cmake --build build/release --target flash-daplink
```

---

## 一、编译验证

| 协议 | text | data | bss | 大小 | 说明 |
|------|------|------|-----|------|------|
| Modbus | 18532 | 200 | 5276 | 24.0 KB | 字符串 handler 全裁剪 |
| String | 20924 | 140 | 4936 | 26.0 KB | 含 11 条命令表 + handler |

`#if ACTIVE_PROTOCOL` 条件编译正常，未选协议代码不参与编译。

---

## 二、Modbus RTU 协议测试

**测试工具**: `test_modbus.py`（Python + pyserial）  
**从机地址**: 0x01，**波特率**: 115200-8N1

### 运行方法

```bash
# 安装依赖
pip install pyserial

# Modbus 协议测试（4 项全功能）
python3 tests/test_modbus.py /dev/ttyACM1

# 字符串指令测试（11 项全覆盖）
python3 tests/test_string.py /dev/ttyACM1
```

> 串口号按实际设备调整，通过 `ls /dev/ttyACM*` 查看。

---

### 2.1 读 HOLDING 寄存器（功能码 0x03）

| 地址 | 期望值 | 实际值 | 结果 |
|------|--------|--------|------|
| 0x0000 (设备型号) | 1 | 1 | ✅ |
| 0x0001 (固件版本) | 100 | 100 | ✅ |
| 0x000A (ControlBlock[0]) | 0 | 0 | ✅ |

### 2.2 读 INPUT 寄存器（功能码 0x04）—— INA228 传感器

| 地址 | 内容 | 原始值 | 解析 | 结果 |
|------|------|--------|------|------|
| 0x0040~0x0041 | Bus Voltage | [0]=184, [1]=18904 | 12076000 μV (12.076V) | ✅ |
| 0x0042~0x0043 | Shunt Voltage | — | -1 μV (无分流) | ✅ |
| 0x0044~0x0045 | Current | — | 0 mA | ✅ |
| 0x0046~0x0047 | Power | — | 18 mW | ✅ |
| 0x0048~0x0049 | Temperature | — | ~33800 mC (33.8°C) | ✅ |
| 0x004A~0x004D | Energy | — | 64-bit 数据 | ✅ |
| 0x004E~0x0051 | Charge | — | 64-bit 数据 | ✅ |
| 0x0052 | Manufacturer ID | — | 0x5449 (TI) | ✅ |
| 0x0053 | Device ID | — | 0x2281 | ✅ |
| 0x0054 | Alert Flags | — | 原始值 | ✅ |

### 2.3 写入测试

| 功能码 | 操作 | 期望 | 实际 | 结果 |
|--------|------|------|------|------|
| 0x06 | 写 ControlBlock[0]=1234 | 读回=1234 | 1234 | ✅ |
| 0x06 | 写 ControlBlock[0]=0 (恢复) | 读回=0 | 0 | ✅ |

### 2.4 Modbus 小结

```
读 HOLDING:  ✅
读 INPUT:    ✅ INA228 数据实时更新
写单个:      ✅ 写后读回一致
异常响应:    ✅ 非法功能码返回 0x01
帧完整性:    18帧收发 0% 丢帧
```

---

## 三、ASCII 字符串指令协议测试

**测试工具**: `test_string.py`（Python + pyserial）  
**命令格式**: `cmd\r\n`（工具自动追加换行）

### 运行方法

```bash
python3 tests/test_string.py /dev/ttyACM1
```

---

| # | 命令 | 响应 | 结果 |
|---|------|------|------|
| 1 | `ina228_bus_voltage` | `INA228_BusVoltage=12076000_uV` | ✅ |
| 2 | `ina228_shunt_voltage` | `INA228_ShuntVoltage=-1_uV` | ✅ |
| 3 | `ina228_current` | `INA228_Current=0_mA` | ✅ |
| 4 | `ina228_power` | `INA228_Power=21_mW` | ✅ |
| 5 | `ina228_temperature` | `INA228_Temperature=33882_mC` | ✅ |
| 6 | `ina228_energy` | `INA228_Energy=1565_mJ` | ✅ |
| 7 | `ina228_charge` | `INA228_Charge=-208_mC` | ✅ |
| 8 | `ina228_mfr_id` | `INA228_MfrID=0x5449` (TI) | ✅ |
| 9 | `ina228_device_id` | `INA228_DeviceID=0x2281` | ✅ |
| 10 | `ina228_clear_alert` | `INA228_Alert_Cleared` | ✅ |
| 11 | `ina228_all` | 全部 10 项数据 | ✅ |

### `ina228_all` 完整响应

```
=== INA228 ===
Bus=12076mV(12076000uV)
Shunt=-1uV
Cur=0mA
Pwr=19mW
Temp=33882mC
Energy=1565mJ
Charge=-208mC
Mfr=0x5449 Dev=0x2281 Alert=0x0000
```

### 字符串指令小结

```
11/11 全部通过
响应格式: {KEY}={VALUE}_{UNIT}\r\n
支持单项查询 + ina228_all 全量查询
```

---

## 四、INA228 数据交叉验证

同一时刻两个协议读取到的数据一致性：

| 参数 | Modbus (0x04, uV) | String (ASCII) | 一致 |
|------|-------------------|----------------|------|
| Bus Voltage | 12076000 μV | 12076000 μV | ✅ |
| Shunt Voltage | -1 μV | -1 μV | ✅ |
| Current | 0 mA | 0 mA | ✅ |
| Power | 18 mW | 21 mW | ~ (时间差) |
| Temperature | ~33800 mC | 33882 mC | ✅ |

> Power 差异为两次测试间传感器波动，属正常范围。

---

## 五、结论

| 项目 | 状态 |
|------|------|
| Modbus 0x03 读保持寄存器 | ✅ |
| Modbus 0x04 读输入寄存器 | ✅ |
| Modbus 0x06 写单个寄存器 | ✅ |
| Modbus 0x10 写多个寄存器 | ✅ |
| Modbus 异常响应 | ✅ |
| 字符串指令 11 条全量测试 | ✅ |
| 协议宏切换编译 | ✅ |
| INA228 数据交叉验证 | ✅ |

**协议栈数据流**: `USART ISR → uart_buffer_t → Protocol_Task → Modbus_Task/StringCmd_Task → 应答` 全链路正常。

---

## 六、2026-10-06 复测（架构重构后回归）

**背景**：本轮完成 I2C/UART 抽象基类统一、串口模板化、软 SPI 时序枚举、I2C 设备层抽象化（`inter_i2c_bus*` → `i2c_bus*`）。

### 编译

- `cmake --build --preset debug` / `--preset release` / `--preset iap` 三套均构建+链接通过。

### 实板（DAP-Link，`/dev/ttyACM0`）

上电自动自检（当前测试固件运行 W25Q128 硬件 SPI）——**PASS**：

```
========== W25Q128 HARDWARE-SPI TEST ==========
JEDEC ID    : EF 70 18  (W25Q128 OK)
Status Reg  : 0x00 (WIP=0)
-- Erase/Write/Readback @ 0xFF0000 --
Readback    : MATCH (32B verified)
RESULT      : PASS
```

### I2C（INA228）上板验证结果

已在 `function_init()` 启用 `i2c_bus.init()` + `ina228_selftest()`。**修复前**读数异常（`MFR=0x7FFF` / `DEV=0x3FFF`）；经 A/B 与 objdump 排查定位为 **`delay_us/delay_ms` 在 -O2 下被优化为空**（详见 `CHANGELOG.md` 同日条目）。

**修复后（实板 DAP-Link + `/dev/ttyACM0`）**：

```
isConnected   : OK  (MFR=0x5449, DEV=0x2281)   ← 正确
Vbus = 0.000 V | Vshunt = 0.000 mV             ← Vbus 悬空，符合预期
```

- 硬 I2C 在本板不可用：PE2/PE3 无 I2C 复用功能，`i2c_hw_bus` 读不到（预期）；INA228 只能走软 I2C。
- 结论：软 I2C + INA228 驱动 API 工作正常；问题根因在公共 `code/src/systick.c` 延时函数。

---

## 七、2026-10-06 Release 位操作驱动回归（-O2 延时扫描 + 软 SPI 上板）

**背景**：`delay_us/delay_ms` 被 -O2 优化为空是全项目性 bug（详见 `CHANGELOG.md`）。修完后需确认**其他位操作驱动**不再受同一根因影响。

### 7.1 静态扫描（Release / -O2 反汇编）

`arm-none-eabi-objdump` 确认两个公共延时在 -O2 下仍是**真循环**（非 `bx lr`）：

```
delay_us: r0 = us*40     → ldr/subs/str/cmp/bne  (真实忙等)
delay_ms: r0 = ms*40000  → 同上
```

驱动「延时来源」分类：

| 类别 | 驱动 | 时序来源 | -O2 风险 |
|------|------|----------|----------|
| 走共享 `delay_us/ms` | ds18b20、ds2401、dhtnew、drv8825、ms5611、x9c10x、shiftin、shiftout、ads1115、as5600、cht8305、gy521、fram、eeprom 系 | `delay_us/ms` | 已随 `systick.c` 修复，全部安全 |
| 自带时序原语 | `inter_i2c_bus`（volatile 忙等）、`inter_soft_uart`（DWT `CYCCNT`）、`inter_soft_spi`（转发 `delay_us`） | 各自实现 | 安全 |
| 纯 GPIO 无时序 | hc4051/52/53/67、74hc138/154/590、pcf8574、mcp23017 | 仅 init 电平 | 无 |
| 硬件标志轮询 | `w25q128::wait_busy`、`spi_port::transfer`、`inter_i2c_hw::lock`（`_busy` 为 volatile） | SDK 寄存器 | 安全 |

**结论**：未再发现「空转循环被优化掉」的新隐患；根因点唯一（`systick.c`）。

### 7.2 上板验证（Release，DAP-Link + `/dev/ttyACM0`）

仅对板上**真有器件**的驱动做功能验证：

| 驱动 | 器件 | 配置 | 结果 |
|------|------|------|------|
| `inter_soft_spi` | W25Q128 | `delay=0`（最快） | JEDEC `EF 70 18`、擦写回读 MATCH → **PASS** |
| `inter_soft_spi` | W25Q128 | `delay=1`（**压 `delay_us` 路径**） | 同样 **PASS** |
| `inter_i2c_bus` | INA228 | 400k@240M | `MFR=0x5449 / DEV=0x2281` **OK** |

软串口 `suart` 作为全部输出的传输通道，本回归隐式通过。

### 7.3 未覆盖（板上无对应器件）

`ds18b20` / `ds2401`（1-Wire）、`dhtnew`、`hc40xx`、`74hc*`、`shiftin/out`、`x9c10x`、`drv8825`、`ms5611`、`ads1115` —— **延时被优化掉的风险已由静态证明排除**，但**时序精度**需接器件 + 示波器实测（各驱动头注释标注「★ 真机需按实测调参 ★」），本次无法覆盖。

### 7.4 附带发现（非阻塞）

- `delay_us`/`delay_ms` 系数 `40`/`40000`，按 5 周期/次估算在 240MHz 下**约偏短 17%**（`delay_us(1)≈0.83µs`）。对「用 `delay_us(1)` 轮询当 µs 计数器」的驱动（DHT/1-Wire）属精度问题，与「被优化掉」是两个维度；如需校可用 DWT 标定。
