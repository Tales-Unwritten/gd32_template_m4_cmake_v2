#include "hard_debug_uart.h"

uint32_t DEBUG_USART_BAUDRATE = DEBUG_BAUDRATE;
PC_Receive_Buffer_t Debug_Receive_Buffer[CHCHE_COUNT];

static struct
{
	volatile FlagStatus Finish_Flag;
	uint8_t Buffer_Length;
	uint8_t Buffer[sizeof(Debug_Receive_Buffer[0].Buffer)];
} Debug_IT_Secd_Buffer;

static void debug_recv_config(void)
{
	nvic_irq_enable(DEBUG_USART_IRQ, 0, 0);	/* 使能串口接收和空闲中断 */
	usart_interrupt_enable(DEBUG_USART, USART_INT_RBNE);
	usart_interrupt_enable(DEBUG_USART, USART_INT_IDLE);
	usart_receive_config(DEBUG_USART, USART_RECEIVE_ENABLE);
}

void debug_init_config(uint32_t baudrate)
{
	/* 开启时钟 */
	Debug_IT_Secd_Buffer.Finish_Flag = SET; // 初始标志位置位，表示空闲
	Debug_IT_Secd_Buffer.Buffer_Length = 0;
	memset(Debug_IT_Secd_Buffer.Buffer, 0, sizeof(Debug_IT_Secd_Buffer.Buffer));
	rcu_periph_clock_enable(DEBUG_USART_TX_RCU); // 开启串口时钟
	rcu_periph_clock_enable(DEBUG_USART_RX_RCU); // 开启端口时钟
	rcu_periph_clock_enable(DEBUG_USART_RCU);	 // 开启端口时钟

	/* 配置GPIO复用功能 */
	gpio_af_set(DEBUG_USART_TX_PORT, DEBUG_USART_AF, DEBUG_USART_TX_PIN);
	gpio_af_set(DEBUG_USART_RX_PORT, DEBUG_USART_AF, DEBUG_USART_RX_PIN);

	/* 配置GPIO的模式 */
	/* 配置TX为复用模式 上拉模式 */
	gpio_mode_set(DEBUG_USART_TX_PORT, GPIO_MODE_AF, GPIO_PUPD_PULLUP, DEBUG_USART_TX_PIN);
	/* 配置RX为复用模式 上拉模式 */
	gpio_mode_set(DEBUG_USART_RX_PORT, GPIO_MODE_AF, GPIO_PUPD_PULLUP, DEBUG_USART_RX_PIN);

	/* 配置TX为推挽输出 50MHZ */
	gpio_output_options_set(DEBUG_USART_TX_PORT, GPIO_OTYPE_PP, GPIO_OSPEED_50MHZ, DEBUG_USART_TX_PIN);
	/* 配置RX为推挽输出 50MHZ */
	gpio_output_options_set(DEBUG_USART_RX_PORT, GPIO_OTYPE_PP, GPIO_OSPEED_50MHZ, DEBUG_USART_RX_PIN);

	/* 配置串口的参数 */
	usart_deinit(DEBUG_USART);						   // 复位串口
	usart_baudrate_set(DEBUG_USART, baudrate);		   // 设置波特率
	usart_parity_config(DEBUG_USART, USART_PM_NONE);   // 没有校验位
	usart_word_length_set(DEBUG_USART, USART_WL_8BIT); // 8位数据位
	usart_stop_bit_set(DEBUG_USART, USART_STB_1BIT);   // 1位停止位
    debug_recv_config();							   // 配置接收中断
	/* 使能串口 */
	usart_enable(DEBUG_USART);								   // 使能串口
	usart_transmit_config(DEBUG_USART, USART_TRANSMIT_ENABLE); // 使能串口发送
}

int fputc(int ch, FILE *f)
{
	/* 等待发送缓冲区空 */
	while (RESET == usart_flag_get(DEBUG_USART, USART_FLAG_TBE));
	/* 发送字符 */
	usart_data_transmit(DEBUG_USART, (uint32_t)ch);
	/* 等待发送完成 */
	while (RESET == usart_flag_get(DEBUG_USART, USART_FLAG_TC));
	return ch;
}

void debug_send_it_data(uint8_t *buffer, uint8_t length)
{
	while (Debug_IT_Secd_Buffer.Finish_Flag == RESET)
	{
		delay_ms(1); // 等待上次发送完成
	}
	if (length > sizeof(Debug_IT_Secd_Buffer.Buffer))
	{
		length = sizeof(Debug_IT_Secd_Buffer.Buffer); // 限制长度
	}
	Debug_IT_Secd_Buffer.Finish_Flag = RESET;			 // 重置完成标志
	Debug_IT_Secd_Buffer.Buffer_Length = length;		 // 设置缓冲区长度
	memcpy(Debug_IT_Secd_Buffer.Buffer, buffer, length); // 复制数据到缓冲区
	usart_interrupt_enable(DEBUG_USART, USART_INT_TBE);	 // 使能发送中断
	usart_interrupt_enable(DEBUG_USART, USART_INT_TC);	 // 使能发送完成中断
}

void debug_send_data(uint8_t *buffer, uint8_t length)
{
	uint32_t tx_count;
	usart_interrupt_disable(DEBUG_USART, USART_INT_TBE); // 禁止发送中断
	usart_interrupt_disable(DEBUG_USART, USART_INT_TC);	 // 禁止发送完成中断
	for (tx_count = 0; tx_count < length; tx_count++)
	{
		usart_data_transmit(DEBUG_USART, buffer[tx_count]);
		while (RESET == usart_flag_get(DEBUG_USART, USART_FLAG_TBE))
			; // 等待发送数据缓冲区标志置位
	}
	while (RESET == usart_flag_get(DEBUG_USART, USART_FLAG_TC))
		;
}

void USART0_IRQHandler(void)
{
	static uint8_t Temp_Recevice_Buffer[sizeof(Debug_Receive_Buffer[0].Buffer)];
	static uint16_t Temp_Recevice_Count = 0;
	static uint8_t Send_Count = 0;
	/* 处理错误中断 - 优先处理错误 */
	if (usart_interrupt_flag_get(DEBUG_USART, USART_INT_FLAG_ERR_ORERR) ||
		usart_interrupt_flag_get(DEBUG_USART, USART_INT_FLAG_ERR_FERR) ||
		usart_interrupt_flag_get(DEBUG_USART, USART_INT_FLAG_ERR_NERR) ||
		usart_interrupt_flag_get(DEBUG_USART, USART_INT_FLAG_PERR))
	{
		/* 清除所有错误标志 */
		usart_flag_clear(DEBUG_USART, USART_FLAG_ORERR | USART_FLAG_FERR |USART_FLAG_NERR | USART_FLAG_PERR);
		usart_data_receive(DEBUG_USART); // 读取数据寄存器清除错误标志
		/* 错误发生时重置接收状态 */
		memset(Temp_Recevice_Buffer, 0, sizeof(Temp_Recevice_Buffer));
		Temp_Recevice_Count = 0;
		return; // 错误处理后直接返回
	}
	/* 接收缓冲区非空中断 */
	if (usart_interrupt_flag_get(DEBUG_USART, USART_INT_FLAG_RBNE) == SET)
	{

		if (Temp_Recevice_Count < (sizeof(Temp_Recevice_Buffer)))
		{
			Temp_Recevice_Buffer[Temp_Recevice_Count++] = usart_data_receive(DEBUG_USART);
		}
		else
		{
			usart_data_receive(DEBUG_USART); // 丢弃数据
		}
	}
	/* 空闲中断 - 接收完成 */
	else if (usart_interrupt_flag_get(DEBUG_USART, USART_INT_FLAG_IDLE) == SET)
	{
		usart_data_receive(DEBUG_USART); // 清除空闲标志
		/* 查找空闲缓冲区并存储数据 */
		for (size_t i = 0; i < CHCHE_COUNT; i++)
		{
			if (Debug_Receive_Buffer[i].Buffer_Status == 0)
			{
				Debug_Receive_Buffer[i].Buffer_Length = Temp_Recevice_Count;
				memcpy(Debug_Receive_Buffer[i].Buffer, Temp_Recevice_Buffer, Temp_Recevice_Count);
				Debug_Receive_Buffer[i].Buffer_Status = 1; // 标记为已接收
				break;
			}
		}
		memset(Temp_Recevice_Buffer, 0, sizeof(Temp_Recevice_Buffer));   /* 清空临时接收缓冲区 */
		Temp_Recevice_Count = 0;
	}
	/* 发送缓冲区空中断 */
	if (usart_interrupt_flag_get(DEBUG_USART, USART_INT_FLAG_TBE) == SET)
	{
		if (Debug_IT_Secd_Buffer.Finish_Flag == RESET && Send_Count < Debug_IT_Secd_Buffer.Buffer_Length)
		{
			usart_data_transmit(DEBUG_USART, Debug_IT_Secd_Buffer.Buffer[Send_Count++]);
		}
		else
		{
			usart_interrupt_disable(DEBUG_USART, USART_INT_TBE); // 禁止发送中断
		}
	}
	/* 发送完成中断 */
	if (usart_interrupt_flag_get(DEBUG_USART, USART_INT_FLAG_TC) == SET)
	{
		usart_interrupt_disable(DEBUG_USART, USART_INT_TC); // 禁止发送完成中断
		Debug_IT_Secd_Buffer.Finish_Flag = SET;				// 设置完成标志
		Send_Count = 0;										// 重置发送计数器
	}
}
