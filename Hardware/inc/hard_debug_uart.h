#pragma once

#include "gd32f4xx.h"
#include "systick.h"
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "hard_device_led.h"

#define DEBUG_BAUDRATE                    115200U    /*!< 默认通信波特率 */



#define CHCHE_COUNT 8

#define DEBUG_USART_TX_RCU       RCU_GPIOA    // 串口TX的端口时钟
#define DEBUG_USART_RX_RCU       RCU_GPIOA    // 串口RX的端口时钟
#define DEBUG_USART_RCU          RCU_USART0	  // 串口0的时钟

#define DEBUG_USART_TX_PORT      GPIOA	      // 串口TX的端口
#define DEBUG_USART_RX_PORT      GPIOA	      // 串口RX的端口
#define DEBUG_USART_AF           GPIO_AF_7	  // 串口0的复用功能
#define DEBUG_USART_TX_PIN       GPIO_PIN_9   // 串口TX的引脚
#define DEBUG_USART_RX_PIN       GPIO_PIN_10  // 串口RX的引脚

#define DEBUG_USART              USART0	      // 串口0
#define DEBUG_USART_IRQ          USART0_IRQn  // 串口0的中断号

#define DEBUG_IRQHandler         USART0_IRQHandler

#ifndef __PC_Receive_Buffer_t__
#define __PC_Receive_Buffer_t__

/**
 * @brief Structure representing the PC transmit buffer.
 *
 * This structure is used to manage the data buffer for PC transmission,
 * including its status, length, and the actual data buffer.
 */
typedef struct
{
	uint8_t Buffer_Status;  /** Status of the buffer.*/
	uint8_t Buffer_Length;  /** Length of the data currently in the buffer. */
	uint8_t Buffer[250];    /** Data buffer with a maximum capacity of 250 bytes. */
} PC_Receive_Buffer_t;

#endif // !__PC_Receive_Buffer_t__



extern void debug_init_config(uint32_t baudrate);				  // 配置串口
extern void debug_send_it_data(uint8_t *buffer, uint8_t length); // 发送数据
extern void debug_send_data(uint8_t *buffer, uint8_t length);	  // 发送数据

extern uint32_t DEBUG_USART_BAUDRATE;
extern PC_Receive_Buffer_t Debug_Receive_Buffer[CHCHE_COUNT];
