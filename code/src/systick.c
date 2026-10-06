/*!
    \file    systick.c
    \brief   the systick configuration file

    \version 2026-02-05, V3.3.3, firmware for GD32F4xx
*/

/*
    Copyright (c) 2026, GigaDevice Semiconductor Inc.

    Redistribution and use in source and binary forms, with or without modification,
are permitted provided that the following conditions are met:

    1. Redistributions of source code must retain the above copyright notice, this
       list of conditions and the following disclaimer.
    2. Redistributions in binary form must reproduce the above copyright notice,
       this list of conditions and the following disclaimer in the documentation
       and/or other materials provided with the distribution.
    3. Neither the name of the copyright holder nor the names of its contributors
       may be used to endorse or promote products derived from this software without
       specific prior written permission.

    THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT,
INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY
OF SUCH DAMAGE.
*/

#include "gd32f4xx.h"
#include "systick.h"
#include "rtos_conf.h"

static volatile uint32_t tick;   /* 1kHz 毫秒计数器，SysTick_Handler 中递增 */

/*!
    \brief    configure systick
    \param[in]  none
    \param[out] none
    \retval     none
*/
void systick_config(void)
{
    /* setup systick timer for 1000Hz interrupts */
    if(SysTick_Config(SystemCoreClock / 1000U)) {
        /* capture error */
        while(1) {
        }
    }
    /* configure the systick handler priority */
    NVIC_SetPriority(SysTick_IRQn, 0x00U);
}

/* ⚠️ USE_FREERTOS=1 时不应调用 systick_config()：
 *   SysTick 由 FreeRTOS 的 xPortStartScheduler() 配置，优先级取自
 *   configKERNEL_INTERRUPT_PRIORITY；此处硬编码 0x00 会覆盖它并破坏
 *   内核临界区（BASEPRI）语义。裸机模式下仍正常使用。*/

void delay_ms(uint32_t ms)
{
    /* volatile：阻止 -O2 优化掉空转循环（否则延时为 0） */
    volatile uint32_t n = 40000U * ms;
    while (n--)
    {
    }
}

void delay_us(uint32_t us)
{
    volatile uint32_t n = 40U * us;
    while (n--)
    {
    }
}

void tick_increment(void)
{
    tick++;
}

uint32_t get_tick(void)
{
    return tick;
}

#if USE_FREERTOS
/* FreeRTOS 每 1ms 调用一次（configUSE_TICK_HOOK=1，运行于 ISR 上下文）。
 * 作用：让裸机遗留的 get_tick()（device 层若干驱动用作毫秒时间戳）在
 *        FreeRTOS 下继续有效，无需改动调用方。 */
void vApplicationTickHook(void)
{
    tick_increment();
}
#endif /* USE_FREERTOS */
