#pragma once

#include "gd32f4xx.h"
#include "systick.h"
#include <stdint.h>
#include <stdio.h>
#include <string.h>

typedef enum
{
	Write_Cmd = 0,
	Read_Cmd = 1
} Read_Write_Cmd;

typedef struct
{
	uint32_t GPIOx;
	uint16_t SCL;
	uint16_t SDA;
	uint16_t I2C_Add;
} I2C_Info;

void Soft_I2C_Init(I2C_Info *i2c,uint32_t GPIOx, uint16_t SCL, uint16_t SDA, uint16_t I2C_Add);
void Soft_I2C_WriteReg(I2C_Info *i2c,uint8_t RegAddress, uint8_t Data);
uint8_t Soft_I2C_ReadReg(I2C_Info *i2c,uint8_t RegAddress); // Read register data

///////////////////////Administrator///////////////////////////
void Soft_W_SCL(I2C_Info *i2c,uint8_t BitValue);
void Soft_W_SDA(I2C_Info *i2c,uint8_t BitValue);
uint8_t Soft_R_SDA(I2C_Info *i2c); // Read SDA data

void Soft_I2C_Start(I2C_Info *i2c);
void Soft_I2C_Stop(I2C_Info *i2c);
void Soft_I2C_WriteByte(I2C_Info *i2c,uint8_t Byte);
uint8_t Soft_I2C_ReceiveByte(I2C_Info *i2c);
void Soft_I2C_WriteAck(I2C_Info *i2c,uint8_t AckBit);
uint8_t Soft_I2C_ReceiveAck(I2C_Info *i2c);
