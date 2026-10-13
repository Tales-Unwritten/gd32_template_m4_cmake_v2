# MickeyPlus V2.0.0 引脚功能表

> 来源：`file/MickeyPlusV2.0.0.enet` 网表，MCU: GD32F450ZGT6 (144-pin)

## 一、已使用引脚

| 引脚 | 网络名 | 功能 | 备注 |
|------|--------|------|------|
| PA9 (101) | DAP_RXD | USART0 TX / DAP-Link 串口 | debug_uart |
| PA10 (102) | DAP_TXD | USART0 RX / DAP-Link 串口 | debug_uart |
| PA13 (105) | PA13 | SWDIO | DAP-Link 调试 |
| PA14 (109) | PA14 | SWCLK | DAP-Link 调试 |
| NRST (25) | DAP_RST | 复位 | DAP-Link 控制 |
| PA2 (36) | PA2 | USART1 TX | RS485 |
| PA3 (37) | PA3 | USART1 RX | RS485 |
| PG13 (128) | 485_EN | RS485 DE 方向 | RS485 |
| PF6 (18) | PF6 | SPI4 CS | W25Q128 Flash |
| PF7 (19) | PF7 | SPI4 CLK | W25Q128 Flash |
| PF8 (20) | PF8 | SPI4 MISO | W25Q128 Flash |
| PF9 (21) | PF9 | SPI4 MOSI | W25Q128 Flash |
| PE2 (1) | I2C_SCL1 | I2C1 SCL | INA228/INA226 |
| PE3 (2) | I2C_SDA1 | I2C1 SDA | INA228/INA226 |
| PB6 (136) | 24C02_SCL | I2C SCL | AT24C02 EEPROM |
| PB7 (137) | 24C02_SDA | I2C SDA | AT24C02 EEPROM |
| PC1 (27) | oled_SCL | I2C SCL | oled 显示屏 |
| PC2 (28) | oled_SDA | I2C SDA | oled 显示屏 |
| PC5 (45) | user_led | LED | 低电平亮 |
| PC4 (44) | flash_led | LED | 低电平亮 |
| PA7 (43) | LED4 | LED | 低电平亮 |
| PG14 (129) | ALERT | INA228 ALERT | 传感器报警 |
| PB15 (76) | LCD_BL | LCD 背光 | PWM 控制 |

## 二、可用引脚（未占用或自由 GPIO）

| 引脚 | 当前网络 | 可复用功能 |
|------|----------|-----------|
| PB8 (139) | PB8 | **CAN0_RX** (AF9) |
| PB9 (140) | PB9 | **CAN0_TX** (AF9) |
| PA0 (34) | PA0 | ADC123_IN0, TIMER2_CH1 |
| PA1 (35) | PA1 | ADC123_IN1, TIMER2_CH2 |
| PA4 (40) | DCMI_HREF | ADC12_IN4 |
| PA5 (41) | PA5 | ADC12_IN5, TIMER2_CH1 |
| PA6 (42) | DCMI_PCLK | TIMER3_CH1 |
| PA8 (100) | DCMI_XCLK | TIMER1_CH1, MCO |
| PA11 (103) | PA11 | CAN1_RX, TIMER1_CH4 |
| PA12 (104) | PA12 | CAN1_TX, TIMER1_ETR |
| PA15 (110) | PA15 | TIMER2_CH1 |
| PB0 (46) | PB0 | TIMER3_CH3 |
| PB1 (47) | PB1 | TIMER3_CH4 |
| PB3 (133) | PB3 | TIMER2_CH2, SPI1_SCK |
| PB4 (134) | PB4 | TIMER3_CH1, SPI1_MISO |
| PB5 (135) | PB5 | TIMER3_CH2, SPI1_MOSI |
| PB10 (69) | PB10 | I2C2_SCL, TIMER2_CH3 |
| PB11 (70) | PB11 | I2C2_SDA, TIMER2_CH4 |
| PB12 (73) | PB12 | SPI2_NSS, TIMER1_BKIN |
| PB13 (74) | PB13 | SPI2_SCK, TIMER1_CH1N |
| PB14 (75) | PB14 | SPI2_MISO, TIMER1_CH2N |
| PC0 (26) | PC0 | ADC123_IN10 |
| PC3 (29) | PC3 | ADC123_IN13 |
| PC10 (111) | PC10 | SPI3_SCK, UART4_TX |
| PC12 (113) | PC12 | SPI3_MOSI, UART5_TX |
| PD2 (116) | PD2 | UART5_RX, SDIO_CMD |
| PD11 (80) | PD11 | — |
| PD12 (81) | PD12 | TIMER4_CH1 |
| PD13 (82) | PD13 | TIMER4_CH2 |
| PD14 (85) | EXMC_D0 | — (EXMC禁用时可用) |
| PD15 (86) | EXMC_D1 | — (EXMC禁用时可用) |
| PE0 (141) | PE0 | — |
| PE1 (142) | PE1 | — |
| PE4 (3) | PE4 | — |
| PG3 (88) | PG3 | — |
| PG4 (89) | PG4 | — |
| PG5 (90) | PG5 | — |
| PG6 (91) | PG6 | — |
| PG7 (92) | PG7 | — |
| PG8 (93) | PG8 | — |
| PG10 (125) | PG10 | — |
| PG11 (126) | PG11 | — |
| PF10 (22) | PF10 | — |
| PF11 (49) | PF11 | — |

## 三、EXMC 总线（SDRAM/LCD，当前不编译）

| 功能 | 引脚 |
|------|------|
| 地址 A0~A12 | PF0~PF5, PF12~PF15, PG0~PG1, PG2 |
| 数据 D0~D15 | PD14, PD15, PD0, PD1, PE7~PE15, PD8~PD10 |
| FSMC_NOE | PD4 |
| FSMC_NWE | PD5 |
| FSMC_NE4 | PG12 |

## 四、DCMI 摄像头接口（当前不编译）

| 功能 | 引脚 |
|------|------|
| D0~D7 | PC6~PC9, PC11, PE5, PE6, PE4(缺失) |
| HREF | PA4 |
| PCLK | PA6 |
| XCLK | PA8 |
| SCL/SDA (SCCB) | PD6/PD7 |
| PWDN | PG9 |
| RESET | PG15 |

## 五、关键结论

| 外设 | 推荐引脚 | AF | 说明 |
|------|----------|-----|------|
| **CAN0** | PB8 (RX), PB9 (TX) | AF9 | 唯一可用 CAN 引脚对 |
| **CAN1** | PA11 (RX), PA12 (TX) | AF9 | 次选 |
| **硬件 I2C** | PE2/PE3 (I2C1), PB6/PB7 (I2C) | AF4 | 已用软 I2C |
| **ADC** | PA0, PA1, PA5, PC0, PC3 | — | 多通道可用 |
| **DAC** | PA4, PA5 | — | PA4 与 DCMI 冲突 |
| **PWM** | PA6(T3CH1), PA8(T1CH1), PB0(T3CH3) | AF2 | 多通道 |
| **USART2** | PD5/PD6 被占用 | — | 需要改引脚 |







================================================================================
         GD32F470xx 复用功能 (Alternate Function) 完整分类索引
================================================================================

说明: 每个信号后标注的是 [引脚(AF编号)]，即配置该功能时需要写入的 AF 值。
      同一信号可用多个引脚时，选其一即可。


━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
 1. 定时器 (TIMER)
━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━

── TIMER0 ── 高级定时器 ────────────────────────────────────────────────────────

  CH0 (通道0输出):         PA8(AF1)   PE9(AF1)
  CH1 (通道1输出):         PA9(AF1)   PE11(AF1)
  CH2 (通道2输出):         PA10(AF1)  PE13(AF1)
  CH3 (通道3输出):         PA11(AF1)  PE14(AF1)
  CH0_ON (通道0互补输出):  PA7(AF1)   PB13(AF1)  PE8(AF1)
  CH1_ON (通道1互补输出):  PB0(AF1)   PB14(AF1)  PE1(AF1)   PE10(AF1)
  CH2_ON (通道2互补输出):  PB1(AF1)   PB15(AF1)  PE12(AF1)
  ETI (外部触发输入):      PA12(AF1)  PE7(AF1)
  BRKIN (刹车输入):        PA6(AF1)   PB12(AF1)  PE15(AF1)


── TIMER1 ── 通用定时器 ────────────────────────────────────────────────────────

  CH0 / ETI:  PA0(AF1)   PA5(AF1)   PA15(AF1)  PB8(AF1)
  CH1:        PA1(AF1)   PB3(AF1)   PB9(AF1)
  CH2:        PA2(AF1)   PB10(AF1)
  CH3:        PA3(AF1)   PB2(AF1)   PB11(AF1)


── TIMER2 ── 通用定时器 ────────────────────────────────────────────────────────

  CH0:  PA6(AF2)   PB4(AF2)   PC6(AF2)
  CH1:  PA7(AF2)   PB5(AF2)   PC7(AF2)
  CH2:  PB0(AF2)   PC8(AF2)
  CH3:  PB1(AF2)   PC9(AF2)
  ETI:  PD2(AF2)


── TIMER3 ── 通用定时器 ────────────────────────────────────────────────────────

  CH0:  PB6(AF2)   PD12(AF2)
  CH1:  PB7(AF2)   PD13(AF2)
  CH2:  PB8(AF2)   PD14(AF2)
  CH3:  PB9(AF2)   PD15(AF2)
  ETI:  PE0(AF2)


── TIMER4 ── 通用定时器 ────────────────────────────────────────────────────────

  CH0:  PA0(AF2)   PH10(AF2)
  CH1:  PA1(AF2)   PH11(AF2)
  CH2:  PA2(AF2)   PH12(AF2)
  CH3:  PA3(AF2)   PI0(AF2)


── TIMER7 ── 通用定时器 ────────────────────────────────────────────────────────

  CH0 (通道0输出):         PC6(AF3)   PI5(AF3)
  CH1 (通道1输出):         PC7(AF3)   PI6(AF3)
  CH2 (通道2输出):         PC8(AF3)   PI7(AF3)
  CH3 (通道3输出):         PC9(AF3)   PI2(AF3)
  CH0_ON (通道0互补输出):  PA5(AF3)   PA7(AF3)   PH13(AF3)
  CH1_ON (通道1互补输出):  PB0(AF3)   PB14(AF3)  PH14(AF3)
  CH2_ON (通道2互补输出):  PB1(AF3)   PB15(AF3)  PH15(AF3)
  ETI (外部触发输入):      PA0(AF3)   PI3(AF3)
  BRKIN (刹车输入):        PA6(AF3)   PI4(AF3)


── TIMER8 ── 通用定时器 ────────────────────────────────────────────────────────

  CH0:  PA2(AF3)   PE5(AF3)
  CH1:  PA3(AF3)   PE6(AF3)


── TIMER9 ──────────────────────────────────────────────────────────────────────

  CH0:  PB8(AF3)   PF6(AF3)


── TIMER10 ─────────────────────────────────────────────────────────────────────

  CH0:  PB9(AF3)   PF7(AF3)


── TIMER11 ─────────────────────────────────────────────────────────────────────

  CH0:  PB14(AF9)  PH6(AF9)
  CH1:  PB15(AF9)  PH9(AF9)


── TIMER12 ─────────────────────────────────────────────────────────────────────

  CH0:  PA6(AF9)   PF8(AF9)


── TIMER13 ─────────────────────────────────────────────────────────────────────

  CH0:  PA7(AF9)   PF9(AF9)



━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
 2. SPI / I2S
━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━

── SPI0 ── 全部 AF5 ────────────────────────────────────────────────────────────

  NSS:   PA4(AF5)    PA15(AF5)
  SCK:   PA5(AF5)    PB3(AF5)
  MISO:  PA6(AF5)    PB4(AF5)
  MOSI:  PA7(AF5)    PB5(AF5)


── SPI1 / I2S1 ── 大部分 AF5，个别 AF7 ────────────────────────────────────────

  SPI1_SCK / I2S1_CK:
         PA9(AF5)    PB10(AF5)   PB13(AF5)   PC7(AF5)    PD3(AF5)    PI1(AF5)

  SPI1_NSS / I2S1_WS:
         PB9(AF5)    PB12(AF5)   PD1(AF7)    PI0(AF5)

  SPI1_MISO:
         PB14(AF5)   PC2(AF5)    PI2(AF5)

  SPI1_MOSI / I2S1_SD:
         PB15(AF5)   PC1(AF7)    PC3(AF5)    PI3(AF5)

  I2S1_MCK (主时钟输出):
         PA3(AF5)    PA6(AF6)    PC6(AF5)

  I2S1_ADD_SD (全双工附加数据):
         PB14(AF6)   PC2(AF6)    PI2(AF6)


── SPI2 / I2S2 ── 大部分 AF6 ──────────────────────────────────────────────────

  SPI2_NSS / I2S2_WS:
         PA4(AF6)    PA15(AF6)

  SPI2_SCK / I2S2_CK:
         PB3(AF6)    PC10(AF6)

  SPI2_MISO:
         PB4(AF6)    PC11(AF7)  ← 注意: PC11上为AF7

  SPI2_MOSI / I2S2_SD:
         PB0(AF6)    PB1(AF6)    PB2(AF6)    PB5(AF6)
         PD0(AF6)    PD6(AF5)    PC12(AF6)  ← 注意: PD6上为AF5

  I2S2_MCK (主时钟输出):
         PB10(AF6)   PC7(AF6)

  I2S2_ADD_SD (全双工附加数据):
         PC11(AF6)


── SPI3 ── 大部分 AF5，PB12/PB13为AF6 ─────────────────────────────────────────

  NSS:   PE4(AF5)    PE11(AF5)   PB12(AF6)   PG14(AF5)
  SCK:   PE2(AF5)    PB13(AF6)   PG11(AF5)
  MISO:  PE5(AF5)    PD0(AF5)    PG12(AF5)
  MOSI:  PA1(AF5)    PE6(AF5)    PG13(AF5)


── SPI4 ── AF5 / AF6 混合 ─────────────────────────────────────────────────────

  NSS:   PB1(AF5)    PE11(AF6)   PF6(AF5)    PH5(AF5)
  SCK:   PB0(AF5)    PE12(AF6)   PF7(AF5)    PH6(AF5)
  MISO:  PA12(AF6)   PE13(AF6)   PF8(AF5)    PH7(AF5)
  MOSI:  PA10(AF6)   PB8(AF6)    PE14(AF6)   PF9(AF5)    PF11(AF5)


── SPI5 ── 全部 AF4，仅 Port G ────────────────────────────────────────────────

  NSS:   PG8(AF4)
  SCK:   PG13(AF4)
  MISO:  PG12(AF4)
  MOSI:  PG14(AF4)
  IO2:   PG10(AF4)   (Quad SPI 模式双向IO2)
  IO3:   PG11(AF4)   (Quad SPI 模式双向IO3)


── I2S_CKIN ── I2S 外部时钟输入 ── 全部 AF5 ───────────────────────────────────

         PA2(AF5)    PB5(AF5)    PB11(AF5)   PC9(AF5)



━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
 3. USART / UART
━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━

── USART0 ── 全部 AF7 ──────────────────────────────────────────────────────────

  TX:   PA9(AF7)    PA15(AF7)   PB6(AF7)
  RX:   PA10(AF7)   PB2(AF7)    PB3(AF7)    PB7(AF7)
  CTS:  PA11(AF7)
  RTS:  PA12(AF7)
  CK:   PA8(AF7)


── USART1 ── 全部 AF7 ──────────────────────────────────────────────────────────

  TX:   PA2(AF7)    PD5(AF7)
  RX:   PA3(AF7)    PD6(AF7)
  CTS:  PA0(AF7)    PD3(AF7)
  RTS:  PA1(AF7)    PD4(AF7)
  CK:   PA4(AF7)    PD7(AF7)


── USART2 ── 主要 AF7，PC12 CK为AF8 ───────────────────────────────────────────

  TX:   PB10(AF7)   PC10(AF7)   PD8(AF7)
  RX:   PB11(AF7)   PC5(AF7)    PD9(AF7)
  CK:   PB12(AF7)   PC12(AF8)   PD10(AF7)   ← PC12上CK为AF8
  CTS:  PB13(AF7)   PD11(AF7)
  RTS:  PB14(AF7)   PD12(AF7)


── UART3 ── AF8/AF9 ────────────────────────────────────────────────────────────

  TX:   PA0(AF8)    PC10(AF8)
  RX:   PA1(AF8)    PC11(AF9)   ← PC11上RX为AF9


── UART4 ── AF8/AF9 ────────────────────────────────────────────────────────────

  TX:   PC12(AF9)
  RX:   PD2(AF8)


── USART5 ── Port A/C 为 AF8，Port G 为 AF7 ───────────────────────────────────

  TX:   PA11(AF8)   PC6(AF8)    PG14(AF7)
  RX:   PA12(AF8)   PC7(AF8)    PG9(AF7)
  CK:   PC8(AF8)    PG7(AF7)
  RTS:  PG8(AF7)    PG12(AF7)
  CTS:  PG13(AF7)   PG15(AF7)


── UART6 ── 全部 AF8 ───────────────────────────────────────────────────────────

  TX:   PE8(AF8)    PF7(AF8)
  RX:   PE7(AF8)    PF6(AF8)


── UART7 ── 全部 AF8，仅 Port E ───────────────────────────────────────────────

  TX:   PE1(AF8)
  RX:   PE0(AF8)



━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
 4. I2C
━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━

── I2C0 ── 全部 AF4 ────────────────────────────────────────────────────────────

  SCL:      PB6(AF4)    PB8(AF4)
  SDA:      PB7(AF4)    PB9(AF4)
  SMBA:     PB5(AF4)
  TXFRAME:  PB4(AF4)


── I2C1 ── 主要 AF4，PB2/PB3 SDA 为 AF9 ───────────────────────────────────────

  SCL:      PB10(AF4)   PF1(AF4)    PH4(AF4)
  SDA:      PB2(AF9)    PB3(AF9)    PB11(AF4)   PC12(AF4)
            PF0(AF4)    PH5(AF4)
            ← PB2/PB3上SDA为AF9
  SMBA:     PB12(AF4)   PF2(AF4)    PH6(AF4)
  TXFRAME:  PF3(AF4)    PH3(AF4)


── I2C2 ── AF4，PB4 SDA 为 AF9 ────────────────────────────────────────────────

  SCL:      PA8(AF4)    PH7(AF4)
  SDA:      PB4(AF9)    PC9(AF4)    PH8(AF4)
            ← PB4上SDA为AF9
  SMBA:     PA9(AF4)    PH9(AF4)
  TXFRAME:  PA10(AF4)   PH10(AF4)



━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
 5. CAN
━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━

── CAN0 ── 全部 AF9 ────────────────────────────────────────────────────────────

  RX:  PA11(AF9)   PB8(AF9)    PD0(AF9)    PI9(AF9)
  TX:  PA12(AF9)   PB9(AF9)    PD1(AF9)    PH13(AF9)


── CAN1 ── 全部 AF9 ────────────────────────────────────────────────────────────

  RX:  PB5(AF9)    PB12(AF9)
  TX:  PB6(AF9)    PB13(AF9)



━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
 6. USB
━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━

── USBFS (全速USB) ── AF10 ─────────────────────────────────────────────────────

  DM:   PA11(AF10)
  DP:   PA12(AF10)
  SOF:  PA8(AF10)
  ID:   PA10(AF10)


── USBHS (高速USB) ── ULPI 引脚 AF10，其他 AF12 ───────────────────────────────

  ── ULPI 接口 (AF10) ──
  ULPI_CK:   PA5(AF10)
  ULPI_STP:  PC0(AF10)
  ULPI_DIR:  PC1(AF10)    PI11(AF10)
  ULPI_NXT:  PC3(AF10)    PH4(AF10)
  ULPI_D0:   PA3(AF10)
  ULPI_D1:   PB0(AF10)
  ULPI_D2:   PB1(AF10)    PC2(AF10)
  ULPI_D3:   PB10(AF10)
  ULPI_D4:   PB2(AF10)    PB11(AF10)
  ULPI_D5:   PB12(AF10)
  ULPI_D6:   PB13(AF10)
  ULPI_D7:   PB5(AF10)

  ── 内部 PHY 引脚 (AF12) ──
  SOF:  PA4(AF12)
  ID:   PB12(AF12)
  DM:   PB14(AF12)
  DP:   PB15(AF12)



━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
 7. 以太网 (Ethernet)
━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
  全部 AF11

── 管理接口 ──

  MDIO:   PA2(AF11)
  MDC:    PC1(AF11)

── PPS ──

  PPS_OUT: PB5(AF11)   PG8(AF11)

── MII/RMII 接收 ──

  MII_CRS / (RMII未使用):        PA0(AF11)    PH2(AF11)
  MII_COL / (RMII未使用):        PA3(AF11)    PH3(AF11)
  MII_RX_CLK / RMII_REF_CLK:    PA1(AF11)
  MII_RX_DV / RMII_CRS_DV:      PA7(AF11)
  MII_RXD0 / RMII_RXD0:         PC4(AF11)
  MII_RXD1 / RMII_RXD1:         PC5(AF11)
  MII_RXD2 (仅MII):             PB0(AF11)    PH6(AF11)
  MII_RXD3 (仅MII):             PB1(AF11)    PH7(AF11)
  MII_RX_ER:                    PB10(AF11)   PI10(AF11)

── MII/RMII 发送 ──

  MII_TX_CLK (仅MII):                        PC3(AF11)
  MII_TX_EN / RMII_TX_EN:                     PB11(AF11)   PG11(AF11)
  MII_TXD0 / RMII_TXD0:                       PB12(AF11)   PG13(AF11)
  MII_TXD1 / RMII_TXD1:                       PB13(AF11)   PG14(AF11)
  RMII_TXD1 (备选):                            PB14(AF11)
  MII_TXD2 (仅MII):                           PC2(AF11)
  MII_TXD3 (仅MII):                           PB8(AF11)    PE2(AF11)



━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
 8. EXMC (外部存储控制器)
━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
  全部 AF12

── SDRAM 控制信号 ──

  SDCLK:   PG8
  SDNCAS:  PG15
  SDNRAS:  PF11
  SDNWE:   PA7     PC0     PH5
  SDNE0:   PC2     PC4     PH3     (片选0)
  SDCKE0:  PC3     PC5     PH2     (时钟使能0)
  SDNE1:   PB5     PH6     (片选1)
  SDCKE1:  PH7     (时钟使能1)

── SRAM/ NOR 控制信号 ──

  NOE:     PD4     (读使能)
  NWE:     PD5     (写使能)
  NWAIT:   PD6     (等待)
  CLK:     PD3
  NL:      PB7
  NE0/NCE1: PD7    (片选0)
  NE1/NCE2: PG9    (片选1)
  NE3:     PG12    (片选3)
  NBL0:    PE0     (字节使能低)
  NBL1:    PE1     (字节使能高)
  NBL2:    PI4
  NBL3:    PI5

── PC Card / CF 控制信号 ──

  NIORD:   PF6
  NREG:    PF7
  NIOWR:   PF8
  CD:      PF9
  INTR:    PF10
  INT1:    PG6
  INT2:    PG7

── NAND 控制信号 ──

  NCE3_0/NCE2: PG10
  NCE3_1:      PG11

── 地址总线 (A0-A25) ──

  A0:  PF0    A7:  PF13   A14: PG4    A21: PE5
  A1:  PF1    A8:  PF14   A15: PG5    A22: PE6
  A2:  PF2    A9:  PF15   A16: PD11   A23: PE2
  A3:  PF3    A10: PG0    A17: PD12   A24: PG13
  A4:  PF4    A11: PG1    A18: PD13   A25: PG14
  A5:  PF5    A12: PG2    A19: PE3
  A6:  PF12   A13: PG3    A20: PE4

── 数据总线 (D0-D31) ──

  D0:  PD14   D8:  PE11   D16: PH8    D24: PI0
  D1:  PD15   D9:  PE12   D17: PH9    D25: PI1
  D2:  PD0    D10: PE13   D18: PH10   D26: PI2
  D3:  PD1    D11: PE14   D19: PH11   D27: PI3
  D4:  PE7    D12: PE15   D20: PH12   D28: PI6
  D5:  PE8    D13: PD8    D21: PH13   D29: PI7
  D6:  PE9    D14: PD9    D22: PH14   D30: PI8
  D7:  PE10   D15: PD10   D23: PH15   D31: PI9, PI10



━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
 9. DCI (数字摄像头接口)
━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
  全部 AF13

  PIXCLK (像素时钟):  PA6
  HSYNC (行同步):      PA4      PH8
  VSYNC (场同步):      PB7      PG9     PI5

  D0:   PA9     PC6     PH9
  D1:   PA10    PC7     PH10
  D2:   PC8     PE0     PG10    PH11
  D3:   PC9     PE1     PG11    PH12
  D4:   PC11    PE4     PH14
  D5:   PB6     PD3     PI4
  D6:   PB8     PE5     PI6
  D7:   PB9     PE6     PI7
  D8:   PC10    PH6     PI1
  D9:   PC12    PH7     PI2
  D10:  PB5     PD6     PI3
  D11:  PD2     PF10    PH15
  D12:  PF11    PG6
  D13:  PG7     PG15



━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
10. TLI (TFT-LCD 显示接口)
━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
  大部分 AF14，PB0/PB1 为 AF9，PG10/PG12 有 AF8/AF9 复用

── 同步/时钟信号 ──

  HSYNC (行同步):     PC6(AF14)     PI10(AF14)
  VSYNC (场同步):     PA4(AF14)     PI9(AF14)
  DE (数据使能):      PE13(AF14)    PF10(AF14)
  PIXCLK (像素时钟):  PE14(AF14)    PG7(AF14)

── 红色通道 ──

  R0:  PH2(AF14)
  R1:  PH3(AF14)
  R2:  PC10(AF14)    PH8(AF14)
  R3:  PB0(AF9)      PH9(AF14)    ← PB0上为AF9
  R4:  PA11(AF14)    PH10(AF14)
  R5:  PA12(AF14)    PH11(AF14)
  R6:  PA8(AF14)     PB1(AF9)     PH12(AF14)  ← PB1上为AF9
  R7:  PE15(AF14)    PG6(AF14)

── 绿色通道 ──

  G0:  PE5(AF14)
  G1:  PE6(AF14)
  G2:  PA6(AF14)     PH13(AF14)
  G3:  PE11(AF14)    PG10(AF9)    PH14(AF14)  ← PG10上为AF9
  G4:  PB10(AF14)    PH15(AF14)
  G5:  PB11(AF14)    PI0(AF14)
  G6:  PC7(AF14)     PI1(AF14)
  G7:  PD3(AF14)     PI2(AF14)

── 蓝色通道 ──

  B0:  PE4(AF14)
  B1:  PG12(AF14)
  B2:  PD6(AF14)     PG10(AF14)
  B3:  PD10(AF14)    PG11(AF14)
  B4:  PE12(AF14)    PG12(AF8)    PI4(AF14)   ← PG12上为AF8
  B5:  PA3(AF14)     PI5(AF14)
  B6:  PB8(AF14)     PI6(AF14)
  B7:  PB9(AF14)     PI7(AF14)



━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
11. SDIO / SD/MMC 接口
━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
  全部 AF12

  CMD:  PA6      PD2
  CK:   PB2      PC12
  D0:   PB3      PC8
  D1:   PA8      PB0     PC9
  D2:   PA9      PB1     PC10
  D3:   PC11
  D4:   PB8
  D5:   PB9
  D6:   PC6
  D7:   PB10     PC7



━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
12. 其他功能
━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━

── JTAG / SWD 调试接口 ── AF0

  JTMS/SWDIO:    PA13(AF0)
  JTCK/SWCLK:    PA14(AF0)
  JTDI:          PA15(AF0)
  JTDO/TRACESWO: PB2(AF0)
  NJTRST:        PB3(AF0)

── 时钟输出 ── AF0

  CK_OUT0:  PA8(AF0)
  CK_OUT1:  PC9(AF0)

── CTC (时钟校准) ──

  CTC_SYNC:  PA8(AF9)    PD15(AF0)   PF0(AF0)

── RTC ── AF0

  RTC_REFEN:  PB15(AF0)

── EVENTOUT ── AF15

  所有引脚均支持，AF15 = EVENTOUT (用于内核信号输出到引脚)



━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
附录: AF编号速查 (哪些外设在哪个AF)
━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━

  AF0  → JTAG/SWD, CK_OUT, CTC, RTC
  AF1  → TIMER0, TIMER1
  AF2  → TIMER2, TIMER3, TIMER4
  AF3  → TIMER7, TIMER8, TIMER9, TIMER10
  AF4  → I2C0, I2C1, I2C2, SPI5
  AF5  → SPI0, SPI1/I2S1, SPI2/I2S2(部分), SPI3(大部分), SPI4(部分), I2S_CKIN
  AF6  → SPI2/I2S2(大部分), SPI3(PB12/PB13), SPI4(部分), I2S1_MCK, I2S1_ADD_SD, I2S2_MCK
  AF7  → USART0, USART1, USART2, SPI1(部分PC/PD引脚), SPI2_MISO(PC11)
  AF8  → UART3, UART4(RX), USART5(PA/PC), UART6, UART7, USART2_CK(PC12), TLI_B4(PG12)
  AF9  → TIMER11, TIMER12, TIMER13, CAN0, CAN1, I2C1_SDA(PB2/PB3), I2C2_SDA(PB4),
         CTC_SYNC(PA8), TLI_R3(PB0), TLI_R6(PB1), TLI_G3(PG10), UART4_TX(PC12)
  AF10 → USBFS, USBHS ULPI
  AF11 → ENET (以太网)
  AF12 → EXMC, USBHS(内部PHY), SDIO
  AF13 → DCI (摄像头)
  AF14 → TLI (LCD)
  AF15 → EVENTOUT
