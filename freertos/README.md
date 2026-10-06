# FreeRTOS 内核（随工程裁剪）

- **版本**：FreeRTOS Kernel **V11.3.1**
- **来源**：https://github.com/FreeRTOS/FreeRTOS-Kernel （tag `V11.3.1`）
- **许可证**：MIT（见 `LICENSE.md`）

## 目录结构

```
freertos/
├── *.c                          # 内核源码
│   ├── tasks.c                  # 任务调度
│   ├── list.c                   # 内核链表
│   ├── queue.c                  # 队列 / 信号量 / 互斥量
│   ├── timers.c                 # 软件定时器
│   ├── event_groups.c           # 事件组
│   ├── stream_buffer.c          # 流 / 消息缓冲
│   └── croutine.c               # 协程（当前未编译）
├── include/                     # 内核头文件（FreeRTOS.h / task.h / ...）
└── portable/
    ├── GCC/ARM_CM4F/            # ★ 本工程使用的移植层（arm-none-eabi-gcc, 硬浮点）
    │   ├── port.c
    │   └── portmacro.h
    ├── RVDS/ARM_CM4F/           # Keil/ARMCC(AC5) 备用移植层（不参与 CMake 构建）
    │   ├── port.c
    │   └── portmacro.h
    └── MemMang/                 # 内存管理，五选一
        ├── heap_1.c … heap_5.c
        └── （本工程编译 heap_4.c）
```

## 上游目录裁剪说明

上游仓库还包含 IAR / CCS / ARMClang / MSVC 等其他编译器与架构的移植层，
以及 `examples/`、`History.txt` 等。本工程只保留了：

- Cortex-M4F 的 **GCC** 与 **RVDS(Keil)** 两个移植层；
- 全部 MemMang 内存管理（便于按需切换）。

因此本目录不是上游完整发行版；如需其他端口，请从对应 tag 重新获取。

## 本工程集成要点

1. **Cortex-M4F + 硬浮点**：必须使用 `portable/GCC/ARM_CM4F`（会保存 FPU 上下文），
   不可用 `ARM_CM3`（不保存 FPU，hard-float 下会出错）。
2. **应用配置**：`code/inc/FreeRTOSConfig.h`（随内核版本维护）。
3. **内存管理**：编译 `heap_4.c`，堆大小 `configTOTAL_HEAP_SIZE`（默认 32KB），
   位于其内部静态数组 `ucHeap[]`（BSS），不需要改链接脚本。
4. **异常接管**：`FreeRTOSConfig.h` 中通过宏映射
   `vPortSVCHandler→SVC_Handler`、`xPortPendSVHandler→PendSV_Handler`、
   `xPortSysTickHandler→SysTick_Handler`，由 `port.c` 提供实现；
   故 `code/src/gd32f4xx_it.c` 中同名裸机实现已被 `#if !USE_FREERTOS` 屏蔽。
5. **开关**：编译期由 `USE_FREERTOS` 控制（`code/inc/rtos_conf.h` /
   CMake 选项 `USE_FREERTOS`，默认 ON）。

## Keil（MDK）备用

- **Keil MDK + ARM Compiler 5（AC5 / `armcc`）**：使用 `portable/RVDS/ARM_CM4F`
  （`RVDS` 即 Keil 旧版 ARMCC 端口目录；`portable/Keil/` 为空目录，仅指示）。
- **Keil MDK + ARM Compiler 6（AC6 / `armclang`）**：官方说明直接使用 **GCC** 端口
  （见上游 `portable/ARMClang/Use-the-GCC-ports.txt`），即 `portable/GCC/ARM_CM4F`。

Keil 工程只需把本目录的 `*.c`、`GCC/ARM_CM4F`（或 `RVDS/ARM_CM4F`）、`MemMang/heap_4.c`
加入分组，并设置相应的 include 路径即可。
