# GD32F470 项目开发计划

> **分支策略**：`planB` 开发 → 验证通过 → 合并到 `master`
>
> 架构总览、外设状态表、编码规范 → 详见 [.agents/skills/my-gd-skill/SKILL.md](/.agents/skills/my-gd-skill/SKILL.md)
> 变更历史 → 详见 [changelog.md](changelog.md)

---

## 一、已知技术债务

| 问题 | 文件 | 严重度 | 计划 |
|------|------|--------|------|
| `usart_port` 使用 `new[]` 堆分配 | `interface/src/inter_usart.cpp` | ~~中~~→ ✅ | Phase 1.1 已完成 |
| `inter_i2c_bus` 用 `while(_busy)` 自旋锁 | `interface/src/inter_i2c_bus.cpp` | 低 | Phase 1.2 |
| `hardware/` 有 12 个旧 C 驱动未编译 | `hardware/src/*.c` | 信息 | 逐步迁移或清理 |

---

## 二、分阶段计划

### Phase 1：基础重构（当前阶段 — de-risk）

| 序号 | 功能 | 目标文件 | 说明 |
|------|------|----------|------|
| **1.1** | 重构 `usart_port` | `interface/inter_usart.hpp/cpp` | ✅ 消除 `new[]`，模板静态 buffer |
| **1.2** | 重构 `inter_i2c_bus` 锁 | `interface/inter_i2c_bus.hpp/cpp` | `while(_busy)` → `critical_spinlock` |

### Phase 2：核心外设封装

| 序号 | 功能 | 目标类名 | 依赖 | 测试方式 |
|------|------|----------|------|----------|
| **2.1** | SPI 主机 | `spi_port` | `io_ctrl` × 4 | W25Q128 JEDEC ID ✅ |
| **2.2** | 定时器/PWM | `timer_port` | 无（纯寄存器） | 示波器测量 PWM 输出 |

### Phase 3：通信外设

| 序号 | 功能 | 目标类名 | 说明 |
|------|------|----------|------|
| **3.1** | CAN 总线 | `can_port` | `io_ctrl` × 2，含 CAN 帧收发 |
| **3.2** | 硬件 I2C | `i2c_hw_bus` | `io_ctrl` × 2，替代软 I2C |

### Phase 4：模拟与系统外设

| 序号 | 功能 | 目标类名 |
|------|------|----------|
| **4.1** | ADC | `adc_port` |
| **4.2** | DAC | `dac_port` |
| **4.3** | DMA | `dma_port` |
| **4.4** | RTC | `rtc_port` |
| **4.5** | 外部中断 | `exti_port` |
| **4.6** | 看门狗 | `wdt_port` |

---

## 三、当前状态

| 项目 | 状态 |
|------|------|
| 分支 | `planB`（当前工作分支） |
| 编译 | ✅ 零错误 |
| Modbus 0x03/0x06 读写 | ✅ |
| Modbus 0x04 INA228 数据 | ✅ |
| 安全性审查 | ✅ 已加固 |
| usart_port 冒烟 | ✅ |
