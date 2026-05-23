#include "../Hardware/inc/eeprom.h"

I2C_Info eepron_info;

void eeprom_config(uint32_t GPIOx, uint16_t SCL, uint16_t SDA)
{
	Soft_I2C_Init(&eepron_info, GPIOx, SCL, SDA, EEPROM_ADDRESS_7bit_0);
}

uint8_t ee_Write_Byte(uint16_t addr, uint8_t data)
{
	Soft_I2C_WriteReg(&eepron_info, addr, data);
	delay_ms(2); // 等待写入完成，通常EEPROM需要一些时间来完成写操作
	// Soft_I2C_Start(&eepron_info);
	return Soft_I2C_ReceiveAck(&eepron_info); // 返回ACK位，0表示写入成功，1表示写入失败
}

uint8_t ee_Read_Byte(uint16_t addr)
{
	uint8_t data = 0;
	data = Soft_I2C_ReadReg(&eepron_info, addr);
	delay_ms(2); // 等待读取完成，通常EEPROM需要一些时间来完成读操作
	return data;
}

uint8_t ee_Write_Page_Byte(uint16_t page, uint8_t *data, uint16_t len)
{
	if (page >= EEPROM_SIZE / EEPROM_PAGE_SIZE) // 检查页面是否超出范围
	{
		return 1; // 返回1表示写入失败
	}
	else
	{
		uint16_t addr = page * EEPROM_PAGE_SIZE; // 计算页面起始地址
		if (len > EEPROM_PAGE_SIZE)				 // 检查写入长度是否超过页面大小
		{
			return 1; // 返回1表示写入失败
		}
		for (uint16_t i = 0; i < len; i++)
		{
			ee_Write_Byte(addr + i, data[i]);
		}
		return 0; // 返回0表示写入成功
	}
}

uint8_t ee_Read_Page_Byte(uint16_t page, uint8_t *data, uint16_t len)
{
	if (page >= EEPROM_SIZE / EEPROM_PAGE_SIZE) // 检查页面是否超出范围
	{
		return 1; // 返回1表示读取失败
	}
	else
	{
		uint16_t addr = page * EEPROM_PAGE_SIZE; // 计算页面起始地址
		if (len > EEPROM_PAGE_SIZE)				 // 检查读取长度是否超过页面大小
		{
			return 1; // 返回1表示读取失败
		}
		for (uint16_t i = 0; i < len; i++)
		{
			data[i] = ee_Read_Byte(addr + i); // 逐字节读取数据
		}
		return 0; // 返回0表示读取成功
	}
}

uint8_t ee_Read_Multi_Byte(uint16_t addr, uint8_t *data, uint16_t len)
{
	uint16_t i;
	if (len > EEPROM_SIZE - addr) // 检查读取长度是否超过EEPROM大小
	{
		return 0; // 返回0表示读取失败
	}
	for (i = 0; i < len; i++)
	{
		data[i] = ee_Read_Byte(addr + i); // 逐字节读取数据
	}
	return 1; // 返回1表示读取成功
}

uint8_t ee_Write_Multi_Byte(uint16_t addr, uint8_t *data, uint16_t len)
{
	uint8_t ack = 0;
	for (uint16_t i = 0; i < len; i++)
	{
		ack = ee_Write_Byte(addr + i, data[i]);
		if (ack != 0)
		{
			break;
		}
	}
	return ack;
}

// 编写一个测试读多字节的函数，256全内存读操作
void ee_Test_Read_Multi_Byte(void)
{
	uint8_t read_data[256];
	// 写入多字节
	for (size_t i = 0; i < 256; i++)
	{
		read_data[i % 256] = i % 256; // 写入0-255循环的数据
		ee_Write_Byte(i % 256, read_data[i % 256]);
	}
	// 读取多字节数据
	ee_Read_Multi_Byte(0, read_data, sizeof(read_data));

	// 打印读取的数据
	for (uint8_t i = 0; i < 256; i++)
	{
		printf("Data[%d]: 0x%02X\n", i, read_data[i]);
		delay_ms(50); // 延时100ms
	}
}

// 擦除EEPROM通常是将指定地址写入0xFF
void ee_erase_Byte(uint16_t addr)
{
	// EEPROM 擦除通常是写入 0xFF
	ee_Write_Byte(addr, 0xFF);
}

void ee_erase_Page(uint16_t page)
{
	uint16_t addr;
	for (addr = page * EEPROM_PAGE_SIZE; addr < (page + 1) * EEPROM_PAGE_SIZE; addr++)
	{
		ee_erase_Byte(addr); // 擦除每个字节
	}
}

void ee_erase_All(void)
{
	uint16_t addr;
	for (addr = 0; addr < EEPROM_SIZE; addr++)
	{
		ee_erase_Byte(addr); // 擦除每个字节
	}
}

uint8_t ee_Check_Device(void)
{
	uint8_t ack = 0;
	Soft_I2C_Start(&eepron_info);
	Soft_I2C_WriteByte(&eepron_info, (eepron_info.I2C_Add << 1) | 0); // 发送设备地址和写位
	ack = Soft_I2C_ReceiveAck(&eepron_info);
	Soft_I2C_Stop(&eepron_info);
	return ack; // 返回ACK位
}
