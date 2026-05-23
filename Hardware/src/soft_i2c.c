#include "../Hardware/inc/soft_i2c.h"



void Soft_W_SCL(I2C_Info *i2c, uint8_t BitValue)
{ // SCL write data
	gpio_bit_write(i2c->GPIOx, i2c->SCL, (bit_status)BitValue);
	delay_us(2);
}

void Soft_W_SDA(I2C_Info *i2c, uint8_t BitValue)
{ // SDA write data
	gpio_bit_write(i2c->GPIOx, i2c->SDA, (bit_status)BitValue);
	delay_us(2);
}

uint8_t Soft_R_SDA(I2C_Info *i2c)
{ // SDA read data
	uint8_t BitValue = gpio_input_bit_get(i2c->GPIOx, i2c->SDA);
	delay_us(2);
	return BitValue;
}

void Soft_I2C_Init(I2C_Info *i2c, uint32_t GPIOx, uint16_t SCL, uint16_t SDA, uint16_t I2C_Add)
{
	i2c->GPIOx = GPIOx;
	i2c->SCL = SCL;
	i2c->SDA = SDA;
	i2c->I2C_Add = I2C_Add;

	if (i2c->GPIOx==GPIOA)
	{
		rcu_periph_clock_enable(RCU_GPIOA); // Enable GPIOA clock
	}
	else if (i2c->GPIOx==GPIOB)
	{
		rcu_periph_clock_enable(RCU_GPIOB); // Enable GPIOB clock
	}
	else if (i2c->GPIOx==GPIOC)
	{
		rcu_periph_clock_enable(RCU_GPIOC); // Enable GPIOC clock
	}
	else if (i2c->GPIOx==GPIOD)
	{
		rcu_periph_clock_enable(RCU_GPIOD); // Enable GPIOD clock
	}
	else if (i2c->GPIOx==GPIOE)
	{
		rcu_periph_clock_enable(RCU_GPIOE); // Enable GPIOE clock
	}
	else if (i2c->GPIOx==GPIOF)
	{
		rcu_periph_clock_enable(RCU_GPIOF); // Enable GPIOF clock
	}
	else if (i2c->GPIOx==GPIOG)
	{
		rcu_periph_clock_enable(RCU_GPIOG); // Enable GPIOG clock
	}
	else if (i2c->GPIOx==GPIOH)
	{
		rcu_periph_clock_enable(RCU_GPIOH); // Enable GPIOH clock
	}
	else if (i2c->GPIOx==GPIOI)
	{
		rcu_periph_clock_enable(RCU_GPIOI); // Enable GPIOI clock
	}
	
	gpio_mode_set(i2c->GPIOx, GPIO_MODE_OUTPUT, GPIO_PUPD_PULLUP, i2c->SCL);
	gpio_mode_set(i2c->GPIOx, GPIO_MODE_OUTPUT, GPIO_PUPD_PULLUP, i2c->SDA);

	gpio_output_options_set(i2c->GPIOx, GPIO_OTYPE_OD, GPIO_OSPEED_50MHZ, i2c->SCL);
	gpio_output_options_set(i2c->GPIOx, GPIO_OTYPE_OD, GPIO_OSPEED_50MHZ, i2c->SDA);

	// 先释放SDA和SCL，确保总线空闲
	Soft_W_SCL(i2c, 1);
	Soft_W_SDA(i2c, 1);

	// 发送一个完整的起始-停止序列来清理总线状态
	Soft_I2C_Start(i2c);
	Soft_I2C_Stop(i2c);
}

void Soft_I2C_Start(I2C_Info *i2c)
{
	Soft_W_SDA(i2c, 1);
	Soft_W_SCL(i2c, 1);
	Soft_W_SDA(i2c, 0);
	Soft_W_SCL(i2c, 0);
}

void Soft_I2C_Stop(I2C_Info *i2c)
{
	Soft_W_SDA(i2c, 0);
	Soft_W_SCL(i2c, 1);
	Soft_W_SDA(i2c, 1);
}

void Soft_I2C_WriteByte(I2C_Info *i2c, uint8_t Byte)
{
	uint8_t i;
	for (i = 0; i < 8; i++)
	{
		Soft_W_SDA(i2c, Byte & (0x80 >> i));
		delay_us(2);
		Soft_W_SCL(i2c, 1);
		delay_us(2);
		Soft_W_SCL(i2c, 0);
		delay_us(2);
	}
}

uint8_t Soft_I2C_ReceiveByte(I2C_Info *i2c)
{
	uint8_t i, Byte = 0x00;
	Soft_W_SDA(i2c, 1);
	for (i = 0; i < 8; i++)
	{
		Soft_W_SCL(i2c, 1);
		if (Soft_R_SDA(i2c) == 1)
		{
			Byte |= (0x80 >> i);
		}
		Soft_W_SCL(i2c, 0);
	}
	return Byte;
}

void Soft_I2C_WriteAck(I2C_Info *i2c, uint8_t AckBit)
{
	Soft_W_SDA(i2c, AckBit);
	Soft_W_SCL(i2c, 1);
	Soft_W_SCL(i2c, 0);
}

uint8_t Soft_I2C_ReceiveAck(I2C_Info *i2c)
{ // receive ask
	uint8_t AckBit;
	Soft_W_SDA(i2c, 1);
	Soft_W_SCL(i2c, 1);
	AckBit = Soft_R_SDA(i2c);
	Soft_W_SCL(i2c, 0);
	return AckBit;
}

void Soft_I2C_WriteReg(I2C_Info *i2c,uint8_t RegAddress, uint8_t Data)
{
	Soft_I2C_Start(i2c);
	Soft_I2C_WriteByte(i2c, i2c->I2C_Add << 1);
	Soft_I2C_ReceiveAck(i2c);
	Soft_I2C_WriteByte(i2c, RegAddress);
	Soft_I2C_ReceiveAck(i2c);
	Soft_I2C_WriteByte(i2c, Data);
	Soft_I2C_ReceiveAck(i2c);
	Soft_I2C_Stop(i2c);
}

uint8_t Soft_I2C_ReadReg(I2C_Info *i2c,uint8_t RegAddress)
{
	uint8_t Data = 0;

	Soft_I2C_Start(i2c);
	Soft_I2C_WriteByte(i2c, i2c->I2C_Add << 1);
	Soft_I2C_ReceiveAck(i2c);
	Soft_I2C_WriteByte(i2c, RegAddress);
	Soft_I2C_ReceiveAck(i2c);
	Soft_I2C_Start(i2c);
	Soft_I2C_WriteByte(i2c, i2c->I2C_Add << 1 | 0x01); //|0x01将最低位置1，代表读操作
	Soft_I2C_ReceiveAck(i2c);
	Data = Soft_I2C_ReceiveByte(i2c);
	Soft_I2C_WriteAck(i2c, 1); // 发送NACK，表示读取结束
	Soft_I2C_Stop(i2c);
	return Data;
}
