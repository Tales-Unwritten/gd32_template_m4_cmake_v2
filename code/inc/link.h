#pragma once

#ifdef __cplusplus
#define register
extern "C" {
#endif

// === 基础层 ===
#include "gd32f4xx.h"          // 芯片基础定义，放最前面
#include "core_cm4.h"           // ARM Cortex-M4 内核
#include "system_gd32f4xx.h"   // 系统时钟配置

// === 外设驱动层 ===
#include "gd32f4xx_adc.h"
#include "gd32f4xx_can.h"
#include "gd32f4xx_crc.h"
#include "gd32f4xx_ctc.h"
#include "gd32f4xx_dac.h"
#include "gd32f4xx_dbg.h"
#include "gd32f4xx_dci.h"
#include "gd32f4xx_dma.h"
#include "gd32f4xx_enet.h"
#include "gd32f4xx_exmc.h"
#include "gd32f4xx_exti.h"
#include "gd32f4xx_fmc.h"
#include "gd32f4xx_fwdgt.h"
#include "gd32f4xx_gpio.h"
#include "gd32f4xx_i2c.h"
#include "gd32f4xx_ipa.h"
#include "gd32f4xx_iref.h"
#include "gd32f4xx_misc.h"
#include "gd32f4xx_pmu.h"
#include "gd32f4xx_rcu.h"
#include "gd32f4xx_rtc.h"
#include "gd32f4xx_sdio.h"
#include "gd32f4xx_spi.h"
#include "gd32f4xx_syscfg.h"
#include "gd32f4xx_timer.h"
#include "gd32f4xx_tli.h"
#include "gd32f4xx_trng.h"
#include "gd32f4xx_usart.h"
#include "gd32f4xx_wwdgt.h"

// === 中断和服务层 ===
#include "gd32f4xx_it.h"
#include "gd32f4xx_libopt.h"
#include "systick.h"

#ifdef __cplusplus
#undef register
}

// === C++ 专用驱动层 ===
#include "usart_async_port.hpp"

extern usart_async_port<256> debug_uart;
extern usart_async_port<256> rs485_uart;
extern usart_async_port<256> rs232_uart;

#endif
