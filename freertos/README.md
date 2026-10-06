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

---

## 常用 API 速查（按功能分类）

> 格式：`函数名  /// 功能说明`。**函数名是可点击链接**，点击直接跳到**代码实现处**：
> 函数 → 对应 `.c` 实现；宏 → 跳到它最终调用的 `.c` 实现
> （如 `xQueueCreate` → `queue.c` 的 `xQueueGenericCreate`；`xTimerStart` → `timers.c` 的 `xTimerGenericCommand`）。
> 仅极少数**纯头文件宏**（内联寄存器操作/列表宏，如 `taskYIELD`、`portYIELD`、`listGET_OWNER_OF_NEXT_ENTRY`）只能指向头文件。
>
> 可用性标注（对应本工程 `code/inc/FreeRTOSConfig.h` 当前配置）：
> **✅ 已启用** ｜ **⚙️ 需开某宏**（见括注，当前未启用） ｜ **➖ 底层/内部**（一般不直接调用） ｜ **⛔ 本工程不适用**（MPU / SMP 多核 / 协程）

### 1. 任务（Tasks） — `include/task.h`

```text
[xTaskCreate](tasks.c#L1729)                           /// ✅ 动态创建任务（自动分配栈 + TCB）
[xTaskCreateStatic](tasks.c#L1320)                     /// ⚙️ 静态创建任务（自备栈/TCB，需 configSUPPORT_STATIC_ALLOCATION=1）
[xTaskCreateRestricted](tasks.c#L1560)                 /// ⛔ MPU 受限任务
[xTaskCreateRestrictedStatic](tasks.c#L1440)           /// ⛔ MPU 静态受限任务
[xTaskCreateAffinitySet](tasks.c#L1767)                /// ⛔ SMP 多核亲和
[xTaskCreateStaticAffinitySet](tasks.c#L1358)          /// ⛔
[xTaskCreateRestrictedAffinitySet](tasks.c#L1595)      /// ⛔
[xTaskCreateRestrictedStaticAffinitySet](tasks.c#L1476) /// ⛔
[vTaskDelete](tasks.c#L2212)                           /// ✅ 删除任务
[vTaskDelay](tasks.c#L2506)                            /// ✅ 相对阻塞延时（单位 tick）
[vTaskDelayUntil](tasks.c#L2414)                       /// ✅ 绝对周期性延时（旧名，仍兼容）
[xTaskDelayUntil](tasks.c#L2414)                       /// ✅ 绝对周期性延时（v11 新名）
[xTaskAbortDelay](tasks.c#L4737)                       /// ✅ 强制解除目标任务阻塞
[vTaskPrioritySet](tasks.c#L2844)                      /// ✅ 修改任务优先级
[uxTaskPriorityGet](tasks.c#L2688)                     /// ✅ 读取任务优先级
[uxTaskPriorityGetFromISR](tasks.c#L2716)              /// ✅ ISR 中读优先级
[uxTaskBasePriorityGet](tasks.c#L2766)                 /// ✅ 读“基础”优先级（不含优先级继承）
[uxTaskBasePriorityGetFromISR](tasks.c#L2794)          /// ✅ ISR 版本
[vTaskSuspend](tasks.c#L3197)                          /// ✅ 挂起任务
[vTaskResume](tasks.c#L3430)                           /// ✅ 恢复任务
[xTaskResumeFromISR](tasks.c#L3491)                    /// ✅ ISR 中恢复任务（返回是否需切换）
[xTaskGetSchedulerState](tasks.c#L6705)                /// ✅ 查询调度器状态（未启动/运行/挂起）
[xTaskGetCurrentTaskHandle](tasks.c#L6645)             /// ✅ 当前任务句柄
[xTaskGetIdleTaskHandle](tasks.c#L4626)                /// ✅ 空闲任务句柄
[xTaskGetHandle](tasks.c#L4370)                        /// ✅ 按名字查任务句柄
[eTaskGetState](tasks.c#L2556)                         /// ✅ 查询任务状态（就绪/阻塞/挂起…）
[uxTaskGetNumberOfTasks](tasks.c#L4267)                /// ✅ 任务总数
[pcTaskGetName](tasks.c#L4279)                         /// ✅ 取任务名
[uxTaskGetStackHighWaterMark](tasks.c#L6535)           /// ✅ 栈历史最小剩余（word）
[uxTaskGetStackHighWaterMark2](tasks.c#L6495)          /// ✅ 同上（返回类型 configSTACK_DEPTH_TYPE）
[vTaskGetInfo](tasks.c#L6285)                          /// ✅ 取任务详情结构体（需 TRACE_FACILITY）
[vTaskSetApplicationTaskTag](tasks.c#L5083)            /// ✅ 设置任务标签（调试）
[xTaskGetApplicationTaskTag](tasks.c#L5117)            /// ✅ 读任务标签
[xTaskGetApplicationTaskTagFromISR](tasks.c#L5146)     /// ✅ ISR 版本
[vTaskSetThreadLocalStoragePointer](tasks.c#L6100)     /// ⚙️ 设 TLS 指针（需 configNUM_THREAD_LOCAL_STORAGE_POINTERS>0）
[pvTaskGetThreadLocalStoragePointer](tasks.c#L6124)    /// ⚙️ 读 TLS 指针
[vTaskSetTimeOutState](tasks.c#L5665)                  /// ✅ 记录超时起点（配合下一条）
[xTaskCheckForTimeOut](tasks.c#L5693)                  /// ✅ 判断是否超时（可续时重试）
[xTaskGetTickCount](tasks.c#L4213)                     /// ✅ 当前 tick 计数
[xTaskGetTickCountFromISR](tasks.c#L4232)              /// ✅ ISR 取 tick
[xTaskCatchUpTicks](tasks.c#L4707)                     /// ✅ 补偿 tick（tickless 场景）
[uxTaskGetSystemState](tasks.c#L4589)                  /// ✅ 取全部任务状态快照（需 TRACE_FACILITY）
[vTaskList](tasks.c#L7396)                             /// ✅ 打印任务列表（需 TRACE + STATS_FORMATTING）
[vTaskGetRunTimeStats](tasks.c#L7566)                  /// ⚙️ 打印各任务 CPU 占比（需 configGENERATE_RUN_TIME_STATS=1）
[ulTaskGetRunTimeCounter](tasks.c#L8521)               /// ⚙️ 取任务累计运行时间
[ulTaskGetRunTimePercent](tasks.c#L8562)               /// ⚙️ 取任务运行占比(%)
[ulTaskGetIdleRunTimeCounter](tasks.c#L8603)           /// ⚙️ 空闲任务累计运行时间
[ulTaskGetIdleRunTimePercent](tasks.c#L8648)           /// ⚙️ 空闲任务运行占比(%)
```

### 2. 内核 / 调度器控制（Kernel Control） — `include/task.h`

```text
[vTaskStartScheduler](tasks.c#L3742)                   /// ✅ 启动调度器（正常不返回）
[vTaskEndScheduler](tasks.c#L3847)                     /// ✅ 停止调度器
[vTaskSuspendAll](tasks.c#L3890)                       /// ✅ 挂起调度器（不关中断，可嵌套）
[xTaskResumeAll](tasks.c#L4064)                        /// ✅ 恢复调度器（返回期间是否有切换）
[taskENTER_CRITICAL()](portable/GCC/ARM_CM4F/port.c#L475)                  /// ✅ 进入临界区（宏：关中断至 BASEPRI）
[taskEXIT_CRITICAL()](portable/GCC/ARM_CM4F/port.c#L492)                   /// ✅ 退出临界区
[taskENTER_CRITICAL_FROM_ISR()](include/task.h#L243)         /// ✅ ISR 进入临界区
[taskEXIT_CRITICAL_FROM_ISR(x)](include/task.h#L262)         /// ✅ ISR 退出临界区
[taskDISABLE_INTERRUPTS()](include/task.h#L275)              /// ✅ 全局关中断
[taskENABLE_INTERRUPTS()](include/task.h#L285)               /// ✅ 全局开中断
[taskYIELD()](include/task.h#L227)                           /// ✅ 请求任务切换（触发 PendSV）
[portYIELD_FROM_ISR(x)](portable/GCC/ARM_CM4F/portmacro.h#L114)                 /// ✅ ISR 内请求切换（底层宏）
[portYIELD()](portable/GCC/ARM_CM4F/portmacro.h#L88)                           /// ✅ port 层触发 PendSV 的宏
```

### 3. 队列（Queues） — `include/queue.h`

```text
[xQueueCreate](queue.c#L502)                          /// ✅ 动态创建队列
[xQueueCreateStatic](queue.c#L376)                    /// ⚙️ 静态创建队列（需 STATIC_ALLOCATION=1）
[vQueueDelete](queue.c#L2257)                          /// ✅ 删除队列
[xQueueSend](queue.c#L949)                            /// ✅ 入队（= xQueueSendToBack）
[xQueueSendToBack](queue.c#L949)                      /// ✅ 从队尾入队
[xQueueSendToFront](queue.c#L949)                     /// ✅ 从队头入队（插队）
[xQueueOverwrite](queue.c#L949)                       /// ✅ 覆盖写（仅长度 1 的队列）
[xQueueReceive](queue.c#L1509)                         /// ✅ 出队（读取并移除）
[xQueuePeek](queue.c#L1884)                            /// ✅ 读取但不移除
[xQueueReset](queue.c#L303)                           /// ✅ 清空队列
[uxQueueMessagesWaiting](queue.c#L2200)                /// ✅ 当前消息数
[uxQueueSpacesAvailable](queue.c#L2220)                /// ✅ 剩余空位
[uxQueueGetQueueItemSize](queue.c#L2348)               /// ✅ 单个 item 字节数
[uxQueueGetQueueLength](queue.c#L2358)                 /// ✅ 队列容量
[pcQueueGetName](queue.c#L3065)                        /// ✅ 取队列名（需 configQUEUE_REGISTRY_SIZE>0）
[vQueueAddToRegistry](queue.c#L3014)                   /// ✅ 加入队列注册表（调试）
[vQueueUnregisterQueue](queue.c#L3100)                 /// ✅ 移出注册表
[xQueueIsQueueEmptyFromISR](queue.c#L2640)             /// ✅ ISR 判空
[xQueueIsQueueFullFromISR](queue.c#L2685)              /// ✅ ISR 判满
[xQueueSendFromISR](queue.c#L1167)                     /// ✅ ISR 入队（队尾）
[xQueueSendToBackFromISR](queue.c#L1167)               /// ✅ ISR 队尾入队
[xQueueSendToFrontFromISR](queue.c#L1167)              /// ✅ ISR 队头入队
[xQueueOverwriteFromISR](queue.c#L1167)                /// ✅ ISR 覆盖写
[xQueueReceiveFromISR](queue.c#L2039)                  /// ✅ ISR 出队
[xQueuePeekFromISR](queue.c#L2139)                     /// ✅ ISR 窥视
[uxQueueMessagesWaitingFromISR](queue.c#L2241)         /// ✅ ISR 取消息数
[xQueueCreateSet](queue.c#L3181)                       /// ✅ 创建队列集
[xQueueCreateSetStatic](queue.c#L3199)                 /// ⚙️ 静态创建队列集
[xQueueAddToSet](queue.c#L3219)                        /// ✅ 把队列加入队列集
[xQueueRemoveFromSet](queue.c#L3266)                   /// ✅ 从队列集移除
[xQueueSelectFromSet](queue.c#L3307)                   /// ✅ 等待集合中任一队列有数据
[xQueueSelectFromSetFromISR](queue.c#L3326)            /// ✅ ISR 版本
[xQueueGetMutexHolder](queue.c#L692)                  /// ✅ 取互斥量持有者
[xQueueGetMutexHolderFromISR](queue.c#L729)           /// ✅ ISR 版本
[uxQueueGetQueueNumber](queue.c#L2307)                 /// ✅ 取队列内部编号（需 TRACE_FACILITY）
[vQueueSetQueueNumber](queue.c#L2321)                  /// ✅ 设置队列编号（需 TRACE_FACILITY）
[xQueueGenericCreate](queue.c#L502)                   /// ➖ 底层通用创建
[xQueueGenericCreateStatic](queue.c#L376)             /// ➖
[xQueueGenericSend](queue.c#L949)                     /// ➖ 底层通用发送
[xQueueGenericSendFromISR](queue.c#L1167)              /// ➖
[xQueueGenericReset](queue.c#L303)                    /// ➖ 底层复位
[xQueueGetStaticBuffers](queue.c#L446)                /// ➖ 取静态缓冲指针
[xQueueCRSend](queue.c#L2711)                          /// ⛔ 协程发送（configUSE_CO_ROUTINES=1）
[xQueueCRReceive](queue.c#L2794)                       /// ⛔ 协程接收
```

### 4. 信号量 / 互斥量（Semaphores / Mutexes） — `include/semphr.h`（底层在 `queue.h`）

```text
[xSemaphoreCreateBinary](queue.c#L502)                /// ✅ 创建二值信号量（初值不可用）
[xSemaphoreCreateBinaryStatic](queue.c#L376)          /// ⚙️ 静态创建二值信号量
[vSemaphoreCreateBinary](queue.c#L502)                /// ⚠️ 旧版宏（已废弃，勿用）
[xSemaphoreCreateCounting](queue.c#L912)              /// ✅ 创建计数信号量
[xSemaphoreCreateCountingStatic](queue.c#L872)        /// ⚙️ 静态创建计数信号量
[xSemaphoreCreateMutex](queue.c#L647)                 /// ✅ 创建互斥量（带优先级继承）
[xSemaphoreCreateMutexStatic](queue.c#L667)           /// ⚙️ 静态创建互斥量
[xSemaphoreCreateRecursiveMutex](queue.c#L647)        /// ✅ 创建递归互斥量
[xSemaphoreCreateRecursiveMutexStatic](queue.c#L667)  /// ⚙️ 静态创建递归互斥量
[vSemaphoreDelete](queue.c#L2257)                      /// ✅ 删除信号量 / 互斥量
[xSemaphoreTake](queue.c#L1659)                        /// ✅ 获取（P 操作，带超时）
[xSemaphoreTakeRecursive](queue.c#L818)               /// ✅ 递归获取
[xSemaphoreTakeFromISR](queue.c#L2039)                 /// ✅ ISR 中获取
[xSemaphoreGive](queue.c#L949)                        /// ✅ 释放（V 操作）
[xSemaphoreGiveRecursive](queue.c#L759)               /// ✅ 递归释放
[xSemaphoreGiveFromISR](queue.c#L1338)                 /// ✅ ISR 中释放
[uxSemaphoreGetCount](queue.c#L2200)                   /// ✅ 计数信号量当前计数（宏）
[uxSemaphoreGetCountFromISR](queue.c#L2241)            /// ✅ ISR 版本
[xSemaphoreGetMutexHolder](queue.c#L692)              /// ✅ 取互斥量持有者任务
[xSemaphoreGetMutexHolderFromISR](queue.c#L729)       /// ✅ ISR 版本
```

### 5. 软件定时器（Timers） — `include/timers.h`（需 `configUSE_TIMERS=1`，当前已开）

```text
[xTimerCreate](timers.c#L336)                          /// ✅ 动态创建软件定时器
[xTimerCreateStatic](timers.c#L370)                    /// ⚙️ 静态创建（需 STATIC_ALLOCATION=1）
[xTimerStart](timers.c#L448)                           /// ✅ 启动（重新开始计时）
[xTimerStartFromISR](timers.c#L448)                    /// ✅ ISR 版本
[xTimerStop](timers.c#L448)                            /// ✅ 停止
[xTimerStopFromISR](timers.c#L448)                     /// ✅ ISR 版本
[xTimerReset](timers.c#L448)                           /// ✅ 复位（重新计时）
[xTimerResetFromISR](timers.c#L448)                    /// ✅ ISR 版本
[xTimerChangePeriod](timers.c#L448)                    /// ✅ 修改周期
[xTimerChangePeriodFromISR](timers.c#L448)             /// ✅ ISR 版本
[xTimerDelete](timers.c#L448)                          /// ✅ 删除定时器
[xTimerIsTimerActive](timers.c#L1167)                   /// ✅ 是否处于活动状态
[xTimerGetPeriod](timers.c#L555)                       /// ✅ 取周期
[xTimerGetExpiryTime](timers.c#L636)                   /// ✅ 取下次到期绝对 tick
[pcTimerGetName](timers.c#L683)                        /// ✅ 取定时器名
[xTimerGetReloadMode](timers.c#L594)                   /// ✅ 取自动重载模式
[vTimerSetReloadMode](timers.c#L569)                   /// ✅ 设置自动重载模式
[pvTimerGetTimerID](timers.c#L1196)                     /// ✅ 取定时器 ID
[vTimerSetTimerID](timers.c#L1217)                      /// ✅ 设置定时器 ID
[xTimerGetTimerDaemonTaskHandle](timers.c#L541)        /// ✅ 取定时器守护任务句柄
[xTimerPendFunctionCall](timers.c#L1268)                /// ✅ 向守护任务投递函数（在守护任务上下文执行）
[xTimerPendFunctionCallFromISR](timers.c#L1238)         /// ✅ ISR 版本
[xTimerCreateTimerTask](timers.c#L237)                 /// ✅ 手动创建守护任务（一般自动创建）
[uxTimerGetTimerNumber](timers.c#L1303)                 /// ✅ 取编号（需 TRACE_FACILITY）
[vTimerSetTimerNumber](timers.c#L1317)                  /// ✅ 设置编号（需 TRACE_FACILITY）
```

### 6. 事件组（Event Groups） — `include/event_groups.h`

```text
[xEventGroupCreate](event_groups.c#L145)                     /// ✅ 动态创建事件组
[xEventGroupCreateStatic](event_groups.c#L86)               /// ⚙️ 静态创建（需 STATIC_ALLOCATION=1）
[vEventGroupDelete](event_groups.c#L650)                     /// ✅ 删除事件组
[xEventGroupSetBits](event_groups.c#L547)                    /// ✅ 置位事件位
[xEventGroupSetBitsFromISR](event_groups.c#L817)             /// ✅ ISR 置位（经守护任务）
[xEventGroupClearBits](event_groups.c#L472)                  /// ✅ 清除事件位
[xEventGroupClearBitsFromISR](event_groups.c#L506)           /// ✅ ISR 清位
[xEventGroupGetBits](event_groups.c#L472)                    /// ✅ 读取当前事件位
[xEventGroupGetBitsFromISR](event_groups.c#L524)             /// ✅ ISR 读取
[xEventGroupWaitBits](event_groups.c#L312)                   /// ✅ 等待满足指定事件位
[xEventGroupSync](event_groups.c#L185)                       /// ✅ 多任务同步（置位 + 等待）
[vEventGroupSetBitsCallback](event_groups.c#L748)            /// ✅ 供 xTimerPendFunctionCall 使用的回调
[vEventGroupClearBitsCallback](event_groups.c#L764)          /// ✅ 供 xTimerPendFunctionCall 使用的回调
[xEventGroupGetStaticBuffer](event_groups.c#L701)            /// ⚙️ 取静态缓冲指针
[uxEventGroupGetNumber](event_groups.c#L838)                 /// ✅ 取事件组编号（需 TRACE_FACILITY）
[vEventGroupSetNumber](event_groups.c#L868)                  /// ✅ 设置编号（需 TRACE_FACILITY）
```

### 7. 流缓冲 / 消息缓冲（Stream / Message Buffers） — `include/stream_buffer.h`、`include/message_buffer.h`

```text
[xStreamBufferCreate](stream_buffer.c#L340)                   /// ✅ 创建字节流缓冲
[xStreamBufferCreateStatic](stream_buffer.c#L436)             /// ⚙️ 静态创建
[xStreamBufferCreateWithCallback](stream_buffer.c#L340)       /// ✅ 创建（带发送/接收完成回调）
[xStreamBufferCreateStaticWithCallback](stream_buffer.c#L436) /// ⚙️ 静态创建（带回调）
[vStreamBufferDelete](stream_buffer.c#L566)                   /// ✅ 删除
[xStreamBufferReset](stream_buffer.c#L603)                    /// ✅ 复位（清空）
[xStreamBufferResetFromISR](stream_buffer.c#L664)             /// ✅ ISR 复位
[xStreamBufferSend](stream_buffer.c#L817)                     /// ✅ 发送字节流（可多字节）
[xStreamBufferSendFromISR](stream_buffer.c#L952)              /// ✅ ISR 发送
[xStreamBufferSendCompletedFromISR](stream_buffer.c#L1410)     /// ✅ ISR 通知“发送已消费”
[xStreamBufferReceive](stream_buffer.c#L1065)                  /// ✅ 接收字节流
[xStreamBufferReceiveFromISR](stream_buffer.c#L1225)           /// ✅ ISR 接收
[xStreamBufferReceiveCompletedFromISR](stream_buffer.c#L1449)  /// ✅ ISR 通知“接收已完成”
[xStreamBufferBytesAvailable](stream_buffer.c#L800)           /// ✅ 可读字节数
[xStreamBufferSpacesAvailable](stream_buffer.c#L763)          /// ✅ 剩余可写字节数
[xStreamBufferIsEmpty](stream_buffer.c#L1343)                  /// ✅ 是否空
[xStreamBufferIsFull](stream_buffer.c#L1371)                   /// ✅ 是否满
[xStreamBufferSetTriggerLevel](stream_buffer.c#L729)          /// ✅ 设置“唤醒接收者”的触发水位
[xStreamBufferNextMessageLengthBytes](stream_buffer.c#L1181)   /// ✅ 下一个消息长度（宏）
[xStreamBatchingBufferCreate](stream_buffer.c#L340)           /// ✅ 创建批处理流缓冲（v11 新特性）
[xStreamBatchingBufferCreateStatic](stream_buffer.c#L436)     /// ⚙️ 静态创建批处理流缓冲
[xStreamBatchingBufferCreateWithCallback](stream_buffer.c#L340) /// ✅
[uxStreamBufferGetStreamBufferNumber](stream_buffer.c#L1712)   /// ✅ 取编号（需 TRACE_FACILITY）
[vStreamBufferSetStreamBufferNumber](stream_buffer.c#L1726)    /// ✅ 设置编号（需 TRACE_FACILITY）
[xMessageBufferCreate](stream_buffer.c#L340)                  /// ✅ 创建消息缓冲
[xMessageBufferCreateStatic](stream_buffer.c#L436)            /// ⚙️ 静态创建
[xMessageBufferCreateWithCallback](stream_buffer.c#L340)      /// ✅ 创建（带回调）
[xMessageBufferCreateStaticWithCallback](stream_buffer.c#L436) /// ⚙️
[vMessageBufferDelete](stream_buffer.c#L566)                  /// ✅ 删除
[xMessageBufferReset](stream_buffer.c#L603)                   /// ✅ 复位
[xMessageBufferResetFromISR](stream_buffer.c#L664)            /// ✅ ISR 复位
[xMessageBufferSend](stream_buffer.c#L817)                    /// ✅ 发送一条消息（变长）
[xMessageBufferSendFromISR](stream_buffer.c#L952)             /// ✅ ISR 发送
[xMessageBufferSendCompletedFromISR](stream_buffer.c#L1410)    /// ✅ ISR 发送完成通知
[xMessageBufferReceive](stream_buffer.c#L1065)                 /// ✅ 接收一条消息
[xMessageBufferReceiveFromISR](stream_buffer.c#L1225)          /// ✅ ISR 接收
[xMessageBufferReceiveCompletedFromISR](stream_buffer.c#L1449) /// ✅ ISR 接收完成通知
[xMessageBufferIsEmpty](stream_buffer.c#L1343)                 /// ✅ 是否空
[xMessageBufferIsFull](stream_buffer.c#L1371)                  /// ✅ 是否满
[xMessageBufferNextLengthBytes](stream_buffer.c#L1181)         /// ✅ 下一条消息长度（宏）
[xMessageBufferSpaceAvailable](stream_buffer.c#L763)          /// ✅ 剩余空间（字节）
[xMessageBufferGetStaticBuffers](stream_buffer.c#L532)        /// ⚙️ 取静态缓冲指针
```

### 8. 任务通知（Task Notifications） — `include/task.h`（需 `configUSE_TASK_NOTIFICATIONS=1`，当前已开）

```text
[xTaskNotify](tasks.c#L8013)                           /// ✅ 发送通知（覆盖/置位/自增可选）
[xTaskNotifyFromISR](tasks.c#L8133)                    /// ✅ ISR 发送通知
[xTaskNotifyAndQuery](tasks.c#L8013)                   /// ✅ 发送并查询接收方原值
[xTaskNotifyAndQueryFromISR](tasks.c#L8133)            /// ✅ ISR 版本
[xTaskNotifyIndexed](tasks.c#L8013)                    /// ✅ 带索引（多通知槽位）版本
[xTaskNotifyIndexedFromISR](tasks.c#L8133)             /// ✅
[xTaskNotifyAndQueryIndexed](tasks.c#L8013)            /// ✅
[xTaskNotifyAndQueryIndexedFromISR](tasks.c#L8133)     /// ✅
[xTaskNotifyWait](tasks.c#L7899)                       /// ✅ 等待通知（可进出时清位）
[xTaskNotifyWaitIndexed](tasks.c#L7899)                /// ✅ 带索引版本
[xTaskNotifyGive](tasks.c#L8013)                       /// ✅ 轻量“计数型”通知（自增）
[vTaskNotifyGiveFromISR](tasks.c#L8315)                /// ✅ ISR 轻量通知
[xTaskNotifyGiveIndexed](tasks.c#L8013)                /// ✅ 带索引版本
[vTaskNotifyGiveIndexedFromISR](tasks.c#L8315)         /// ✅
[ulTaskNotifyTake](tasks.c#L7797)                      /// ✅ 接收计数型通知（可清零）
[ulTaskNotifyTakeIndexed](tasks.c#L7797)               /// ✅ 带索引版本
[xTaskNotifyStateClear](tasks.c#L8447)                 /// ✅ 清除“有通知待取”状态
[xTaskNotifyStateClearIndexed](tasks.c#L8447)          /// ✅ 带索引版本
[ulTaskNotifyValueClear](tasks.c#L8486)                /// ✅ 清除通知值
[ulTaskNotifyValueClearIndexed](tasks.c#L8486)         /// ✅ 带索引版本
[xTaskGenericNotify](tasks.c#L8013)                    /// ➖ 底层通用发送
[xTaskGenericNotifyFromISR](tasks.c#L8133)             /// ➖
[vTaskGenericNotifyGiveFromISR](tasks.c#L8315)         /// ➖
[ulTaskGenericNotifyTake](tasks.c#L7797)               /// ➖
[ulTaskGenericNotifyValueClear](tasks.c#L8486)         /// ➖
```

### 9. 内存管理（Heap） — `portable/MemMang/heap_4.c`

```text
[pvPortMalloc](portable/MemMang/heap_4.c#L173)                          /// ✅ 从 RTOS 堆分配内存
[pvPortCalloc](portable/MemMang/heap_4.c#L437)                          /// ✅ 分配并清零
[vPortFree](portable/MemMang/heap_4.c#L354)                             /// ✅ 释放
[xPortGetFreeHeapSize](portable/MemMang/heap_4.c#L413)                  /// ✅ 当前空闲堆字节数
[xPortGetMinimumEverFreeHeapSize](portable/MemMang/heap_4.c#L419)       /// ✅ 历史最小空闲堆
[vPortGetHeapStats](portable/MemMang/heap_4.c#L572)                     /// ✅ 取堆统计（块数/最大块等）
[vPortInitialiseBlocks](portable/MemMang/heap_4.c#L431)                 /// ✅ 初始化堆块
[vPortHeapResetState](portable/MemMang/heap_4.c#L629)                   /// ✅ 复位堆状态（调试用）
[xPortResetHeapMinimumEverFreeHeapSize](portable/MemMang/heap_4.c#L425) /// ✅ 复位“历史最小空闲堆”记录
[vPortDefineHeapRegions](portable/MemMang/heap_5.c#L173)                /// ⚙️ 定义多内存区（仅 heap_5 用）
```

### 10. 列表（Lists，内核内部） — `include/list.h`

```text
[vListInitialise](list.c#L49)                       /// ➖ 初始化链表头
[vListInitialiseItem](list.c#L89)                   /// ➖ 初始化链表项
[vListInsert](list.c#L139)                           /// ➖ 按值升序插入
[vListInsertEnd](list.c#L105)                        /// ➖ 插入表尾
[uxListRemove](list.c#L217)                          /// ➖ 移除链表项
[listGET_OWNER_OF_NEXT_ENTRY](include/list.h#L286)           /// ➖ 宏：遍历下一个项的宿主
```

> 说明：`INCLUDE_*` 只门控「可选 API」（以任务类为主）；队列 / 信号量 / 互斥量 / 事件组 / 流缓冲等由 `configUSE_*`、`configSUPPORT_*` 控制，完整可用性对照见 `docs/CHANGELOG.md` 中「FreeRTOS 常用 API / 功能补全」一节。
