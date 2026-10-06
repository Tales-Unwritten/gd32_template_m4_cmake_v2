/*!
    \file    rtos_conf.h
    \brief   FreeRTOS / 裸机 切换总开关

    该头文件是「是否使用 FreeRTOS 调度器」的唯一软件开关，供 C / C++ 源码
    在编译期判断，例如：

        #include "rtos_conf.h"
        #if USE_FREERTOS
            vTaskStartScheduler();
        #else
            while (1) { function_loop(); }
        #endif

    取值说明：
        1 = 使用 FreeRTOS 调度器（由 freertos/ 内核接管）
        0 = 裸机主循环（不使用 FreeRTOS）

    覆盖方式（二者任选其一，CMake 优先级更高）：
        - 改本文件下面的默认值；
        - 或在 CMake 配置时传入：cmake --preset debug -DUSE_FREERTOS=OFF

    ⚠️ 与 USE_FREERTOS=1 关联的三处「冲突点」（详见代码注释）：
        1. freertos/portable/GCC/ARM_CM4F/port.c 通过 FreeRTOSConfig.h 的
           宏映射接管 SVC / PendSV / SysTick 三个异常；
           故 code/src/gd32f4xx_it.c 中对应的裸机实现必须屏蔽。
        2. SysTick 由 FreeRTOS 配置，code/src/systick.c 的 systick_config()
           不应再被调用（否则覆盖 FreeRTOS 的中断优先级）。
        3. get_tick() 的毫秒计数改由 vApplicationTickHook() 维持。
*/

#ifndef __RTOS_CONF_H
#define __RTOS_CONF_H

/* 若构建系统未传入 USE_FREERTOS，则采用此默认值 */
#ifndef USE_FREERTOS
#define USE_FREERTOS 1
#endif

#endif /* __RTOS_CONF_H */
