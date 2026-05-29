// ============================================================
// @platform GD32F4xx（当前平台）
//   移植到新 MCU 时，本文件需要完整重写，但公共接口签名不变。
//   需要替换的映射：
//     - 外设基址:   USART0/USART1/... → 目标平台外设基址
//     - 时钟使能:   已在 .hpp 模板中完成
//     - 中断管理:   nvic_irq_enable → 目标平台 NVIC API
//     - ISR 入口:   USART0_IRQHandler → 目标平台中断向量名
//     - 寄存器访问:  usart_flag_get/usart_data_receive → 目标平台 API
// ============================================================

#include "usart_async_port.hpp"

#include "gd32f4xx.h"
#include "gd32f4xx_rcu.h"
#include "gd32f4xx_usart.h"
#include "gd32f4xx_misc.h"

// ============================================================
//  全局端口映射表
// ============================================================

namespace {

constexpr uint8_t UART_MAX = 8;
usart_isr_ctx_t *g_port_map[UART_MAX] = {};

uint8_t _port_index(usart_id id)
{
    return static_cast<uint8_t>(id);
}

} // anonymous namespace

// ============================================================
//  ISR 注册 / 注销
// ============================================================

void usart_isr_register(usart_id id, usart_isr_ctx_t *ctx)
{
    uint8_t idx = _port_index(id);
    if (idx < UART_MAX) {
        g_port_map[idx] = ctx;
    }
}

void usart_isr_unregister(usart_id id)
{
    uint8_t idx = _port_index(id);
    if (idx < UART_MAX) {
        g_port_map[idx] = nullptr;
    }
}

// ============================================================
//  ISR 核心处理函数（文件作用域，不暴露）
// ============================================================

static void _usart_isr_handler(usart_isr_ctx_t *ctx)
{
    if (!ctx || ctx->periph == 0) return;

    uint32_t periph = ctx->periph;

    // ---- 接收：RBNE ----
    if (usart_interrupt_flag_get(periph, USART_INT_FLAG_RBNE) == SET)
    {
        uint8_t data = (uint8_t)usart_data_receive(periph);

        if (*ctx->rx_len < ctx->rx_capacity)
        {
            ctx->rx_buf[*ctx->rx_len] = data;
            (*ctx->rx_len)++;
        }
    }

    // ---- 空闲帧：IDLE ----
    if (usart_interrupt_flag_get(periph, USART_INT_FLAG_IDLE) == SET)
    {
        usart_data_receive(periph);     // 读 DR 清除 IDLE 标志
        *ctx->rx_flag = 1;
    }

    // ---- 错误处理 ----
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

        *ctx->rx_len  = 0;              // 丢弃残缺帧
        *ctx->rx_flag = 0;
    }

    // ---- 发送：TBE ----
    if (usart_interrupt_flag_get(periph, USART_INT_FLAG_TBE) == SET)
    {
        if (ctx->tx_idx < ctx->tx_len)
        {
            usart_data_transmit(periph, ctx->tx_data[ctx->tx_idx]);
            ctx->tx_idx++;
        }
        else
        {
            usart_interrupt_disable(periph, USART_INT_TBE);
            usart_interrupt_enable(periph, USART_INT_TC);
        }
    }

    // ---- 发送完成：TC ----
    if (usart_interrupt_flag_get(periph, USART_INT_FLAG_TC) == SET)
    {
        usart_flag_clear(periph, USART_FLAG_TC);
        usart_interrupt_disable(periph, USART_INT_TC);
        *ctx->tx_busy = 0;
    }
}

// ============================================================
//  各 USART 中断入口（全部内聚在此文件）
//  用户无需在 gd32f4xx_it.c 中添加任何 USART ISR
// ============================================================

extern "C" void USART0_IRQHandler(void) {
    _usart_isr_handler(g_port_map[_port_index(usart_id::usart0)]);
}

extern "C" void USART1_IRQHandler(void) {
    _usart_isr_handler(g_port_map[_port_index(usart_id::usart1)]);
}

extern "C" void USART2_IRQHandler(void) {
    _usart_isr_handler(g_port_map[_port_index(usart_id::usart2)]);
}

extern "C" void UART3_IRQHandler(void) {
    _usart_isr_handler(g_port_map[_port_index(usart_id::uart3)]);
}

extern "C" void UART4_IRQHandler(void) {
    _usart_isr_handler(g_port_map[_port_index(usart_id::uart4)]);
}

extern "C" void USART5_IRQHandler(void) {
    _usart_isr_handler(g_port_map[_port_index(usart_id::usart5)]);
}

extern "C" void UART6_IRQHandler(void) {
    _usart_isr_handler(g_port_map[_port_index(usart_id::uart6)]);
}

extern "C" void UART7_IRQHandler(void) {
    _usart_isr_handler(g_port_map[_port_index(usart_id::uart7)]);
}
