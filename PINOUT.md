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
| PC1 (27) | OLED_SCL | I2C SCL | OLED 显示屏 |
| PC2 (28) | OLED_SDA | I2C SDA | OLED 显示屏 |
| PC5 (45) | LED1 | LED | 低电平亮 |
| PC4 (44) | LED2 | LED | 低电平亮 |
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
