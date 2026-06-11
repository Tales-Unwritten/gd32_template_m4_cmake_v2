#pragma once
#include "GD32F4xx.h"
#include "systick.h"
#include "stdint.h"
#include "string.h"
#include <stdio.h>
#include "gd32f4xx_can.h"

/* select can */
// #define DEV_CAN0_USED


void can_config(void); // CAN配置函数声明
void can_transmit_data(uint8_t *data, uint8_t len);
void can_transmit_text(void);

#ifdef DEV_CAN0_USED
#define CANX CAN0
#else
#define CANX CAN1
#endif
