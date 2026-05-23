#include "../Hardware/inc/w25q128.h"



static void w25q128_gpio_init(void)
{
	rcu_periph_clock_enable(W25Qxx_RCU_GPIO);
	rcu_periph_clock_enable(W25Qxx_RCU_GPIO_CSS);

	// 配置 SPI 引脚 (CLK, MISO, MOSI)
	gpio_af_set(W25Qxx_PORT, W25Qxx_AF_SPI, CLOCK_PIN);
	gpio_af_set(W25Qxx_PORT, W25Qxx_AF_SPI, MISO_PIN);
	gpio_af_set(W25Qxx_PORT, W25Qxx_AF_SPI, MOSI_PIN);

	gpio_mode_set(W25Qxx_PORT, GPIO_MODE_AF, GPIO_PUPD_NONE, CLOCK_PIN);
	gpio_mode_set(W25Qxx_PORT, GPIO_MODE_AF, GPIO_PUPD_NONE, MISO_PIN);
	gpio_mode_set(W25Qxx_PORT, GPIO_MODE_AF, GPIO_PUPD_NONE, MOSI_PIN);

	gpio_output_options_set(W25Qxx_PORT, GPIO_OTYPE_PP, GPIO_OSPEED_50MHZ, CLOCK_PIN);
	gpio_output_options_set(W25Qxx_PORT, GPIO_OTYPE_PP, GPIO_OSPEED_50MHZ, MOSI_PIN);
	// MISO 为输入，不需要设置输出选项

	// 配置 CS 引脚
	gpio_mode_set(W25Qxx_CSS_PORT, GPIO_MODE_OUTPUT, GPIO_PUPD_NONE, CSS_PIN);
	gpio_output_options_set(W25Qxx_CSS_PORT, GPIO_OTYPE_PP, GPIO_OSPEED_50MHZ, CSS_PIN);

	w25qxx_css_disable(); // 确保CSS引脚初始状态为高电平
}

static void w25q128_spi_init(void)
{
	rcu_periph_clock_enable(W25Qxx_RCU_SPI);

	spi_parameter_struct spi_init_struct;
	spi_init_struct.trans_mode = SPI_TRANSMODE_FULLDUPLEX;
	spi_init_struct.device_mode = SPI_MASTER;
	spi_init_struct.frame_size = SPI_FRAMESIZE_8BIT;
	spi_init_struct.clock_polarity_phase = SPI_CK_PL_HIGH_PH_2EDGE;
	spi_init_struct.nss = SPI_NSS_SOFT;
	spi_init_struct.prescale = SPI_PSC_2;
	spi_init_struct.endian = SPI_ENDIAN_MSB;

	spi_init(W25Qxx_SPI, &spi_init_struct);
	spi_enable(W25Qxx_SPI);
}

void w25qxx_css_enable(void)
{
	gpio_bit_reset(W25Qxx_CSS_PORT, CSS_PIN);
}

void w25qxx_css_disable(void)
{
	gpio_bit_set(W25Qxx_CSS_PORT, CSS_PIN);
}

void w25qxx_wait_busy(void)
{
	uint8_t status;
	w25qxx_css_enable();
	spi_read_write_byte(Read_Status_Register1);
	do
	{
		status = spi_read_write_byte(0xFF);
	} while ((status & 0x01) != 0);
	w25qxx_css_disable();
}

void w25qxx_write_enable(void)
{
	w25qxx_css_enable();
	spi_read_write_byte(Write_Enable);
	w25qxx_css_disable();
}

uint8_t spi_read_write_byte(uint8_t dat)
{
	uint32_t timeout = 0;

	// 等待发送缓冲区为空
	while (RESET == spi_i2s_flag_get(W25Qxx_SPI, SPI_FLAG_TBE))
	{
		if (++timeout > SPI_TIMEOUT)
			return 0xFF;
	}

	spi_i2s_data_transmit(W25Qxx_SPI, dat);

	// 等待接收缓冲区非空
	timeout = 0;
	while (RESET == spi_i2s_flag_get(W25Qxx_SPI, SPI_FLAG_RBNE))
	{
		if (++timeout > SPI_TIMEOUT)
			return 0xFF;
	}

	return spi_i2s_data_receive(W25Qxx_SPI);
}

void w25qxx_erase_sector(uint32_t sector_num)
{
	uint32_t addr = sector_num * 4096;

	w25qxx_write_enable();
	w25qxx_wait_busy();

	w25qxx_css_enable();
	spi_read_write_byte(Sector_Erase);
	spi_read_write_byte((uint8_t)(addr >> 16));
	spi_read_write_byte((uint8_t)(addr >> 8));
	spi_read_write_byte((uint8_t)addr);
	w25qxx_css_disable();

	// 等待擦除完成
	w25qxx_wait_busy();
}

void w25qxx_write(uint8_t *buffer, uint32_t addr, uint16_t numbyte)
{
	uint32_t i = 0;
	uint32_t sector_num = addr / 4096;

	// 擦除扇区
	w25qxx_erase_sector(sector_num);
	w25qxx_write_enable();
	w25qxx_wait_busy();

	// 写入数据
	w25qxx_css_enable();
	spi_read_write_byte(Page_Program);
	spi_read_write_byte((uint8_t)(addr >> 16));
	spi_read_write_byte((uint8_t)(addr >> 8));
	spi_read_write_byte((uint8_t)addr);

	for (i = 0; i < numbyte; i++)
	{
		spi_read_write_byte(buffer[i]);
	}

	w25qxx_css_disable();
	w25qxx_wait_busy();
}

void w25qxx_read(uint8_t *buffer, uint32_t addr, uint16_t numbyte)
{
	uint16_t i;

	w25qxx_css_enable();
	spi_read_write_byte(Read_Data);
	spi_read_write_byte((uint8_t)(addr >> 16));
	spi_read_write_byte((uint8_t)(addr >> 8));
	spi_read_write_byte((uint8_t)addr);

	for (i = 0; i < numbyte; i++)
	{
		buffer[i] = spi_read_write_byte(0xFF);
	}

	w25qxx_css_disable();
}

uint32_t w25qxx_read_id(void)
{
	uint32_t id = 0;

	w25qxx_css_enable();
	spi_read_write_byte(Read_JEDEC_ID);
	id = spi_read_write_byte(0xFF) << 16;
	id |= spi_read_write_byte(0xFF) << 8;
	id |= spi_read_write_byte(0xFF);
	w25qxx_css_disable();

	return id;
}

void My_W25Q28_TEXT(void)
{
	uint8_t write_buffer[256];
	uint8_t read_buffer[256];
	uint32_t flash_id;
	uint32_t test_addr = 0x000000; // 测试地址
	uint16_t test_length = 32;
	uint16_t i;

	printf("=====================================\r\n");
	printf("W25Q128 测试程序开始\r\n");
	printf("=====================================\r\n");

	// 1. 初始化 SPI
	printf("1. 初始化 W25Q128 SPI...\r\n");
	w25qxx_spi_config();
	printf("   SPI 初始化完成\r\n");

	// 2. 读取 Flash ID
	printf("2. 读取 Flash ID...\r\n");
	flash_id = w25qxx_read_id();
	printf("   Flash ID: 0x%06X\r\n", (unsigned int)flash_id);

	if (flash_id == 0xEF7018)
	{
		printf("   ID 校验成功 - 检测到 W25Q128\r\n");
	}
	else
	{
		printf("   ID 校验失败 - 未检测到正确的 Flash\r\n");
		return;
	}

	// 3. 准备测试数据
	printf("3. 准备测试数据...\r\n");
	for (i = 0; i < test_length; i++)
	{
		write_buffer[i] = 0x55 + i; // 生成测试数据
	}
	write_buffer[test_length - 1] = '\0'; // 字符串结束符

	printf("   写入数据: ");
	for (i = 0; i < test_length - 1; i++)
	{
		printf("0x%02X ", write_buffer[i]);
	}
	printf("\r\n");

	// 4. 写入数据到 Flash
	printf("4. 写入数据到 Flash (地址: 0x%06X)...\r\n", (unsigned int)test_addr);
	w25qxx_write(write_buffer, test_addr, test_length);
	printf("   数据写入完成\r\n");

	// 5. 从 Flash 读取数据
	printf("5. 从 Flash 读取数据 (地址: 0x%06X)...\r\n", (unsigned int)test_addr);
	memset(read_buffer, 0, sizeof(read_buffer));
	w25qxx_read(read_buffer, test_addr, test_length);

	printf("   读取数据: ");
	for (i = 0; i < test_length - 1; i++)
	{
		printf("0x%02X ", read_buffer[i]);
	}
	printf("\r\n");

	// 6. 验证数据
	printf("6. 验证数据...\r\n");
	if (memcmp(write_buffer, read_buffer, test_length) == 0)
	{
		printf("   数据验证成功 - 读写功能正常\r\n");
		printf("   读取的字符串: %s\r\n", (char *)read_buffer);
	}
	else
	{
		printf("   数据验证失败 - 读写功能异常\r\n");
		printf("   写入字符串: %s\r\n", (char *)write_buffer);
		printf("   读取字符串: %s\r\n", (char *)read_buffer);
	}

	// 7. 测试不同地址的读写
	printf("7. 测试不同地址读写...\r\n");
	test_addr = 0x1000; // 测试另一个地址
	char test_string[] = "Hello W25Q128!";
	strcpy((char *)write_buffer, test_string);
	test_length = strlen(test_string) + 1;

	printf("   在地址 0x%06X 写入字符串: %s\r\n", (unsigned int)test_addr, test_string);
	w25qxx_write(write_buffer, test_addr, test_length);

	memset(read_buffer, 0, sizeof(read_buffer));
	w25qxx_read(read_buffer, test_addr, test_length);
	printf("   从地址 0x%06X 读取字符串: %s\r\n", (unsigned int)test_addr, (char *)read_buffer);

	if (strcmp((char *)write_buffer, (char *)read_buffer) == 0)
	{
		printf("   不同地址读写测试成功\r\n");
	}
	else
	{
		printf("   不同地址读写测试失败\r\n");
	}

	printf("=====================================\r\n");
	printf("W25Q128 测试程序结束\r\n");
	printf("=====================================\r\n");
}

void w25qxx_spi_config(void)
{
	w25q128_gpio_init();
	w25q128_spi_init();
}