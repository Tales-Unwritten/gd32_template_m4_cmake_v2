# 采集卡 FreeRTOS 练习任务集

> **用途**：长期留存的练习任务书（**最终版**）。
> 每份「作业」= 一个要交付的需求，结构固定为 **功能 / 限定 / 验收** 三段。
> 目标不是"练某个 API"，而是**熟练地把一个需求实现出来**（含边界与容错）。
>
> **出题标准**见 [`practice_task_spec.md`](practice_task_spec.md)。

---

## 0. 学习方式（配合 AI 进行）

1. AI 出题：只给 **功能 / 限定 / 验收** 三段。
2. 学习者自己实现：只在**白名单函数**与**允许改动的文件**范围内编码。
3. 交付：改动文件清单 + 关键代码 + 串口**实测原始输出** + 遇到的问题。
4. AI 审核：按评分维度打分 → 指导修改 → 总结错误 → 放行下一题。

> 规则：**AI 只出题、不代写实现**；申请白名单外 API 需先说明理由。

---

## 1. 学习者画像与目标

- **当前水平**：刚学会 `xTaskCreate` 创建任务。
- **目标**：以真实需求为线索，熟练运用任务 / 队列 / 互斥 / 信号量 / 事件组 /
  软件定时器 / 任务同步，并练就**纯代码能力**（缓冲、协议解析、状态机、滤波、内存管理…）。
- **原则**：一题一个需求；难度递进；每份都可独立验收。

---

## 2. 产品设定：数据采集卡 ACQCARD

基于 GD32F470 的数据采集卡，上位机通过**调试串口**用 ASCII 指令交互。

### 2.1 硬件资源（均已封装为项目内 API）

| 资源 | 对象 / 引脚 | 说明 |
|------|-------------|------|
| 主控 | GD32F470ZIT6 | Cortex-M4F @240MHz，FreeRTOS，1ms tick，8 优先级，32KB heap |
| 调试串口 | `debug_uart`：USART0，PA9-TX / PA10-RX，AF7，115200-8N1 | **上位机数据通道** |
| RS485 | `rs485_uart`（PA2/PA3）、`en_485`（PG13） | 从站总线 |
| 功率监测 | `INA228_2`：软 I2C PE2/PE3，地址 0x40 | 电压 / 分流 / 电流 / 功率 / 温度 / 电能 / 电荷 |
| 温湿度 | `SHT31`：软 I2C | 温度 / 湿度 |
| EEPROM | `device_eeprom`（AT24C02，256B） | 配置 / 校准数据 |
| 外部 Flash | `w25q128`（16MB，SPI） | 历史数据 |
| LED | `user_led` PC4、`flash_led` PC5、`state_led` PA7 | 状态指示 |
| 其他 | 按键（`exti_port`）、拨码开关、CAN（`can_port`）、ADC、DAC、RTC、IWDG | 见各作业 |

### 2.2 指令框架（项目已有）

解析入口 `command_process(line, reply, size)`；命令表 `g_command_table[]`
（`protocol/string/src/str_cmd.cpp`）；桥接表 `string_ports[]`（`string_pro.cpp`）——
**数据从哪个口来，回复就从哪个口回**。调用链：
`default_task → function_loop → APP_PC_Task → Protocol_Task → StringCmd_Task`（依赖 `ACTIVE_PROTOCOL == PROTOCOL_STRING_CMD`）。

---

## 3. 通用约定

### 3.1 每份作业的固定结构（三段式）

| 段 | 内容 |
|----|------|
| **功能** | 交付什么；指令类须**按器件能力铺满** |
| **限定** | ① 只能改的文件 ② 只能用（**精确到函数名**）③ 技术 |
| **验收** | 1~4 条可复现的通过条件（含边界/异常/稳定性） |

### 3.2 允许改动范围（长期遵守）

- 分层：`Firmware/`（只读）、`interface/`（唯一可调 GD32 SDK 处）、`device/`、`protocol/`、`app/`、`user/`。
- SDK 调用**只能在 `interface/src/`**；GPIO 操作**只能经 `io_ctrl`**。
- 文件名小写 + 下划线；源码 LF 换行；修改后更新 `docs/CHANGELOG.md`。
- 涉及接口签名 / 增删类 / 分层依赖变更时，**先出方案、等授权**。

### 3.3 交付物

改动文件清单 + 关键代码片段 + 串口**原始输出** + 遇到的问题与解决过程。

### 3.4 评分维度

| 维度 | 关注点 |
|------|--------|
| 需求符合度 | 各功能点逐条满足；边界行为与说明一致 |
| 分层 / 规范 | SDK 不出 `interface/`、GPIO 走 `io_ctrl`、命名、LF |
| 并发安全 | `volatile`、互斥、撕裂读、`FromISR` |
| 实时性 | 无忙等、无阻塞采样、无栈溢、ISR 极简 |
| 容错 | 非法输入、超时、队列满、拔线、复位 |
| 交付 | changelog、说明清晰度 |

---

## 4. 任务索引

| 编号 | 标题 | 难度 |
|------|------|------|
| A0 | INA228 ASCII 采集指令接口 | L1 |
| A1 | 周期采集 + 按需 / 流式上报 | L2 |
| B1 | 设备参数存取（AT24C02，掉电恢复） | L2 |
| B2 | 采样环形记录到 W25Q128 + 回放 | L3 |
| B3 | 状态指示灯花样 | L2 |
| B4 | 按键短按 / 长按 / 双击识别 | L2 |
| B5 | RS485 半双工主从问答 | L2 |
| B6 | 双板 CAN 周期互发状态 | L2 |
| B7 | 多 I2C 传感器轮询采集 | L2 |
| B8 | ADC 采样 + 校准上报 | L2 |
| B9 | RTC 定时采集 + 时间戳 | L3 |
| B10 | 任务健康看门狗 | L3 |
| B11 | Modbus 从站寄存器映射 | L2 |
| C1 | 无锁环形缓冲区 | L2 |
| C2 | 二进制帧解析状态机 | L2 |
| C3 | CRC 校验库 | L1 |
| C4 | 数字滤波库 | L2 |
| C5 | 分级日志系统 | L2 |
| C6 | 通用状态机框架 | L2 |
| C7 | 固定块内存池 | L2 |
| C8 | TLV 编解码 | L2 |
| C9 | 命令解析增强 | L2 |
| C10 | 软件定时器管理器 | L3 |
| C11 | 定点数与单位换算库 | L1 |
| C12 | 简易协作式调度器 | L3 |

---

## 5. 主线作业

### 作业 A0 — INA228 ASCII 采集指令接口

**功能**
把上位机通道切到硬件串口 `debug_uart`，让 INA228 全部能力都能通过 ASCII 指令读取：

| 指令（以 `\r\n` 结尾） | 回复 |
|-------------------------|------|
| `ina228_bus_voltage` | 总线电压 mV |
| `ina228_shunt_voltage` | 分流电压 uV |
| `ina228_current` | 电流 mA |
| `ina228_power` | 功率 mW |
| `ina228_temperature` | 温度 mC |
| `ina228_energy` | 电能 mJ |
| `ina228_charge` | 电荷 mC |
| `ina228_mfr_id` / `ina228_device_id` | 器件信息 |
| `ina228_alert` / `ina228_clear_alert` | 读报警 / 清报警锁存 |
| `ina228_all` | 一次性返回全部 |
| 其他任意内容 | 静默丢弃，不回复 |

**限定**
- 只能改：`user/src/function.cpp`、`protocol/string/src/string_pro.cpp`、`protocol/string/src/str_cmd.cpp`
- 只能用：`DebugPort_Init()`、`debug_uart.init()` / `debug_uart.send_data()` / `debug_uart.buffer()`、`command_process()`、`INA228_2.getBusVoltage_mV()` / `getShuntVoltage_uV()` / `getCurrent_mA()` / `getPower_mW()` / `getTemperature_mC()` / `getEnergy_mJ()` / `getCharge_mC()` / `getManufacturerID()` / `getDeviceID()` / `readAlertFlags()` / `readDiagAlertRaw()` / `clearAlertLatch()`、`snprintf()`
- 技术：字符串指令框架（命令表 + handler）、`ACTIVE_PROTOCOL` 条件编译

**验收**
1. 115200 下发 `ina228_all\r\n` → 收到完整一帧全部字段；
2. 每个 `ina228_*` 指令都返回对应数据；
3. 发未知指令 `foo\r\n` → 无任何回复；
4. 发一行 300 字节 → 不 HardFault、不卡死；连发 100 次不丢帧。

---

### 作业 A1 — 周期采集 + 按需 / 流式上报

**功能**
采集卡持续采集 INA228，上位机可随时取最新值，并可开关周期上报：

- `SAMPLE?` → 立即回最新一帧
- `STREAM 1` / `STREAM 0` → 开 / 关流式上报，回 `OK`
- 流式开启时每 100ms 上行一帧 `SEQ,V,I,P,T\r\n`；关闭时不上行
- 未知指令静默丢弃

**限定**
- 只能改：`user/src/*.cpp`、`user/inc/*.hpp`、`protocol/string/src/str_cmd.cpp`
- 只能用：
  - 任务：`xTaskCreate`、`vTaskDelay`、`xTaskDelayUntil`、`pdMS_TO_TICKS`、`uxTaskGetStackHighWaterMark`
  - 队列：`xQueueCreate`、`xQueueSend`、`xQueueReceive`
  - 互斥：`xSemaphoreCreateMutex`、`xSemaphoreTake`、`xSemaphoreGive`
  - 采集：`INA228_2.getBusVoltage_mV()` / `getCurrent_mA()` / `getPower_mW()` / `getTemperature_mC()`
  - 串口：`debug_uart.send_data()` / `debug_uart.buffer()`
  - 其他：`command_process()` 机制、`snprintf()`、`memcpy()`
- 技术：任务、队列解耦、互斥量保护共享串口、`volatile` 跨任务标志

**验收**
1. `STREAM 1` 后约每 100ms 一帧、`SEQ` 递增；`STREAM 0` 后停止；
2. `SAMPLE?` 随时能取到最新值（不必等下一次采样）；
3. 拔掉上位机后采样不停、`SEQ` 继续增长；重连后 `SAMPLE?` 得到持续更新的数据；
4. 指令回复与流式帧**不交错**；连续跑 ≥10 分钟无 HardFault、栈有余量。

---

## 6. 外设 / 产品作业

### 作业 B1 — 设备参数存取（AT24C02，掉电恢复）

**功能**
把设备参数（采样周期、上限阈值、设备名等）存到 AT24C02，掉电后上电恢复：

| 指令 | 回复 |
|------|------|
| `cfg_save` | 保存当前参数，回 `OK` |
| `cfg_load` | 从 EEPROM 载入，回 `OK` 或 `ERR` |
| `cfg_show` | 回当前参数文本 |
| `cfg_set <key> <value>` | 设置一项，回 `OK` 或 `ERR` |
| 其他 | 静默丢弃 |

**限定**
- 只能改：`user/src/*`、`user/inc/*`、`protocol/string/src/str_cmd.cpp`
- 只能用：`device_eeprom`（构造 + `init()` / `read_config<T>()` / `write_config<T>()` / `verify_config<T>()` / `probe()` / `read()` / `write()`）、`command_process()` 机制、`snprintf()`、`strcmp()`、`strtol()`、`memcpy()`
- 技术：结构化配置存取（magic + version + CRC）、定点参数、版本兼容

**验收**
1. `cfg_set` → `cfg_save` → 断电重启 → `cfg_show` 恢复为上次值；
2. EEPROM 空白首次上电 `cfg_load` 回 `ERR` 并使用默认值；
3. `cfg_show` 与 `cfg_set` 一致（含负数 / 边界值）；
4. 回读校验失败时返回错误且不崩溃。

---

### 作业 B2 — 采样环形记录到 W25Q128 + 回放

**功能**
把采样帧持续写入 W25Q128 环形区，支持指令回放：

| 指令 | 回复 |
|------|------|
| `log_info` | 总条数 / 容量 / 写指针 |
| `log_read <n>` | 最近第 n 条记录 |
| `log_clear` | 清空记录区，回 `OK` |
| 其他 | 静默丢弃 |

**限定**
- 只能改：`user/src/*`、`user/inc/*`、`protocol/string/src/str_cmd.cpp`
- 只能用：`spi_port`（构造 + `init()` / `transfer()` / `cs_select()` / `cs_deselect()`）、`w25q128`（`init()` / `jedec_id()` / `read()` / `write()` / `sector_erase()` / `wait_busy()`）、`xQueueCreate` / `xQueueSend` / `xQueueReceive`、`xTaskCreate` / `vTaskDelay`、`command_process()` 机制、`snprintf()`
- 技术：环形存储（记录号取模）、扇区擦除策略、队列解耦

**验收**
1. 写满一圈后新记录覆盖最旧，`log_info` 计数正确；
2. `log_read 0` 回最新一条、`log_read k` 回第 k 新；
3. `log_clear` 后 `log_read` 回无记录；
4. 连续写 1000 条不越界、不 HardFault。

---

### 作业 B3 — 状态指示灯花样

**功能**
用 3 个 LED 表达设备状态，切换即换花样（不能阻塞）：

- 启动中：`state_led` 1Hz 慢闪
- 正常：`state_led` 常亮，每 2s 双击一下
- 报警：`state_led` 5Hz 快闪
- 通信断：`user_led` 双闪（闪两下停一下）
- 每次采样：`flash_led` 闪一下
- 状态来源：收到有效指令→正常；`INA228_2` 报警→报警；超时无上位机→断连

**限定**
- 只能改：`user/src/*`
- 只能用：`io_ctrl.high()` / `low()` / `toggle()` / `set()`、`xTaskCreate`、`vTaskDelay`、`xTaskDelayUntil`、`xTimerCreate` / `xTimerStart` / `xTimerStop`、`pdMS_TO_TICKS`、`xTaskNotifyGive` / `ulTaskNotifyTake`、`INA228_2.readAlertFlags()`
- 技术：软件定时器 / 非阻塞状态机（禁止用阻塞 `vTaskDelay` 拼花样）

**验收**
1. 四种花样可辨且互不阻塞，切状态即切换；
2. 报警状态由 INA228 报警触发切换；
3. 关掉上位机后进入断连花样；
4. 长时间运行不漂移、LED 任务不独占 CPU。

---

### 作业 B4 — 按键短按 / 长按 / 双击识别

**功能**
识别单按键的短按、长按（≥1s）、双击并映射动作：

- 短按：切换 `state_led` 开 / 关
- 双击：切换上报开关（复用 A1 的 `STREAM`）
- 长按：恢复默认参数（复用 B1 默认值）
- 指令 `btn_events` → 回最近事件计数

**限定**
- 只能改：`user/src/*`
- 只能用：`exti_port`（构造 + `init()` / `on_interrupt()` / `enable()`）、`io_ctrl.read()`、`xTaskCreate`、`vTaskDelay`、`xTaskNotifyGiveFromISR` / `vTaskNotifyGiveFromISR`、`xSemaphoreGiveFromISR`、`pdMS_TO_TICKS`、`xTaskGetTickCount()`
- 技术：EXTI + `FromISR` 同步、去抖状态机、时间测量

**验收**
1. 短 / 长 / 双击识别准确（各测 20 次）；
2. 抖动（快速连按）不误判；
3. ISR 极简（只通知，不处理业务）；
4. EXTI 中断优先级 ≤ `configLIBRARY_MAX_SYSCALL_INTERRUPT_PRIORITY`（=5）。

---

### 作业 B5 — RS485 半双工主从问答

**功能**
采集卡作为 RS485 从站，接收主机指令、回复 INA228 数据（复用 A0 指令集），收发时正确控制 DE 引脚 `en_485`，并处理无响应超时。

**限定**
- 只能改：`user/src/*`、`protocol/string/src/string_pro.cpp`（端口表加 `rs485_uart`）
- 只能用：`RS485Port_Init()`、`rs485_uart.init()` / `send_data()` / `buffer()`、`en_485.high()` / `low()`、`command_process()` 机制、`xTaskCreate` / `vTaskDelay`
- 技术：半双工方向时序（发前拉高 DE、发完拉低）、帧间隔、超时

**验收**
1. 主机经 485 发 `ina228_all` 收到正确回复；
2. DE 时序正确（不丢首字节、不截尾字节）；
3. 总线无回 / 冲突时不卡死。

---

### 作业 B6 — 双板 CAN 周期互发状态

**功能**
两块板通过 CAN 周期性互发状态帧（电压 / 电流 / 计数），收到对方帧即回显到串口并点灯。

**限定**
- 只能改：`user/src/*`、`protocol/string/src/str_cmd.cpp`
- 只能用：`can_port.init()` / `send()` / `recv()` / `rx_ready()`、`xTaskCreate`、`xTaskDelayUntil`、`xQueueCreate` / `xQueueSend` / `xQueueReceive`、`pdMS_TO_TICKS`、`debug_uart.send_data()`、`snprintf()`
- 技术：周期发送任务、接收任务、队列、标准帧 ID 约定

**验收**
1. 双板互收，串口每秒打印对方数据；
2. 拔掉一板，另一板不阻塞；
3. 发送 / 接收计数一致（无丢失）。

---

### 作业 B7 — 多 I2C 传感器轮询采集

**功能**
在同一 I2C 总线上轮询 INA228 与 SHT31，采集电压 / 电流 / 温度 / 湿度并上报（复用 A1 的 `SAMPLE?` / `STREAM`）。

**限定**
- 只能改：`user/src/*`
- 只能用：`SHT31`（构造 + `init()` / `read()` / `getTemperature_mC()` / `getHumidity_x100()` / `isConnected()`）、`INA228_2` 测量 API、`inter_i2c_bus.lock()` / `unlock()` / `bus_recovery()`、`xTaskCreate` / `vTaskDelay` / `xTaskDelayUntil`、`debug_uart.send_data()`、`snprintf()`
- 技术：I2C 总线互斥（多设备共享）、任务、队列

**验收**
1. 两传感器数据都能采集且互不干扰；
2. 单设备离线时另一设备仍工作；
3. 长时间轮询无总线死锁（必要时 `bus_recovery()`）。

---

### 作业 B8 — ADC 采样 + 校准上报

**功能**
采集多路 ADC 电压，做多次平均 + 两点线性校准后上报：

| 指令 | 回复 |
|------|------|
| `adc_all` | 各通道电压 mV + 原始值 |
| `adc_cal <ch> <ref_mv>` | 用已知基准点校准该通道 |
| 其他 | 静默丢弃 |

**限定**
- 只能改：`user/src/*`、`protocol/string/src/str_cmd.cpp`
- 只能用：`adc_port.init()` / `read_mv()` / `read_raw()` / `scan_mv()` / `scan_raw()`、`command_process()` 机制、`snprintf()`、`xTaskCreate` / `vTaskDelay`
- 技术：滑动平均滤波、定点线性校准（斜率 + 截距）、队列

**验收**
1. 输入已知电压（如 3.3V / 1.65V）读数误差在设定范围内；
2. 校准后误差减小；
3. 平均能抑制随机噪声（对比单次 vs 平均波动）。

---

### 作业 B9 — RTC 定时采集 + 时间戳

**功能**
RTC 闹钟每 1s 触发一次采集，给每条数据打时间戳：

| 指令 | 回复 |
|------|------|
| `rtc_time` | 当前时间 |
| `rtc_set <YYYY-MM-DD hh:mm:ss>` | 设置时间，回 `OK` |
| `log_ts <n>` | 最近第 n 条（时间 + 电压 / 电流） |
| 其他 | 静默丢弃 |

**限定**
- 只能改：`user/src/*`、`protocol/string/src/str_cmd.cpp`
- 只能用：`rtc_port.init()` / `set_time()` / `get_time()` / `set_alarm()` / `alarm_enable()` / `on_alarm()`、`xQueueCreate` / `xQueueSend` / `xQueueReceive`、`xTaskCreate`、`xEventGroupCreate` / `xEventGroupSetBits` / `xEventGroupWaitBits`、`command_process()` 机制、`snprintf()`
- 技术：RTC 闹钟中断 → 事件组 / 队列通知采集任务、时间格式化

**验收**
1. 每秒产生一条带正确时间戳的记录；
2. 设置时间后时间戳随之更新；
3. 闹钟回调极简（只发通知，不做采集）。

---

### 作业 B10 — 任务健康看门狗

**功能**
多个任务周期性上报心跳，健康监控任务检测某任务超时未上报则报警并尝试恢复（重启该任务 / 复位系统），同时喂 IWDG。

**限定**
- 只能改：`user/src/*`
- 只能用：`wdt_port.iwdg_init()` / `iwdg_feed()`、`xTaskCreate` / `vTaskDelay`、`xSemaphoreCreateCounting` / `xSemaphoreGive` / `xSemaphoreTake`、`eTaskGetState()` / `vTaskGetInfo()` / `uxTaskGetStackHighWaterMark()`、`vTaskDelete()` / `vTaskSuspend()` / `vTaskResume()`、`debug_uart.send_data()`
- 技术：计数信号量心跳、任务状态查询、任务恢复、看门狗

**验收**
1. 人为 `vTaskSuspend` 某任务 → 监控在规定时间内检出并报告；
2. 恢复机制能生效；
3. 全部正常时 IWDG 不被复位；
4. 监控任务自身不阻塞。

---

### 作业 B11 — Modbus 从站寄存器映射

**功能**
把 INA228 数据映射到 Modbus 输入寄存器（0x04），支持 0x03 / 0x06 / 0x10，供上位机读写。

**限定**
- 只能改：`user/src/function.cpp`（`UpdateModbusRegisters`）、`protocol/modbus/*`
- 只能用：`Modbus_Init()` / `Modbus_Task()` / `Modbus_FillData()` / `Modbus_FrameProcess()` / `Modbus_PortSend()`、`modbus_ports[]`、`INA228_2` 测量 API、`StatusBlock[]`、`debug_uart` / `rs485_uart`
- 技术：寄存器映射、多字节大端编码、异常码

**验收**
1. 上位机 `tests/test_modbus.py` 全部通过；
2. 32 / 64 位值的寄存器编码正确（高低字、符号）；
3. 越界寄存器地址返回异常码（0x02）。

---

## 7. 纯代码能力作业

### 作业 C1 — 无锁环形缓冲区

**功能**
实现单生产者-单消费者环形缓冲区，供某数据流使用（如串口收 → 解析），收发两端**不关中断、不加锁**，正确回绕与溢出。

**限定**
- 只能改：`user/src/*`、`user/inc/*`
- 只能用：`volatile`、`memcpy()`、整数运算；可选 `xTaskCreate` / `vTaskDelay` 验证
- 技术：SPSC lock-free、`volatile` head/tail、容量取 2 的幂

**验收**
1. 写满行为符合定义（满则丢弃或覆盖，明确说明）；
2. 大量读写（≥10 万次）不丢失、不越界、不乱序；
3. 生产者 / 消费者并发无撕裂读写。

---

### 作业 C2 — 二进制帧解析状态机

**功能**
解析帧 `AA 55 | LEN | PAYLOAD | CRC16 | 55 AA`，处理粘包（一读多帧）、半包（跨读补全）、坏 CRC 丢弃、超时丢帧。

**限定**
- 只能改：`user/src/*`、`user/inc/*`
- 只能用：数组 / 状态机、整数运算；可选 `xTaskCreate`；可复用 C3 的 CRC
- 技术：接收状态机、缓冲区管理、帧超时

**验收**
1. 逐字节喂入含多帧 / 半帧的流，全部正确解析；
2. 坏 CRC 帧被丢弃且不影响后续帧；
3. 帧中途超时后正确复位到找帧头。

---

### 作业 C3 — CRC 校验库

**功能**
实现 CRC8(poly 0x07)、CRC16-CCITT(0x1021)、CRC32(0x04C11DB7)，提供逐字节与整块接口（可选查表加速），并带标准测试向量自测。

**限定**
- 只能改：`user/src/*`、`user/inc/*`
- 只能用：位运算、`const` 查表、`uint*_t`
- 技术：位运算 / 查表、测试向量

**验收**
1. 对 `"123456789"`：CRC8=0xF4、CRC16-CCITT=0x29B1、CRC32=0xCBF43926；
2. 逐字节与整块结果一致；
3. 查表与位运算版本结果一致。

---

### 作业 C4 — 数字滤波库

**功能**
实现滑动平均、中值滤波、一阶低通（定点）三种滤波器，并对同一噪声序列对比效果。

**限定**
- 只能改：`user/src/*`、`user/inc/*`
- 只能用：整数 / 定点运算、数组；可选 `INA228_2` / `adc_port` 作数据源；`snprintf()` / `debug_uart.send_data()` 输出对比
- 技术：定点运算、环形窗口、去极值

**验收**
1. 三种滤波对阶跃 / 噪声响应合理；
2. 窗口长度变化时结果正确；
3. 无浮点、无越界。

---

### 作业 C5 — 分级日志系统

**功能**
实现日志系统，等级 ERROR / WARN / INFO / DEBUG，运行期可切换等级，输出含 tick 时间戳与等级；带环形日志缓冲，可指令回取最近 N 条。

**限定**
- 只能改：`user/src/*`、`user/inc/*`
- 只能用：`vsnprintf()` / `snprintf()`、`xTaskGetTickCount()`、`debug_uart.send_data()`、环形缓冲（可用 C1）
- 技术：变参、等级过滤、环形日志、可选互斥

**验收**
1. 切等级后低于阈值的不输出；
2. 每条日志带正确 tick 时间戳；
3. 缓冲满时行为明确（覆盖最旧）；
4. 多任务同时打日志不乱。

---

### 作业 C6 — 通用状态机框架

**功能**
实现通用状态机：状态 + 事件 + 转移表（含进入 / 退出动作与守卫条件），并用它描述"LED 状态机"或"按键模式机"验证。

**限定**
- 只能改：`user/src/*`、`user/inc/*`
- 只能用：函数指针、结构体数组、枚举
- 技术：表驱动状态机、函数指针、非法转移处理

**验收**
1. 转移表正确驱动状态变化（非法事件被忽略或落 default）；
2. 进入 / 退出动作按序执行；
3. 同一实现可用于两种不同状态机。

---

### 作业 C7 — 固定块内存池

**功能**
实现固定块大小内存池：初始化一段静态内存，提供 alloc / free，无碎片、O(1)，替代某功能的 malloc。

**限定**
- 只能改：`user/src/*`、`user/inc/*`
- 只能用：静态数组、对齐、位图 / free list、指针
- 技术：free list 或位图、对齐、双重释放 / 耗尽检测

**验收**
1. 反复分配 / 释放 ≥10 万次不泄漏、不碎片；
2. 耗尽时返回空且不影响已有块；
3. 非法释放被拦截（不崩溃）。

---

### 作业 C8 — TLV 编解码

**功能**
实现 TLV（Type-Length-Value）编码 / 解码，把多个字段打包上报并从缓冲区还原。

**限定**
- 只能改：`user/src/*`、`user/inc/*`
- 只能用：数组、`memcpy()`；可选 `debug_uart` 收发验证
- 技术：序列化、边界校验、端序统一

**验收**
1. 编码 → 解码往返一致（含多字段、变长）；
2. 截断 / 超长 / 非法长度输入安全返回错误；
3. 可与 C2 帧解析组合使用。

---

### 作业 C9 — 命令解析增强

**功能**
在现有命令框架上增强解析：支持多参数（空格分隔）、参数类型（十进制 / 十六进制整数、字符串）、命令别名、大小写不敏感。

**限定**
- 只能改：`protocol/string/*`、`user/src/*`
- 只能用：`strtol()`、`strcmp()`、`strchr()`、`snprintf()`、`command_process()` 机制
- 技术：分词、类型转换、边界处理

**验收**
1. `set_rate 100`、`set_rate 0x64` 均被接受；
2. 大小写混写命令可识别；
3. 参数缺失 / 非法返回明确错误；
4. 不影响既有命令。

---

### 作业 C10 — 软件定时器管理器

**功能**
不依赖 FreeRTOS 定时器，实现多路软件定时器：注册 N 路（周期 / 单次），基于 1ms tick 检查，到期回调。

**限定**
- 只能改：`user/src/*`、`user/inc/*`
- 只能用：`xTaskGetTickCount()` / `get_tick()`、数组 / 最小堆、函数指针、`xTaskCreate`
- 技术：tick 计数、时间轮 / 最小堆、回调、溢出安全

**验收**
1. 多路不同周期定时准确（误差在几个 tick 内）；
2. 单次触发后自动移除；
3. tick 回绕不误触发。

---

### 作业 C11 — 定点数与单位换算库

**功能**
实现定点数工具：整数毫单位放大 / 缩小、四舍五入、饱和、单位换算（mV↔V、mC↔C），统一全项目数值表示。

**限定**
- 只能改：`user/src/*`、`user/inc/*`
- 只能用：整数运算、`int64_t`
- 技术：定点缩放、四舍五入、无溢出中间量、饱和

**验收**
1. 正负值与边界值换算正确（含四舍五入方向）；
2. 溢出时饱和而非回绕；
3. 与 INA228 输出组合打印正确（可配合 C4 / C5）。

---

### 作业 C12 — 简易协作式调度器

**功能**
不用 RTOS，实现基于 tick 的协作式调度器：注册多个周期任务（周期 + 函数），主循环按时间调用，含超时 / 执行时间统计。

**限定**
- 只能改：`user/src/*`、`user/inc/*`
- 只能用：`get_tick()`、函数指针表、数组
- 技术：时间片调度、任务表、错峰、执行时间测量

**验收**
1. 多任务触发周期准确；
2. 某任务耗时过长时不影响其他任务节拍（或按定义报告超时）；
3. 给出与 FreeRTOS 版本的取舍说明。

---

## 8. 变更记录

| 日期 | 变更 |
|------|------|
| 2026-10-09 | 初版：产品设定、通用约定、路线图、作业 A0 / A1 |
| 2026-10-09 | 结构改为三段式（功能/限定/验收）；新增练习方案库（B1~B11、C1~C12） |
| 2026-10-09 | **最终版**：B1~B11、C1~C12 全部展开为完整三段式作业 |
