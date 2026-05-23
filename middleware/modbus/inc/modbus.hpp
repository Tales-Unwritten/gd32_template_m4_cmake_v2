#pragma once
#ifdef __cplusplus

#include "modbus_crc.hpp"
#include "modbus_map.hpp"

#include <stdint.h>

#define MODBUS_SLAVE_ADDR 0x01

#define MODBUS_RX_BUF_SIZE 256
#define MODBUS_TX_BUF_SIZE 256

/* 发送函数指针类型 */
typedef void (*modbus_send_fn)(uint8_t *buf, uint16_t len);
typedef struct
{
    uint16_t rx_len;
    uint8_t  rx_flag;
    uint8_t  rx_buf[MODBUS_RX_BUF_SIZE];

    uint16_t tx_len;
    uint8_t  tx_busy;
    uint8_t  tx_buf[MODBUS_TX_BUF_SIZE];

    modbus_send_fn send; // 发送函数指针

} modbus_t;

typedef struct
{
    pc_receive_buffer_t *buf;
    modbus_send_fn       send;
} modbus_port_t;

extern modbus_t Modbus;

/* 只声明，不赋值 */
extern const modbus_port_t modbus_ports[];
extern const uint8_t       modbus_port_count;

void Modbus_Init(void);

void Modbus_Task(void);

void Modbus_FillData(pc_receive_buffer_t *buf, modbus_send_fn send_fn);

void Modbus_FrameProcess(void);

void Modbus_PortSend(uint8_t *buf, uint16_t len);

#endif