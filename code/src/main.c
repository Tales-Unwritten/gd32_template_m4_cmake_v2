#include "gd32f4xx_rcu.h"
#include "gd32f4xx_gpio.h"
#include "systick.h"
#include <stdio.h>
#include "main.h"
#include "rtos_conf.h"

#include "function.hpp"

#ifdef APP_IAP
#include "iap_app.hpp"
#endif

#if USE_FREERTOS
#include "FreeRTOS.h"
#include "task.h"

/* 默认任务：承载原裸机主循环 function_loop()。
 *
 * ⚠️ 故意不调用 vTaskDelay / taskYIELD：
 *   本工程的软串口 soft_uart_port 是「轮询式」实现，function_loop() 必须
 *   高频调用 suart.poll() 才能不丢字节（115200 下每字节仅 ~87µs）。
 *   加 1ms 延时会令轮询率降到 1kHz → 接收乱码。
 *   等价于裸机的 while(1) 忙轮询：本任务持续占用 CPU，空闲任务不运行。
 *   后续若改为中断/DMA 收串口，可在此加 vTaskDelay 让出 CPU。 */
static void default_task(void *argument)
{
    (void)argument;

    for (;;)
    {
        function_loop();
    }
}
#endif /* USE_FREERTOS */

int main(void)
{
#ifdef APP_IAP
    /* IAP：VTOR 重定位必须先于任何中断使能（systick_config 之前） */
    Iap_AppInit();
#endif

#if USE_FREERTOS
    /* ── FreeRTOS 模式 ──────────────────────────────────────
     * ⚠️ 不调用 systick_config()：SysTick 由 xPortStartScheduler()
     *    配置，优先级取自 configKERNEL_INTERRUPT_PRIORITY。
     *    get_tick() 兼容由 vApplicationTickHook() 维持。 */
    function_init();

    if (xTaskCreate(default_task, "default", 512, NULL,
                    tskIDLE_PRIORITY + 1, NULL) != pdPASS)
    {
        /* 任务创建失败（堆不足 / 参数非法）：停机便于调试 */
        for (;;)
        {
        }
    }

    vTaskStartScheduler();   /* 正常情况不返回 */

    /* 仅当堆不足以创建空闲/定时器任务时才会执行到这里 */
    for (;;)
    {
    }
#else
    /* ── 裸机模式 ─────────────────────────────────────────── */
    systick_config();
    function_init();

    while (1)
    {
        function_loop();
    }
#endif
}
