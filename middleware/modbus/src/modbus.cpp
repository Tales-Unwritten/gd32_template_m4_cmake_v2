/**
 * @file    modbus.cpp
 * @brief   Modbus RTU 从机协议栈 —— 纯协议核心
 *
 * 本文件不依赖任何平台/硬件头文件，仅实现协议层的帧解析和功能码处理。
 * 所有平台相关配置（端口表、发送回调）位于 modbus_port_cfg.cpp。
 *
 * 支持的功能码:
 *   0x03 - 读保持寄存器 (Read Holding Register)
 *   0x04 - 读输入寄存器  (Read Input Register)
 *   0x06 - 写单个寄存器  (Write Single Register)
 *   0x10 - 写多个寄存器  (Write Multiple Registers)
 */

#include "modbus.hpp"
#include "usart_port.hpp"   // uart_buffer_t 完整定义（Modbus_FillData 访问成员需要）

#include <cstring>           // memcpy, memset

/* ── 前向声明 ──────────────────────────────────────────── */

static void Modbus_ExceptionResponse(uint8_t func, uint8_t excode);

static void Modbus_Function03(void);
static void Modbus_Function04(void);
static void Modbus_Function06(void);
static void Modbus_Function16(void);

/* ── 全局实例 ──────────────────────────────────────────── */

modbus_t Modbus;

/* ── 初始化 ────────────────────────────────────────────── */

void Modbus_Init(void)
{
    memset(&Modbus, 0, sizeof(Modbus));
}

/* ── 发送接口 ──────────────────────────────────────────── */

void Modbus_PortSend(uint8_t *buf, uint16_t len)
{
    if (Modbus.send != NULL)
    {
        Modbus.send(buf, len);
    }
}

/* ── 异常响应 ──────────────────────────────────────────── */

/**
 * @brief  构建并发送 Modbus 异常响应帧
 *
 * 异常帧格式: [addr][func|0x80][excode][CRC-low][CRC-high] 共 5 字节
 */
static void Modbus_ExceptionResponse(uint8_t func, uint8_t excode)
{
    Modbus.tx_buf[0] = MODBUS_SLAVE_ADDR;
    Modbus.tx_buf[1] = func | 0x80;
    Modbus.tx_buf[2] = excode;

    uint16_t crc = Modbus_CRC16(Modbus.tx_buf, 3);
    Modbus.tx_buf[3] = crc & 0xFF;
    Modbus.tx_buf[4] = crc >> 8;

    Modbus_PortSend(Modbus.tx_buf, 5);
}

/* ── 数据填充 ──────────────────────────────────────────── */

void Modbus_FillData(uart_buffer_t *buf, modbus_send_fn send_fn)
{
    if (buf->rx_len == 0)
        return;

    Modbus.rx_len = buf->rx_len;
    if (Modbus.rx_len > MODBUS_RX_BUF_SIZE)
        Modbus.rx_len = MODBUS_RX_BUF_SIZE;

    memcpy(Modbus.rx_buf, buf->rx_buf, Modbus.rx_len);

    Modbus.send    = send_fn; // 记住来源端口，回复时原路返回
    Modbus.rx_flag = 1;

    buf->rx_len  = 0;
    buf->rx_flag = 0;
}

/* ── 帧解析 ────────────────────────────────────────────── */

/* 调试开关: 设为 1 可在 CRC/地址失败时回显收到的原始 hex */
#define MODBUS_DEBUG_ECHO 1

#if MODBUS_DEBUG_ECHO
static void Modbus_DebugEcho(void)
{
    if (Modbus.send == NULL) return;
    static const char hex[] = "0123456789ABCDEF";
    uint8_t buf[128];
    uint16_t pos = 0;
    buf[pos++] = '[';
    for (uint16_t i = 0; i < Modbus.rx_len && pos < 120; i++) {
        buf[pos++] = (uint8_t)hex[Modbus.rx_buf[i] >> 4];
        buf[pos++] = (uint8_t)hex[Modbus.rx_buf[i] & 0x0F];
    }
    buf[pos++] = ']';
    buf[pos++] = '\r';
    buf[pos++] = '\n';
    Modbus.send(buf, pos);
}
#endif

void Modbus_FrameProcess(void)
{
    if (Modbus.rx_len < 4)
    {
        Modbus.rx_len  = 0;
        Modbus.rx_flag = 0;
        return;
    }

    /* ── CRC 校验 ── */
    uint16_t crc_calc = Modbus_CRC16(Modbus.rx_buf, Modbus.rx_len - 2);
    uint16_t crc_recv = Modbus.rx_buf[Modbus.rx_len - 2]
                      | (Modbus.rx_buf[Modbus.rx_len - 1] << 8);

    if (crc_calc != crc_recv)
    {
#if MODBUS_DEBUG_ECHO
        Modbus_DebugEcho();  // 诊断: 回显 CRC 失败的帧
#endif
        Modbus.rx_len  = 0;
        Modbus.rx_flag = 0;
        return;
    }

    uint8_t addr = Modbus.rx_buf[0];

    if (addr == 0x00)
    {
        Modbus.rx_len  = 0;
        Modbus.rx_flag = 0;
        return;
    }

    if (addr != MODBUS_SLAVE_ADDR)
    {
#if MODBUS_DEBUG_ECHO
        Modbus_DebugEcho();  // 诊断: 回显地址不匹配的帧
#endif
        Modbus.rx_len  = 0;
        Modbus.rx_flag = 0;
        return;
    }

    /* ── 功能码分发 ── */
    uint8_t func = Modbus.rx_buf[1];

    switch (func)
    {
    case 0x03:
        Modbus_Function03();
        break;

    case 0x04:
        Modbus_Function04();
        break;

    case 0x06:
        Modbus_Function06();
        break;

    case 0x10:
        Modbus_Function16();
        break;

    default:
        Modbus_ExceptionResponse(func, MODBUS_EXC_ILLEGAL_FUNCTION);
        break;
    }

    Modbus.rx_len  = 0;
    Modbus.rx_flag = 0;
}

/* ── 功能码 0x03: 读保持寄存器 ─────────────────────────── */

static void Modbus_Function03(void)
{
    if (Modbus.rx_len < 8)
    {
        Modbus_ExceptionResponse(0x03, MODBUS_EXC_ILLEGAL_DATA_VALUE);
        return;
    }

    uint16_t start    = (Modbus.rx_buf[2] << 8) | Modbus.rx_buf[3];
    uint16_t quantity = (Modbus.rx_buf[4] << 8) | Modbus.rx_buf[5];

    if (quantity == 0 || quantity > MODBUS_MAX_REG_QUANTITY)
    {
        Modbus_ExceptionResponse(0x03, MODBUS_EXC_ILLEGAL_DATA_VALUE);
        return;
    }

    uint16_t data_bytes = quantity * 2;
    if (3 + data_bytes + 2 > MODBUS_TX_BUF_SIZE)
    {
        Modbus_ExceptionResponse(0x03, MODBUS_EXC_ILLEGAL_DATA_VALUE);
        return;
    }

    Modbus.tx_buf[0] = MODBUS_SLAVE_ADDR;
    Modbus.tx_buf[1] = 0x03;
    Modbus.tx_buf[2] = (uint8_t)data_bytes;

    for (uint16_t i = 0; i < quantity; i++)
    {
        uint16_t val = Modbus_ReadRegister(start + i, MODBUS_BLOCK_HOLDING);
        Modbus.tx_buf[3 + i * 2]     = (uint8_t)(val >> 8);
        Modbus.tx_buf[3 + i * 2 + 1] = (uint8_t)(val);
    }

    uint16_t len = 3 + data_bytes;
    uint16_t crc = Modbus_CRC16(Modbus.tx_buf, len);

    Modbus.tx_buf[len]     = (uint8_t)(crc & 0xFF);
    Modbus.tx_buf[len + 1] = (uint8_t)(crc >> 8);

    Modbus_PortSend(Modbus.tx_buf, len + 2);
}

/* ── 功能码 0x04: 读输入寄存器 ─────────────────────────── */

static void Modbus_Function04(void)
{
    if (Modbus.rx_len < 8)
    {
        Modbus_ExceptionResponse(0x04, MODBUS_EXC_ILLEGAL_DATA_VALUE);
        return;
    }

    uint16_t start    = (Modbus.rx_buf[2] << 8) | Modbus.rx_buf[3];
    uint16_t quantity = (Modbus.rx_buf[4] << 8) | Modbus.rx_buf[5];

    if (quantity == 0 || quantity > MODBUS_MAX_REG_QUANTITY)
    {
        Modbus_ExceptionResponse(0x04, MODBUS_EXC_ILLEGAL_DATA_VALUE);
        return;
    }

    uint16_t data_bytes = quantity * 2;
    if (3 + data_bytes + 2 > MODBUS_TX_BUF_SIZE)
    {
        Modbus_ExceptionResponse(0x04, MODBUS_EXC_ILLEGAL_DATA_VALUE);
        return;
    }

    Modbus.tx_buf[0] = MODBUS_SLAVE_ADDR;
    Modbus.tx_buf[1] = 0x04;
    Modbus.tx_buf[2] = (uint8_t)data_bytes;

    for (uint16_t i = 0; i < quantity; i++)
    {
        uint16_t val = Modbus_ReadRegister(start + i, MODBUS_BLOCK_INPUT);
        Modbus.tx_buf[3 + i * 2]     = (uint8_t)(val >> 8);
        Modbus.tx_buf[3 + i * 2 + 1] = (uint8_t)(val);
    }

    uint16_t len = 3 + data_bytes;
    uint16_t crc = Modbus_CRC16(Modbus.tx_buf, len);

    Modbus.tx_buf[len]     = (uint8_t)(crc & 0xFF);
    Modbus.tx_buf[len + 1] = (uint8_t)(crc >> 8);

    Modbus_PortSend(Modbus.tx_buf, len + 2);
}

/* ── 功能码 0x06: 写单个寄存器 ─────────────────────────── */

static void Modbus_Function06(void)
{
    if (Modbus.rx_len < 8)
    {
        Modbus_ExceptionResponse(0x06, MODBUS_EXC_ILLEGAL_DATA_VALUE);
        return;
    }

    uint16_t addr  = (Modbus.rx_buf[2] << 8) | Modbus.rx_buf[3];
    uint16_t value = (Modbus.rx_buf[4] << 8) | Modbus.rx_buf[5];

    Modbus_WriteRegister(addr, value);

    memcpy(Modbus.tx_buf, Modbus.rx_buf, 6);

    uint16_t crc = Modbus_CRC16(Modbus.tx_buf, 6);
    Modbus.tx_buf[6] = (uint8_t)(crc & 0xFF);
    Modbus.tx_buf[7] = (uint8_t)(crc >> 8);

    Modbus_PortSend(Modbus.tx_buf, 8);
}

/* ── 功能码 0x10: 写多个寄存器 ─────────────────────────── */

static void Modbus_Function16(void)
{
    if (Modbus.rx_len < 9)
    {
        Modbus_ExceptionResponse(0x10, MODBUS_EXC_ILLEGAL_DATA_VALUE);
        return;
    }

    uint16_t start    = (Modbus.rx_buf[2] << 8) | Modbus.rx_buf[3];
    uint16_t quantity = (Modbus.rx_buf[4] << 8) | Modbus.rx_buf[5];
    uint8_t  byte_cnt = Modbus.rx_buf[6];

    if (quantity == 0 || quantity > MODBUS_MAX_REG_QUANTITY)
    {
        Modbus_ExceptionResponse(0x10, MODBUS_EXC_ILLEGAL_DATA_VALUE);
        return;
    }

    if (byte_cnt != (uint8_t)(quantity * 2))
    {
        Modbus_ExceptionResponse(0x10, MODBUS_EXC_ILLEGAL_DATA_VALUE);
        return;
    }

    if (Modbus.rx_len < (uint16_t)(7 + byte_cnt + 2))
    {
        Modbus_ExceptionResponse(0x10, MODBUS_EXC_ILLEGAL_DATA_VALUE);
        return;
    }

    for (uint16_t i = 0; i < quantity; i++)
    {
        uint16_t value = ((uint16_t)Modbus.rx_buf[7 + i * 2] << 8)
                       |  (uint16_t)Modbus.rx_buf[8 + i * 2];

        Modbus_WriteRegister(start + i, value);
    }

    memcpy(Modbus.tx_buf, Modbus.rx_buf, 6);

    uint16_t crc = Modbus_CRC16(Modbus.tx_buf, 6);
    Modbus.tx_buf[6] = (uint8_t)(crc & 0xFF);
    Modbus.tx_buf[7] = (uint8_t)(crc >> 8);

    Modbus_PortSend(Modbus.tx_buf, 8);
}

/* ── 主任务 ────────────────────────────────────────────── */

/**
 * @brief  Modbus 轮询任务，应在主循环中周期性调用
 *
 * 遍历 modbus_port_cfg.cpp 中注册的所有通信端口，
 * 发现数据就绪则立即处理该端口的一帧数据。
 */
void Modbus_Task(void)
{
    for (uint8_t i = 0; i < modbus_port_count; i++)
    {
        if (modbus_ports[i].buf->rx_flag == 1)
        {
            Modbus_FillData(modbus_ports[i].buf, modbus_ports[i].send);
            Modbus_FrameProcess();
        }
    }
}
