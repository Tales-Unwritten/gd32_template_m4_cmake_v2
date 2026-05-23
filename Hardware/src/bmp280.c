#include "../Hardware/inc/bmp280.h"

I2C_Info BMP280_i2C;

void BMP280_Init(void)
{
	Soft_I2C_Init(&BMP280_i2C, GPIOB, GPIO_PIN_6, GPIO_PIN_7, BMP280_ADDRESS); // Initialize I2C for BMP280

}

uint8_t BMP280_GetChipID(void)
{
	uint8_t chip_id = Soft_I2C_ReadReg(&BMP280_i2C, 0xD0); 
	return chip_id; // Return the chip ID

}

