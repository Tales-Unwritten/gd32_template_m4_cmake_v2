/*
 * FreeRTOSConfig.h — 项目应用配置（FreeRTOS Kernel V11.3.1）
 * ------------------------------------------------------------------
 * 目标平台：GD32F470ZIT6  (Cortex-M4F, 4 位 NVIC 优先级, 240MHz)
 * 编译器  ：arm-none-eabi-gcc (可切换 freertos/portable/RVDS 供 Keil 备用)
 *
 * 该文件由 FreeRTOS.h 自动包含，必须位于编译器的 include 路径中
 * （本工程放在 code/inc/）。
 * ------------------------------------------------------------------
 */

#ifndef FREERTOS_CONFIG_H
#define FREERTOS_CONFIG_H

/*==========================================================================
 * 调度器基础
 *========================================================================*/
#define configUSE_PREEMPTION                     1   /* 抢占式调度 */
#define configUSE_TIME_SLICING                   1   /* 同优先级时间片轮转 */
#define configUSE_PORT_OPTIMISED_TASK_SELECTION   1 /* Cortex-M 硬件 CLZ 选任务 */
#define configUSE_TICKLESS_IDLE                  0   /* 不使用低功耗 tickless */
#define configCPU_CLOCK_HZ                       ( 240000000UL ) /* GD32F470 @240MHz PLL_25M_HXTAL */
#define configTICK_RATE_HZ                       ( 1000UL )      /* 1ms / tick */

#define configMAX_PRIORITIES                     ( 8 )
#define configMINIMAL_STACK_SIZE                 ( 128 )     /* 空闲任务栈，单位 word (512B) */
#define configMAX_TASK_NAME_LEN                  ( 16 )
#define configTICK_TYPE_WIDTH_IN_BITS            TICK_TYPE_WIDTH_32_BITS
#define configIDLE_SHOULD_YIELD                  ( 1 )

/*==========================================================================
 * 内存管理（使用 heap_4.c：可释放 + 相邻块合并）
 *========================================================================*/
#define configTOTAL_HEAP_SIZE                    ( 32 * 1024 )   /* 32KB 静态堆(ucHeap[]) */
#define configSUPPORT_STATIC_ALLOCATION          ( 0 )
#define configSUPPORT_DYNAMIC_ALLOCATION         ( 1 )
#define configAPPLICATION_ALLOCATED_HEAP         ( 0 )

/*==========================================================================
 * 可选内核对象 / 功能
 *========================================================================*/
#define configUSE_TASK_NOTIFICATIONS             ( 1 )
#define configUSE_MUTEXES                        ( 1 )
#define configUSE_RECURSIVE_MUTEXES              ( 1 )
#define configUSE_COUNTING_SEMAPHORES            ( 1 )
#define configUSE_QUEUE_SETS                     ( 1 )   /* 队列集 xQueueCreateSet / xQueueSelectFromSet */
#define configUSE_STREAM_BUFFERS                 ( 1 )
#define configUSE_APPLICATION_TASK_TAG           ( 1 )   /* 任务标签：vTaskSetApplicationTaskTag 等 */
#define configUSE_SB_COMPLETED_CALLBACK          ( 1 )   /* 流/消息缓冲发送·接收完成回调（*CreateWithCallback） */
#define configQUEUE_REGISTRY_SIZE                ( 8 )
#define configUSE_NEWLIB_REENTRANT               ( 0 )

/*==========================================================================
 * 软件定时器（timers.c）
 *========================================================================*/
#define configUSE_TIMERS                         ( 1 )
#define configTIMER_TASK_PRIORITY                ( configMAX_PRIORITIES - 1 )
#define configTIMER_QUEUE_LENGTH                 ( 10 )
#define configTIMER_TASK_STACK_DEPTH             ( configMINIMAL_STACK_SIZE * 2 )

/*==========================================================================
 * 钩子函数（Hook）
 *   - Tick Hook 用于维持裸机 get_tick() 兼容（见 code/src/systick.c）
 *========================================================================*/
#define configUSE_IDLE_HOOK                      ( 0 )
#define configUSE_TICK_HOOK                      ( 1 )

/*==========================================================================
 * 运行时统计 / 调试
 *========================================================================*/
#define configGENERATE_RUN_TIME_STATS            ( 0 )   /* 需自定义运行时间计数器，暂不启用 */
#define configUSE_TRACE_FACILITY                 ( 1 )   /* vTaskGetInfo / uxTaskGetSystemState / uxQueueGetQueueNumber */
#define configUSE_STATS_FORMATTING_FUNCTIONS     ( 1 )   /* vTaskList（依赖 TRACE_FACILITY；会引入 sprintf） */
#define configCHECK_FOR_STACK_OVERFLOW           ( 0 )
#define configUSE_MALLOC_FAILED_HOOK             ( 0 )

/*==========================================================================
 * 中断优先级配置（GD32F4 / Cortex-M4 使用高 4 位）
 *   configKERNEL_INTERRUPT_PRIORITY       —— 内核(滴答/PendSV)优先级 (最低 15)
 *   configMAX_SYSCALL_INTERRUPT_PRIORITY  —— 可调用 FreeRTOS API 的最高优先级
 *   ⚠️ 任何调用 FreeRTOS API 的 ISR，其 NVIC 优先级必须 <= 5
 *      （数值越大优先级越低）。
 *========================================================================*/
#define configPRIO_BITS                          ( 4 )
#define configLIBRARY_LOWEST_INTERRUPT_PRIORITY         ( 15 )
#define configLIBRARY_MAX_SYSCALL_INTERRUPT_PRIORITY    ( 5 )
#define configKERNEL_INTERRUPT_PRIORITY \
    ( configLIBRARY_LOWEST_INTERRUPT_PRIORITY << ( 8 - configPRIO_BITS ) )
#define configMAX_SYSCALL_INTERRUPT_PRIORITY \
    ( configLIBRARY_MAX_SYSCALL_INTERRUPT_PRIORITY << ( 8 - configPRIO_BITS ) )

/*==========================================================================
 * Handler 名称映射（GCC 端口）
 *   port.c 中的 vPortSVCHandler / xPortPendSVHandler / xPortSysTickHandler
 *   映射到启动文件向量表使用的 CMSIS 名称。
 *   ⇒ 因此 code/src/gd32f4xx_it.c 中的同名裸机实现必须屏蔽！
 *========================================================================*/
#define vPortSVCHandler                          SVC_Handler
#define xPortPendSVHandler                       PendSV_Handler
#define xPortSysTickHandler                      SysTick_Handler

/*==========================================================================
 * API 函数裁剪（按需开启，关闭可减小体积）
 *   注意：INCLUDE_ 只门控「可选 API」；队列 / 信号量 / 互斥量由
 *         configUSE_* 与 configSUPPORT_* 决定，不受 INCLUDE_ 控制。
 *========================================================================*/

/* ── 任务类 ─────────────────────────────────────────────── */
#define INCLUDE_vTaskPrioritySet                 ( 1 )
#define INCLUDE_uxTaskPriorityGet                ( 1 )
#define INCLUDE_vTaskDelete                      ( 1 )
#define INCLUDE_vTaskSuspend                     ( 1 )
#define INCLUDE_vTaskDelay                       ( 1 )
#define INCLUDE_xTaskDelayUntil                  ( 1 )   /* v11 起的新名（替代 INCLUDE_vTaskDelayUntil） */
#define INCLUDE_xTaskAbortDelay                  ( 1 )
#define INCLUDE_xTaskGetCurrentTaskHandle        ( 1 )
#define INCLUDE_xTaskGetSchedulerState           ( 1 )
#define INCLUDE_xTaskGetIdleTaskHandle           ( 1 )
#define INCLUDE_xTaskGetHandle                   ( 1 )
#define INCLUDE_eTaskGetState                    ( 1 )
#define INCLUDE_uxTaskGetStackHighWaterMark      ( 1 )
#define INCLUDE_uxTaskGetStackHighWaterMark2     ( 1 )
#define INCLUDE_xTaskResumeFromISR               ( 1 )

/* ── 队列 / 互斥量 附加查询 ─────────────────────────────── */
#define INCLUDE_xQueueGetMutexHolder             ( 1 )
#define INCLUDE_xSemaphoreGetMutexHolder         ( 1 )

/* ── 软件定时器 ─────────────────────────────────────────── */
#define INCLUDE_xTimerPendFunctionCall           ( 1 )

/*==========================================================================
 * 断言：命中即关中断死循环，便于调试器定位
 *========================================================================*/
#define configASSERT( x )                        \
    if( ( x ) == 0 )                             \
    {                                            \
        taskDISABLE_INTERRUPTS();                \
        for( ;; );                               \
    }

#endif /* FREERTOS_CONFIG_H */
