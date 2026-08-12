#pragma once
#include "gd32f4xx.h"
#include "systick.h"
#include "stdio.h"
#include "string.h"
#include "hard_soft_i2c.h"

#define BMP280_ADDRESS 0x76

void BMP280_Init(void);

void BMP280_Test(void);
