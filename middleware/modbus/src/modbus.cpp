/**
 * @file    modbus.cpp
 * @brief   Modbus RTU 从机协议栈
 *
 * 支持功能码: 0x03（读保持寄存器）、0x04（读输入寄存器）、
 *            0x06（写单个寄存器）、0x10（写多个寄存器）
 *
 * 安全设计:
 *   - 所有功能码入口均有帧长校验，防止越界读
 *   - 寄存器数量上限 MODBUS_MAX_REG_QUANTITY=125，防止越界写
 *   - rx_len < 4 提前拦截，防止 CRC16 参数回绕
 *   - FillData 提前返回时清除 buf->rx_flag，防止死循环重入
 *   - FrameProcess 结束后同时清除 rx_len 和 rx_flag
 */

#include "modbus.hpp"
#include <cstring>

/**
01  Read Coil                        // 读取线圈状态
02  Read Discrete Input              // 读取离散输入状态
03  Read Holding Register            // 读取保持寄存器
04  Read Input Register              // 读取输入寄存器
05  Write Single Coil                // 写单个线圈
06  Write Single Register            // 写单个寄存器
07  Read Exception Status            // 读取异常状态
08  Diagnostics                      // 诊断
10  Write Multiple Registers         // 写多个寄存器
11  Report Slave ID                  // 报告从机标识
15  Write Multiple Coils             // 写多个线圈
16  Mask Write Register              // 屏蔽写寄存器
17  Read/Write Multiple Registers    // 读/写多个寄存器
20  Read File Record                 // 读取文件记录
21  Write File Record                // 写入文件记录
22  Mask Write Register              // 屏蔽写寄存器
23  Read/Write Multiple Registers    // 读/写多个寄存器
24  Read FIFO Queue                  // 读取 FIFO 队列
*/


static void Modbus_Function03(void);
static void Modbus_Function04(void);
static void Modbus_Function06(void);
static void Modbus_Function16(void);

modbus_t Modbus;

static void Debug_Send_IT(uint8_t *b, uint16_t l) { debug_uart.send_data_it(b, l); }

const modbus_port_t modbus_ports[] = {
    { debug_uart.buffer(), Debug_Send_IT },
    // { &pc_uart2_buffer,   UART2_Send },
    // { &pc_uart3_buffer,   UART3_Send },
};

const uint8_t modbus_port_count = sizeof(modbus_ports) / sizeof(modbus_ports[0]);

#define MODBUS_PORT_COUNT  (sizeof(modbus_ports) / sizeof(modbus_ports[0]))

void Modbus_Init(void)
{
    memset(&Modbus, 0, sizeof(Modbus)); // 初始化 Modbus 结构体，将其所有成员清零
    Modbus.rx_buf  = Modbus.rx_buf_arr; // rx_buf 指针指向实际存储数组
    Modbus.tx_data = Modbus.tx_buf_arr; // tx_data 指针指向实际存储数组
}

void Modbus_PortSend(uint8_t *buf, uint16_t len)
{
    if(Modbus.send != NULL)
    {
        Modbus.send(buf, len);
    }
}

void Modbus_FillData(uart_buffer_t *buf, modbus_send_fn send_fn)
{
    if(buf->rx_len == 0)
    {
        buf->rx_flag = 0; // 【安全】清除标志位，防止 Modbus_Task 死循环重入
        return;
    }

    Modbus.rx_len = buf->rx_len;
    if(Modbus.rx_len > MODBUS_RX_BUF_SIZE)
        Modbus.rx_len = MODBUS_RX_BUF_SIZE;

    memcpy(Modbus.rx_buf_arr, buf->rx_buf, Modbus.rx_len);

    Modbus.send = send_fn;   // 记住从哪个口来的，回哪个口
    Modbus.rx_flag = 1;

    buf->rx_len  = 0;
    buf->rx_flag = 0;
}

void Modbus_FrameProcess(void) /** 帧解析 */
{

    uint16_t crc_calc; // 定义变量 crc_calc 用于存储重新计算的 CRC 值
    uint16_t crc_recv; // 定义变量 crc_recv 用于存储接收到的 CRC 值

    /* 【安全】防止 rx_len 过小导致 CRC16 参数回绕（0-2=65534 越界读） */
    if (Modbus.rx_len < 4)
    {
        Modbus.rx_len  = 0;
        Modbus.rx_flag = 0;
        return;
    }

    crc_calc = Modbus_CRC16(Modbus.rx_buf, Modbus.rx_len - 2); // 重新计算接收到的数据的 CRC 值，rx_cnt-2 是为了减去接收到的 CRC 位

    crc_recv = Modbus.rx_buf[Modbus.rx_len - 2] | // 提取接收到的数据的 CRC 位
               (Modbus.rx_buf[Modbus.rx_len - 1] << 8);

    if (crc_calc != crc_recv) // 如果重新计算的 CRC 值与接收到的 CRC 值不同
    {
        Modbus.rx_len  = 0; // 清空接收计数
        Modbus.rx_flag = 0; // 【安全】同步清除标志位
        return;             // 返回
    }

    if (Modbus.rx_buf[0] != MODBUS_SLAVE_ADDR) // 如果接收到的数据的从机地址不匹配
    {
        Modbus.rx_len  = 0; // 清空接收计数
        Modbus.rx_flag = 0; // 【安全】同步清除标志位
        return;             // 返回
    }

    uint8_t func = Modbus.rx_buf[1]; // 提取功能码

    switch (func) // 根据功能码选择对应的处理函数
    {

    case 0x03:               // 如果功能码是 0x03
        Modbus_Function03(); // 调用功能 03 处理函数
        break;

    case 0x04:               // 如果功能码是 0x04
        Modbus_Function04(); // 调用功能 04 处理函数
        break;

    case 0x06:               // 如果功能码是 0x06
        Modbus_Function06(); // 调用功能 06 处理函数
        break;

    case 0x10:               // 如果功能码是 0x10
        Modbus_Function16(); // 调用功能 16 处理函数
        break;

    default:   // 如果功能码不匹配任何已知功能
        break; // 什么都不做
    }

    Modbus.rx_len  = 0; // 清空接收计数
    Modbus.rx_flag = 0; // 【安全】同步清除标志位，防止空帧重入
}

static void Modbus_Function03(void) // Read Holding Register 读取保持寄存器
{

    uint16_t start;    // 定义变量 start 用于存储起始地址
    uint16_t quantity; // 定义变量 quantity 用于存储寄存器数量

    /* 【安全】帧长校验: 地址(2) + 功能码(1) + 起始(2) + 数量(2) + CRC(2) = 8 */
    if (Modbus.rx_len < 8)
        return;

    start = (Modbus.rx_buf[2] << 8) | Modbus.rx_buf[3];    // 从接收缓冲区中提取起始地址
    quantity = (Modbus.rx_buf[4] << 8) | Modbus.rx_buf[5]; // 从接收缓冲区中提取寄存器数量

    /* 【安全】数量上限检查（Modbus 规范: 单次最多 125 个寄存器） */
    if (quantity == 0 || quantity > MODBUS_MAX_REG_QUANTITY)
        return;

    Modbus.tx_buf_arr[0] = MODBUS_SLAVE_ADDR; // 设置从机地址
    Modbus.tx_buf_arr[1] = 0x03;              // 设置功能码为 0x03
    Modbus.tx_buf_arr[2] = quantity * 2;      // 设置数据字节数

    for (uint16_t i = 0; i < quantity; i++) // 遍历寄存器数量
    {
        uint16_t val;                                               // 定义变量 val 用于存储寄存器值
        val = Modbus_ReadRegister(start + i, MODBUS_BLOCK_HOLDING); // 读取保持寄存器的值
        Modbus.tx_buf_arr[3 + i * 2] = val >> 8;                        // 高字节存入发送缓冲区
        Modbus.tx_buf_arr[4 + i * 2] = val;                             // 低字节存入发送缓冲区
    }

    uint16_t len = 3 + quantity * 2; // 计算发送数据长度

    uint16_t crc = Modbus_CRC16(Modbus.tx_buf_arr, len); // 计算 CRC 校验码

    Modbus.tx_buf_arr[len] = crc & 0xFF;   // CRC 低字节存入发送缓冲区
    Modbus.tx_buf_arr[len + 1] = crc >> 8; // CRC 高字节存入发送缓冲区

    Modbus_PortSend(Modbus.tx_buf_arr, len + 2); // 通过端口发送数据
}

static void Modbus_Function04(void) // Read Input Register   读取输入寄存器
{

    uint16_t start;    // 定义变量 start 用于存储起始地址
    uint16_t quantity; // 定义变量 quantity 用于存储寄存器数量

    /* 【安全】帧长校验 */
    if (Modbus.rx_len < 8)
        return;

    start = (Modbus.rx_buf[2] << 8) | Modbus.rx_buf[3];    // 从接收缓冲区中提取起始地址
    quantity = (Modbus.rx_buf[4] << 8) | Modbus.rx_buf[5]; // 从接收缓冲区中提取寄存器数量

    /* 【安全】数量上限检查 */
    if (quantity == 0 || quantity > MODBUS_MAX_REG_QUANTITY)
        return;

    Modbus.tx_buf_arr[0] = MODBUS_SLAVE_ADDR; // 设置从机地址
    Modbus.tx_buf_arr[1] = 0x04;              // 设置功能码为 0x04
    Modbus.tx_buf_arr[2] = quantity * 2;      // 设置数据字节数

    for (uint16_t i = 0; i < quantity; i++) // 遍历寄存器数量
    {
        uint16_t val;                                             // 定义变量 val 用于存储寄存器值
        val = Modbus_ReadRegister(start + i, MODBUS_BLOCK_INPUT); // 读取输入寄存器的值
        Modbus.tx_buf_arr[3 + i * 2] = val >> 8;                      // 高字节存入发送缓冲区
        Modbus.tx_buf_arr[4 + i * 2] = val;                           // 低字节存入发送缓冲区
    }

    uint16_t len = 3 + quantity * 2; // 计算发送数据长度

    uint16_t crc = Modbus_CRC16(Modbus.tx_buf_arr, len); // 计算 CRC 校验码

    Modbus.tx_buf_arr[len] = crc & 0xFF;   // CRC 低字节存入发送缓冲区
    Modbus.tx_buf_arr[len + 1] = crc >> 8; // CRC 高字节存入发送缓冲区

    Modbus_PortSend(Modbus.tx_buf_arr, len + 2); // 通过端口发送数据
}

static void Modbus_Function06(void) // Write Single Register 写单个寄存器
{

    uint16_t addr;  // 定义变量 addr 用于存储寄存器地址
    uint16_t value; // 定义变量 value 用于存储寄存器值

    /* 【安全】帧长校验 */
    if (Modbus.rx_len < 8)
        return;

    addr = (Modbus.rx_buf[2] << 8) | Modbus.rx_buf[3];  // 从接收缓冲区中提取寄存器地址
    value = (Modbus.rx_buf[4] << 8) | Modbus.rx_buf[5]; // 从接收缓冲区中提取寄存器值

    Modbus_WriteRegister(addr, value); // 写入寄存器值到指定地址

    memcpy(Modbus.tx_buf_arr, Modbus.rx_buf, 6); // 将接收缓冲区的前 6 个字节复制到发送缓冲区

    uint16_t crc = Modbus_CRC16(Modbus.tx_buf_arr, 6); // 计算发送缓冲区的 CRC 校验码

    Modbus.tx_buf_arr[6] = crc & 0xFF; // CRC 低字节存入发送缓冲区
    Modbus.tx_buf_arr[7] = crc >> 8;   // CRC 高字节存入发送缓冲区

    Modbus_PortSend(Modbus.tx_buf_arr, 8); // 通过端口发送数据
}

static void Modbus_Function16(void) // Write Multiple Registers 写多个寄存器
{

    uint16_t start;    // 定义变量 start 用于存储起始地址
    uint16_t quantity; // 定义变量 quantity 用于存储寄存器数量
    uint8_t  byte_cnt; // 接收帧中的字节计数

    /* 【安全】帧长校验: 地址(1)+功能码(1)+起始(2)+数量(2)+字节数(1)+CRC(2) = 9 */
    if (Modbus.rx_len < 9)
        return;

    start = (Modbus.rx_buf[2] << 8) | Modbus.rx_buf[3];    // 从接收缓冲区中提取起始地址
    quantity = (Modbus.rx_buf[4] << 8) | Modbus.rx_buf[5]; // 从接收缓冲区中提取寄存器数量
    byte_cnt = Modbus.rx_buf[6];                            // 提取数据字节数

    /* 【安全】数量上限 + 字节数一致性校验 */
    if (quantity == 0 || quantity > MODBUS_MAX_REG_QUANTITY)
        return;
    if (byte_cnt != (uint8_t)(quantity * 2))
        return;

    /* 【安全】接收帧长度校验：确保数据字节完整 */
    if (Modbus.rx_len < (uint16_t)(7 + byte_cnt + 2))
        return;

    for (uint16_t i = 0; i < quantity; i++) // 遍历寄存器数量
    {
        uint16_t value;                           // 定义变量 value 用于存储寄存器值
        value = (Modbus.rx_buf[7 + i * 2] << 8) | // 从接收缓冲区中提取寄存器值的高字节
                Modbus.rx_buf[8 + i * 2];         // 从接收缓冲区中提取寄存器值的低字节
        Modbus_WriteRegister(start + i, value);   // 写入寄存器值到指定地址
    }
    memcpy(Modbus.tx_buf_arr, Modbus.rx_buf, 6);       // 将接收缓冲区的前 6 个字节复制到发送缓冲区
    uint16_t crc = Modbus_CRC16(Modbus.tx_buf_arr, 6); // 计算发送缓冲区的 CRC 校验码
    Modbus.tx_buf_arr[6] = crc & 0xFF;                 // CRC 低字节存入发送缓冲区
    Modbus.tx_buf_arr[7] = crc >> 8;                   // CRC 高字节存入发送缓冲区
    Modbus_PortSend(Modbus.tx_buf_arr, 8);             // 通过端口发送数据
}

void Modbus_Task(void)
{
    for (uint8_t i = 0; i < MODBUS_PORT_COUNT; i++)
    {
        if (modbus_ports[i].buf->rx_flag == 1)
        {
            Modbus_FillData(modbus_ports[i].buf, modbus_ports[i].send);
            break;
        }
    }

    if (Modbus.rx_flag == 1)
    {
        Modbus_FrameProcess();
    }
}
