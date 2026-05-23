#pragma once

#include "gd32f4xx.h"
#include "systick.h"
#include "stdio.h"
#include "string.h"

#include "../Hardware/inc/bsp_led.h"
#include "../Hardware/inc/debug_uart.h"
#include "../Hardware/inc/bitband.h"
#define USARTx_USART_TX_RCU     RCU_GPIOB         // 串口TX的端口时钟
#define USARTx_USART_RX_RCU     RCU_GPIOB         // 串口RX的端口时钟
#define USARTx_USART_RCU        RCU_USART2        // 串口1的时钟

#define USARTx_USART_IRQ        USART2_IRQn       // 串口1的中断号

#define USARTx_USART_TX_PORT    GPIOB		      // 串口TX的端口
#define USARTx_USART_RX_PORT    GPIOB		      // 串口RX的端口
#define USARTx_USART_AF         GPIO_AF_7         // 串口1的复用功能
#define USARTx_USART_TX_PIN     GPIO_PIN_10       // 串口TX的引脚
#define USARTx_USART_RX_PIN     GPIO_PIN_11       // 串口RX的引脚


#define USARTx_EN_PIN           GPIO_PIN_15       // USARTx使能引脚
#define USARTx_EN_PORT          GPIOE             // USARTx使能端口
#define USARTx_USART_EN_RCU     RCU_GPIOE         // USARTx使能端口时钟

#define USARTx_IRQHandler       USART2_IRQHandler // 串口中断函数名

#define USARTx_USART            USART2            // 串口2

#ifndef __PC_Receive_Buffer_t__
#define __PC_Receive_Buffer_t__
typedef struct
{
    uint8_t Buffer_Status; // 状态
    uint8_t Buffer_Length;  // 长度
    uint8_t Buffer[250];   // 缓存
} PC_Receive_Buffer_t;

#endif // !__PC_Receive_Buffer_t__


extern void usartx_config(uint32_t band_rate);                 // 初始化USARTx串口
extern void usartx_send_it_data(uint8_t *buffer, uint8_t length);   // 发送数据（中断方式）
extern void usartx_send_data(uint8_t *buffer, uint8_t length);      // 发送数据（轮询方式）

extern PC_Receive_Buffer_t USARTx_Receive_Buffer[CHCHE_COUNT];
