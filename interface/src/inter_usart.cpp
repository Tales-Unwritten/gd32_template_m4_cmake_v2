#include "inter_usart.hpp"

// 本编译单元仅保留非模板内容：
//   - 全局端口表 g_port_map[]（基类指针）
//   - 注册/注销自由函数（usart_port_register / usart_port_unregister）
//   - 通用 ISR 处理与各 USARTx_IRQHandler
// 模板类 usart_port<RX_BUF_SIZE> 的成员定义内联于 inter_usart.hpp。

// ============================================================
//  文件内全局：端口映射表（ISR 通过它找到对应实例）
//  表项为基类指针，硬件串口 usart_port<RX> 实例注册到此处。
// ============================================================

namespace {

constexpr uint8_t UART_MAX = 8;

usart_port_base *g_port_map[UART_MAX] = {};

uint8_t _port_index(uint32_t periph)
{
    switch (periph)
    {
        case USART0: return 0;
        case USART1: return 1;
        case USART2: return 2;
        case UART3:  return 3;
        case UART4:  return 4;
        case USART5: return 5;
        case UART6:  return 6;
        case UART7:  return 7;
        default:     return UART_MAX;
    }
}

} // anonymous namespace

// ============================================================
//  注册 / 注销（非模板自由函数，供模板成员调用）
// ============================================================

void usart_port_register(usart_port_base *port)
{
    if (!port) return;
    uint8_t idx = _port_index(port->periph());
    if (idx < UART_MAX) g_port_map[idx] = port;
}

void usart_port_unregister(usart_port_base *port)
{
    if (!port) return;
    uint8_t idx = _port_index(port->periph());
    if (idx < UART_MAX && g_port_map[idx] == port) g_port_map[idx] = nullptr;
}

// ============================================================
//  通用 ISR 处理函数（由各 USARTx_IRQHandler 调用）
//  全程通过基类虚接口访问，与具体模板实例无关。
// ============================================================

static void _uart_isr(usart_port_base *port)
{
    if (!port || !port->is_initialized()) return;

    uint32_t         periph = port->periph();
    uart_buffer_t   *buf    = port->buffer();
    uint16_t         cap    = port->rx_buf_capacity();

    /* ---- 接收：RBNE ---- */
    if (usart_interrupt_flag_get(periph, USART_INT_FLAG_RBNE) == SET)
    {
        uint8_t data = (uint8_t)usart_data_receive(periph);

        if (buf->rx_len < cap)
        {
            buf->rx_buf[buf->rx_len++] = data;
        }
    }

    /* ---- 空闲帧：IDLE ---- */
    if (usart_interrupt_flag_get(periph, USART_INT_FLAG_IDLE) == SET)
    {
        usart_data_receive(periph);     // 读 DR 清除 IDLE 标志
        buf->rx_flag = 1;
    }

    /* ---- 错误处理 ---- */
    if (usart_interrupt_flag_get(periph, USART_INT_FLAG_ERR_ORERR) ||
        usart_interrupt_flag_get(periph, USART_INT_FLAG_ERR_FERR) ||
        usart_interrupt_flag_get(periph, USART_INT_FLAG_ERR_NERR) ||
        usart_interrupt_flag_get(periph, USART_INT_FLAG_PERR))
    {
        usart_data_receive(periph);
        usart_flag_clear(periph, USART_FLAG_ORERR);
        usart_flag_clear(periph, USART_FLAG_FERR);
        usart_flag_clear(periph, USART_FLAG_NERR);
        usart_flag_clear(periph, USART_FLAG_PERR);

        buf->rx_len = 0;  buf->rx_flag = 0;  // 丢弃残缺帧
    }

    /* ---- 发送：TBE ---- */
    if (usart_interrupt_flag_get(periph, USART_INT_FLAG_TBE) == SET)
    {
        if (buf->tx_idx < buf->tx_len)
        {
            usart_data_transmit(periph, buf->tx_data[buf->tx_idx++]);
        }
        else
        {
            usart_interrupt_disable(periph, USART_INT_TBE);
            usart_interrupt_enable(periph, USART_INT_TC);
        }
    }

    /* ---- 发送完成：TC ---- */
    if (usart_interrupt_flag_get(periph, USART_INT_FLAG_TC) == SET)
    {
        usart_flag_clear(periph, USART_FLAG_TC);
        usart_interrupt_disable(periph, USART_INT_TC);
        buf->tx_busy = 0;
    }
}

// ============================================================
//  各 USART 中断入口（全部内聚在此，用户无需外部编写）
// ============================================================

extern "C" void USART0_IRQHandler(void)
{
    _uart_isr(g_port_map[_port_index(USART0)]);
}

extern "C" void USART1_IRQHandler(void)
{
    _uart_isr(g_port_map[_port_index(USART1)]);
}

extern "C" void USART2_IRQHandler(void)
{
    _uart_isr(g_port_map[_port_index(USART2)]);
}

extern "C" void UART3_IRQHandler(void)
{
    _uart_isr(g_port_map[_port_index(UART3)]);
}

extern "C" void UART4_IRQHandler(void)
{
    _uart_isr(g_port_map[_port_index(UART4)]);
}

extern "C" void USART5_IRQHandler(void)
{
    _uart_isr(g_port_map[_port_index(USART5)]);
}

extern "C" void UART6_IRQHandler(void)
{
    _uart_isr(g_port_map[_port_index(UART6)]);
}

extern "C" void UART7_IRQHandler(void)
{
    _uart_isr(g_port_map[_port_index(UART7)]);
}
