#pragma once
#include "gd32f4xx.h"
#include "systick.h"
#include <stdint.h>
#include <string.h>
#include <stdio.h>
#include "../Hardware/inc/debug_uart.h"
#include "../Hardware/inc/bsp_led.h"
#include "../App/inc/command_parsing.h"
#include "../Hardware/inc/key.h"
#include "../Hardware/inc/eeprom.h"

extern void APP_PC_Task(void);
