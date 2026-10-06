/*
 * freertos_api_index.c — FreeRTOS API 索引（编辑器跳转 + 速查）
 * ---------------------------------------------------------------------------
 * 用法：在编辑器里对下面任意 API 名做「转到定义」（Zed: Cmd/Ctrl+点击），
 *       即可跳到它的声明/定义处（函数 → 头文件原型或 .c 实现；宏 → 宏定义）。
 *       想在某个 API 下补更多说明，直接在 `/// 名字()` 下继续加 /// 行即可。
 *
 * ★ 注释用于速查/跳转；代码引用放在一个从不调用的 static inline 函数内，
 *   编译后零代码/零数据（已用 nm 验证）★
 * 由脚本自动生成（tools/gen_api_index.py，在项目根目录运行），勿手改；改后重跑脚本。
 * 收录范围：code/inc/FreeRTOSConfig.h 当前「已启用(✅)」的公共 API。
 * ---------------------------------------------------------------------------
 */

#include "FreeRTOS.h"        // FreeRTOS 核心头文件：配置、基础类型、宏定义，通常必须第一个包含
#include "task.h"            // 任务管理：任务创建、删除、延时、调度等 API
#include "queue.h"           // 队列：任务与任务/中断之间通信的基础机制
#include "semphr.h"          // 信号量与互斥量：二值信号量、计数信号量、互斥量 API
#include "timers.h"          // 软件定时器：定时器创建、启动、停止、复位等 API
#include "event_groups.h"    // 事件组：多事件同步，按位设置和等待事件
#include "stream_buffer.h"   // 流缓冲区：面向字节流的发送/接收，适合不定长数据
#include "message_buffer.h"  // 消息缓冲区：基于流缓冲区，但带消息长度，适合离散消息

/* 下面这些引用仅为「保证转到定义可跳转」而存在；
 * 本函数从不被调用，故编译后不产生任何代码。 */
static inline void freertos_api_index_( void )
{


/* ══════════════════════════════════════════════════════════════
 *  1. 任务（Tasks） — `include/task.h`
 * ══════════════════════════════════════════════════════════════ */
/// xTaskCreate()                             动态创建任务（自动分配栈 + TCB）
(void) &xTaskCreate;
/// vTaskDelete()                             删除任务（传 NULL 删除当前任务）
(void) &vTaskDelete;
/// vTaskDelay()                              相对阻塞延时（单位 tick）
(void) &vTaskDelay;
/// vTaskDelayUntil()                         绝对周期性延时（旧名，仍兼容）
vTaskDelayUntil(0, 0);
/// xTaskDelayUntil()                         绝对周期性延时（v11 新名）
(void) &xTaskDelayUntil;
/// xTaskAbortDelay()                         强制解除目标任务阻塞
(void) &xTaskAbortDelay;
/// vTaskPrioritySet()                        修改任务优先级
(void) &vTaskPrioritySet;
/// uxTaskPriorityGet()                       读取任务优先级
(void) &uxTaskPriorityGet;
/// uxTaskPriorityGetFromISR()                ISR 中读优先级
(void) &uxTaskPriorityGetFromISR;
/// uxTaskBasePriorityGet()                   读“基础”优先级（不含优先级继承）
(void) &uxTaskBasePriorityGet;
/// uxTaskBasePriorityGetFromISR()            ISR 版本
(void) &uxTaskBasePriorityGetFromISR;
/// vTaskSuspend()                            挂起任务
(void) &vTaskSuspend;
/// vTaskResume()                             恢复任务
(void) &vTaskResume;
/// xTaskResumeFromISR()                      ISR 中恢复任务（返回是否需切换）
(void) &xTaskResumeFromISR;
/// xTaskGetSchedulerState()                  查询调度器状态（未启动/运行/挂起）
(void) &xTaskGetSchedulerState;
/// xTaskGetCurrentTaskHandle()               当前任务句柄
(void) &xTaskGetCurrentTaskHandle;
/// xTaskGetIdleTaskHandle()                  空闲任务句柄
(void) &xTaskGetIdleTaskHandle;
/// xTaskGetHandle()                          按名字查任务句柄
(void) &xTaskGetHandle;
/// eTaskGetState()                           查询任务状态（就绪/阻塞/挂起…）
(void) &eTaskGetState;
/// uxTaskGetNumberOfTasks()                  任务总数
(void) &uxTaskGetNumberOfTasks;
/// pcTaskGetName()                           取任务名
(void) &pcTaskGetName;
/// uxTaskGetStackHighWaterMark()             栈历史最小剩余（word）
(void) &uxTaskGetStackHighWaterMark;
/// uxTaskGetStackHighWaterMark2()            同上（返回 configSTACK_DEPTH_TYPE）
(void) &uxTaskGetStackHighWaterMark2;
/// vTaskGetInfo()                            取任务详情结构体（需 TRACE_FACILITY）
(void) &vTaskGetInfo;
/// vTaskSetApplicationTaskTag()              设置任务标签（调试）
(void) &vTaskSetApplicationTaskTag;
/// xTaskGetApplicationTaskTag()              读任务标签
(void) &xTaskGetApplicationTaskTag;
/// xTaskGetApplicationTaskTagFromISR()       ISR 版本
(void) &xTaskGetApplicationTaskTagFromISR;
/// vTaskSetTimeOutState()                    记录超时起点（配合 xTaskCheckForTimeOut）
(void) &vTaskSetTimeOutState;
/// xTaskCheckForTimeOut()                    判断是否超时（可续时重试）
(void) &xTaskCheckForTimeOut;
/// xTaskGetTickCount()                       当前 tick 计数
(void) &xTaskGetTickCount;
/// xTaskGetTickCountFromISR()                ISR 取 tick
(void) &xTaskGetTickCountFromISR;
/// xTaskCatchUpTicks()                       补偿 tick（tickless 场景）
(void) &xTaskCatchUpTicks;
/// uxTaskGetSystemState()                    取全部任务状态快照（需 TRACE_FACILITY）
(void) &uxTaskGetSystemState;
/// vTaskList()                               打印任务列表（需 TRACE + STATS_FORMATTING）
vTaskList(0);

/* ══════════════════════════════════════════════════════════════
 *  2. 内核 / 调度器控制（Kernel Control） — `include/task.h`
 * ══════════════════════════════════════════════════════════════ */

/// vTaskStartScheduler()                     启动调度器（正常不返回）
(void) &vTaskStartScheduler;
/// vTaskEndScheduler()                       停止调度器
(void) &vTaskEndScheduler;
/// vTaskSuspendAll()                         挂起调度器（不关中断，可嵌套）
(void) &vTaskSuspendAll;
/// xTaskResumeAll()                          恢复调度器（返回期间是否有切换）
(void) &xTaskResumeAll;
/// taskENTER_CRITICAL()                      进入临界区（宏：关中断至 BASEPRI）
taskENTER_CRITICAL();
/// taskEXIT_CRITICAL()                       退出临界区
taskEXIT_CRITICAL();
/// taskENTER_CRITICAL_FROM_ISR()             ISR 进入临界区
taskENTER_CRITICAL_FROM_ISR();
/// taskEXIT_CRITICAL_FROM_ISR()              ISR 退出临界区
taskEXIT_CRITICAL_FROM_ISR(0);
/// taskDISABLE_INTERRUPTS()                  全局关中断
taskDISABLE_INTERRUPTS();
/// taskENABLE_INTERRUPTS()                   全局开中断
taskENABLE_INTERRUPTS();
/// taskYIELD()                               请求任务切换（触发 PendSV）
taskYIELD();
/// portYIELD_FROM_ISR()                      ISR 内请求切换（底层宏）
portYIELD_FROM_ISR(0);
/// portYIELD()                               port 层触发 PendSV 的宏
portYIELD();

/* ══════════════════════════════════════════════════════════════
 *  3. 队列（Queues） — `include/queue.h`
 * ══════════════════════════════════════════════════════════════ */

/// xQueueCreate()                            动态创建队列
xQueueCreate(0, 0);
/// vQueueDelete()                            删除队列
(void) &vQueueDelete;
/// xQueueSend()                              入队（= xQueueSendToBack）
xQueueSend(0, 0, 0);
/// xQueueSendToBack()                        从队尾入队
xQueueSendToBack(0, 0, 0);
/// xQueueSendToFront()                       从队头入队（插队）
xQueueSendToFront(0, 0, 0);
/// xQueueOverwrite()                         覆盖写（仅长度 1 的队列）
xQueueOverwrite(0, 0);
/// xQueueReceive()                           出队（读取并移除）
(void) &xQueueReceive;
/// xQueuePeek()                              读取但不移除
(void) &xQueuePeek;
/// xQueueReset()                             清空队列
xQueueReset(0);
/// uxQueueMessagesWaiting()                  当前消息数
(void) &uxQueueMessagesWaiting;
/// uxQueueSpacesAvailable()                  剩余空位
(void) &uxQueueSpacesAvailable;
/// uxQueueGetQueueItemSize()                 单个 item 字节数
(void) &uxQueueGetQueueItemSize;
/// uxQueueGetQueueLength()                   队列容量
(void) &uxQueueGetQueueLength;
/// pcQueueGetName()                          取队列名（需 configQUEUE_REGISTRY_SIZE>0）
(void) &pcQueueGetName;
/// vQueueAddToRegistry()                     加入队列注册表（调试）
(void) &vQueueAddToRegistry;
/// vQueueUnregisterQueue()                   移出注册表
(void) &vQueueUnregisterQueue;
/// xQueueIsQueueEmptyFromISR()               ISR 判空
(void) &xQueueIsQueueEmptyFromISR;
/// xQueueIsQueueFullFromISR()                ISR 判满
(void) &xQueueIsQueueFullFromISR;
/// xQueueSendFromISR()                       ISR 入队（队尾）
xQueueSendFromISR(0, 0, 0);
/// xQueueSendToBackFromISR()                 ISR 队尾入队
xQueueSendToBackFromISR(0, 0, 0);
/// xQueueSendToFrontFromISR()                ISR 队头入队
xQueueSendToFrontFromISR(0, 0, 0);
/// xQueueOverwriteFromISR()                  ISR 覆盖写
xQueueOverwriteFromISR(0, 0, 0);
/// xQueueReceiveFromISR()                    ISR 出队
(void) &xQueueReceiveFromISR;
/// xQueuePeekFromISR()                       ISR 窥视
(void) &xQueuePeekFromISR;
/// uxQueueMessagesWaitingFromISR()           ISR 取消息数
(void) &uxQueueMessagesWaitingFromISR;
/// xQueueCreateSet()                         创建队列集
(void) &xQueueCreateSet;
/// xQueueAddToSet()                          把队列加入队列集
(void) &xQueueAddToSet;
/// xQueueRemoveFromSet()                     从队列集移除
(void) &xQueueRemoveFromSet;
/// xQueueSelectFromSet()                     等待集合中任一队列有数据
(void) &xQueueSelectFromSet;
/// xQueueSelectFromSetFromISR()              ISR 版本
(void) &xQueueSelectFromSetFromISR;
/// xQueueGetMutexHolder()                    取互斥量持有者
(void) &xQueueGetMutexHolder;
/// xQueueGetMutexHolderFromISR()             ISR 版本
(void) &xQueueGetMutexHolderFromISR;
/// uxQueueGetQueueNumber()                   取队列内部编号（需 TRACE_FACILITY）
(void) &uxQueueGetQueueNumber;
/// vQueueSetQueueNumber()                    设置队列编号（需 TRACE_FACILITY）
(void) &vQueueSetQueueNumber;

/* ══════════════════════════════════════════════════════════════
 *  4. 信号量 / 互斥量（Semaphores / Mutexes） — `include/semphr.h`（底层在 `queue.h`）
 * ══════════════════════════════════════════════════════════════ */

/// xSemaphoreCreateBinary()                  创建二值信号量（初值不可用）
xSemaphoreCreateBinary();
/// xSemaphoreCreateCounting()                创建计数信号量
xSemaphoreCreateCounting(0, 0);
/// xSemaphoreCreateMutex()                   创建互斥量（带优先级继承）
xSemaphoreCreateMutex();
/// xSemaphoreCreateRecursiveMutex()          创建递归互斥量
xSemaphoreCreateRecursiveMutex();
/// vSemaphoreDelete()                        删除信号量 / 互斥量
vSemaphoreDelete(0);
/// xSemaphoreTake()                          获取（P 操作，带超时）
xSemaphoreTake(0, 0);
/// xSemaphoreTakeRecursive()                 递归获取
xSemaphoreTakeRecursive(0, 0);
/// xSemaphoreTakeFromISR()                   ISR 中获取
xSemaphoreTakeFromISR(0, 0);
/// xSemaphoreGive()                          释放（V 操作）
xSemaphoreGive(0);
/// xSemaphoreGiveRecursive()                 递归释放
xSemaphoreGiveRecursive(0);
/// xSemaphoreGiveFromISR()                   ISR 中释放
xSemaphoreGiveFromISR(0, 0);
/// uxSemaphoreGetCount()                     计数信号量当前计数（宏）
uxSemaphoreGetCount(0);
/// uxSemaphoreGetCountFromISR()              ISR 版本
uxSemaphoreGetCountFromISR(0);
/// xSemaphoreGetMutexHolder()                取互斥量持有者任务
xSemaphoreGetMutexHolder(0);
/// xSemaphoreGetMutexHolderFromISR()         ISR 版本
xSemaphoreGetMutexHolderFromISR(0);

/* ══════════════════════════════════════════════════════════════
 *  5. 软件定时器（Timers） — `include/timers.h`（需 `configUSE_TIMERS=1`，当前已开）
 * ══════════════════════════════════════════════════════════════ */

/// xTimerCreate()                            动态创建软件定时器
(void) &xTimerCreate;
/// xTimerStart()                             启动（重新开始计时）
xTimerStart(0, 0);
/// xTimerStartFromISR()                      ISR 版本
xTimerStartFromISR(0, 0);
/// xTimerStop()                              停止
xTimerStop(0, 0);
/// xTimerStopFromISR()                       ISR 版本
xTimerStopFromISR(0, 0);
/// xTimerReset()                             复位（重新计时）
xTimerReset(0, 0);
/// xTimerResetFromISR()                      ISR 版本
xTimerResetFromISR(0, 0);
/// xTimerChangePeriod()                      修改周期
xTimerChangePeriod(0, 0, 0);
/// xTimerChangePeriodFromISR()               ISR 版本
xTimerChangePeriodFromISR(0, 0, 0);
/// xTimerDelete()                            删除定时器
xTimerDelete(0, 0);
/// xTimerIsTimerActive()                     是否处于活动状态
(void) &xTimerIsTimerActive;
/// xTimerGetPeriod()                         取周期
(void) &xTimerGetPeriod;
/// xTimerGetExpiryTime()                     取下次到期绝对 tick
(void) &xTimerGetExpiryTime;
/// pcTimerGetName()                          取定时器名
(void) &pcTimerGetName;
/// xTimerGetReloadMode()                     取自动重载模式
(void) &xTimerGetReloadMode;
/// vTimerSetReloadMode()                     设置自动重载模式
(void) &vTimerSetReloadMode;
/// pvTimerGetTimerID()                       取定时器 ID
(void) &pvTimerGetTimerID;
/// vTimerSetTimerID()                        设置定时器 ID
(void) &vTimerSetTimerID;
/// xTimerGetTimerDaemonTaskHandle()          取定时器守护任务句柄
(void) &xTimerGetTimerDaemonTaskHandle;
/// xTimerPendFunctionCall()                  向守护任务投递函数（在守护任务上下文执行）
(void) &xTimerPendFunctionCall;
/// xTimerPendFunctionCallFromISR()           ISR 版本
(void) &xTimerPendFunctionCallFromISR;
/// xTimerCreateTimerTask()                   手动创建守护任务（一般自动创建）
(void) &xTimerCreateTimerTask;
/// uxTimerGetTimerNumber()                   取编号（需 TRACE_FACILITY）
(void) &uxTimerGetTimerNumber;
/// vTimerSetTimerNumber()                    设置编号（需 TRACE_FACILITY）
(void) &vTimerSetTimerNumber;

/* ══════════════════════════════════════════════════════════════
 *  6. 事件组（Event Groups） — `include/event_groups.h`
 * ══════════════════════════════════════════════════════════════ */

/// xEventGroupCreate()                       动态创建事件组
(void) &xEventGroupCreate;
/// vEventGroupDelete()                       删除事件组
(void) &vEventGroupDelete;
/// xEventGroupSetBits()                      置位事件位
(void) &xEventGroupSetBits;
/// xEventGroupSetBitsFromISR()               ISR 置位（经守护任务）
(void) &xEventGroupSetBitsFromISR;
/// xEventGroupClearBits()                    清除事件位
(void) &xEventGroupClearBits;
/// xEventGroupClearBitsFromISR()             ISR 清位
(void) &xEventGroupClearBitsFromISR;
/// xEventGroupGetBits()                      读取当前事件位
xEventGroupGetBits(0);
/// xEventGroupGetBitsFromISR()               ISR 读取
(void) &xEventGroupGetBitsFromISR;
/// xEventGroupWaitBits()                     等待满足指定事件位
(void) &xEventGroupWaitBits;
/// xEventGroupSync()                         多任务同步（置位 + 等待）
(void) &xEventGroupSync;
/// vEventGroupSetBitsCallback()              供 xTimerPendFunctionCall 使用的回调
(void) &vEventGroupSetBitsCallback;
/// vEventGroupClearBitsCallback()            供 xTimerPendFunctionCall 使用的回调
(void) &vEventGroupClearBitsCallback;
/// uxEventGroupGetNumber()                   取事件组编号（需 TRACE_FACILITY）
(void) &uxEventGroupGetNumber;
/// vEventGroupSetNumber()                    设置编号（需 TRACE_FACILITY）
(void) &vEventGroupSetNumber;
/* ══════════════════════════════════════════════════════════════
 *  7. 流缓冲 / 消息缓冲（Stream / Message Buffers） — `include/stream_buffer.h`、`include/message_buffer.h`
 * ══════════════════════════════════════════════════════════════ */
/// xStreamBufferCreate()                     创建字节流缓冲
xStreamBufferCreate(0, 0);
/// xStreamBufferCreateWithCallback()         创建（带发送/接收完成回调）
xStreamBufferCreateWithCallback(0, 0, 0, 0);
/// vStreamBufferDelete()                     删除
(void) &vStreamBufferDelete;
/// xStreamBufferReset()                      复位（清空）
(void) &xStreamBufferReset;
/// xStreamBufferResetFromISR()               ISR 复位
(void) &xStreamBufferResetFromISR;
/// xStreamBufferSend()                       发送字节流（可多字节）
(void) &xStreamBufferSend;
/// xStreamBufferSendFromISR()                ISR 发送
(void) &xStreamBufferSendFromISR;
/// xStreamBufferSendCompletedFromISR()       ISR 通知“发送已消费”
(void) &xStreamBufferSendCompletedFromISR;
/// xStreamBufferReceive()                    接收字节流
(void) &xStreamBufferReceive;
/// xStreamBufferReceiveFromISR()             ISR 接收
(void) &xStreamBufferReceiveFromISR;
/// xStreamBufferReceiveCompletedFromISR()    ISR 通知“接收已完成”
(void) &xStreamBufferReceiveCompletedFromISR;
/// xStreamBufferBytesAvailable()             可读字节数
(void) &xStreamBufferBytesAvailable;
/// xStreamBufferSpacesAvailable()            剩余可写字节数
(void) &xStreamBufferSpacesAvailable;
/// xStreamBufferIsEmpty()                    是否空
(void) &xStreamBufferIsEmpty;
/// xStreamBufferIsFull()                     是否满
(void) &xStreamBufferIsFull;
/// xStreamBufferSetTriggerLevel()            设置“唤醒接收者”的触发水位
(void) &xStreamBufferSetTriggerLevel;
/// xStreamBufferNextMessageLengthBytes()     下一个消息长度（宏）
(void) &xStreamBufferNextMessageLengthBytes;
/// xStreamBatchingBufferCreate()             创建批处理流缓冲（v11 新特性）
xStreamBatchingBufferCreate(0, 0);
/// xStreamBatchingBufferCreateWithCallback() 创建批处理流缓冲（带回调，v11 新特性）
xStreamBatchingBufferCreateWithCallback(0, 0, 0, 0);
/// uxStreamBufferGetStreamBufferNumber()     取编号（需 TRACE_FACILITY）
(void) &uxStreamBufferGetStreamBufferNumber;
/// vStreamBufferSetStreamBufferNumber()      设置编号（需 TRACE_FACILITY）
(void) &vStreamBufferSetStreamBufferNumber;
/// xMessageBufferCreate()                    创建消息缓冲
xMessageBufferCreate(0);
/// xMessageBufferCreateWithCallback()        创建（带回调）
xMessageBufferCreateWithCallback(0, 0, 0);
/// vMessageBufferDelete()                    删除
vMessageBufferDelete(0);
/// xMessageBufferReset()                     复位
xMessageBufferReset(0);
/// xMessageBufferResetFromISR()              ISR 复位
xMessageBufferResetFromISR(0);
/// xMessageBufferSend()                      发送一条消息（变长）
xMessageBufferSend(0, 0, 0, 0);
/// xMessageBufferSendFromISR()               ISR 发送
xMessageBufferSendFromISR(0, 0, 0, 0);
/// xMessageBufferSendCompletedFromISR()      ISR 发送完成通知
xMessageBufferSendCompletedFromISR(0, 0);
/// xMessageBufferReceive()                   接收一条消息
xMessageBufferReceive(0, 0, 0, 0);
/// xMessageBufferReceiveFromISR()            ISR 接收
xMessageBufferReceiveFromISR(0, 0, 0, 0);
/// xMessageBufferReceiveCompletedFromISR()   ISR 接收完成通知
xMessageBufferReceiveCompletedFromISR(0, 0);
/// xMessageBufferIsEmpty()                   是否空
xMessageBufferIsEmpty(0);
/// xMessageBufferIsFull()                    是否满
xMessageBufferIsFull(0);
/// xMessageBufferNextLengthBytes()           下一条消息长度（宏）
xMessageBufferNextLengthBytes(0);
/// xMessageBufferSpaceAvailable()            剩余空间（字节）
xMessageBufferSpaceAvailable(0);

/* ══════════════════════════════════════════════════════════════
 *  8. 任务通知（Task Notifications） — `include/task.h`（需 `configUSE_TASK_NOTIFICATIONS=1`，当前已开）
 * ══════════════════════════════════════════════════════════════ */

/// xTaskNotify()                             发送通知（覆盖/置位/自增可选）
xTaskNotify(0, 0, 0);
/// xTaskNotifyFromISR()                      ISR 发送通知
xTaskNotifyFromISR(0, 0, 0, 0);
/// xTaskNotifyAndQuery()                     发送并查询接收方原值
xTaskNotifyAndQuery(0, 0, 0, 0);
/// xTaskNotifyAndQueryFromISR()              ISR 版本
xTaskNotifyAndQueryFromISR(0, 0, 0, 0, 0);
/// xTaskNotifyIndexed()                      带索引（多通知槽位）版本
xTaskNotifyIndexed(0, 0, 0, 0);
/// xTaskNotifyIndexedFromISR()               带索引 ISR 发送通知
xTaskNotifyIndexedFromISR(0, 0, 0, 0, 0);
/// xTaskNotifyAndQueryIndexed()              带索引发送并查询接收方原值
xTaskNotifyAndQueryIndexed(0, 0, 0, 0, 0);
/// xTaskNotifyAndQueryIndexedFromISR()       带索引 ISR 发送并查询
xTaskNotifyAndQueryIndexedFromISR(0, 0, 0, 0, 0, 0);
/// xTaskNotifyWait()                         等待通知（可进出时清位）
xTaskNotifyWait(0, 0, 0, 0);
/// xTaskNotifyWaitIndexed()                  带索引等待通知
xTaskNotifyWaitIndexed(0, 0, 0, 0, 0);
/// xTaskNotifyGive()                         轻量“计数型”通知（自增）
xTaskNotifyGive(0);
/// vTaskNotifyGiveFromISR()                  ISR 轻量通知
vTaskNotifyGiveFromISR(0, 0);
/// xTaskNotifyGiveIndexed()                  带索引轻量“计数型”通知
xTaskNotifyGiveIndexed(0, 0);
/// vTaskNotifyGiveIndexedFromISR()           带索引 ISR 轻量通知
vTaskNotifyGiveIndexedFromISR(0, 0, 0);
/// ulTaskNotifyTake()                        接收计数型通知（可清零）
ulTaskNotifyTake(0, 0);
/// ulTaskNotifyTakeIndexed()                 带索引接收计数型通知
ulTaskNotifyTakeIndexed(0, 0, 0);
/// xTaskNotifyStateClear()                   清除“有通知待取”状态
xTaskNotifyStateClear(0);
/// xTaskNotifyStateClearIndexed()            带索引清除通知状态
xTaskNotifyStateClearIndexed(0, 0);
/// ulTaskNotifyValueClear()                  清除通知值
ulTaskNotifyValueClear(0, 0);
/// ulTaskNotifyValueClearIndexed()           带索引清除通知值
ulTaskNotifyValueClearIndexed(0, 0, 0);

/* ══════════════════════════════════════════════════════════════
 *  9. 内存管理（Heap） — `portable/MemMang/heap_4.c`
 * ══════════════════════════════════════════════════════════════ */

/// pvPortMalloc()                            从 RTOS 堆分配内存
(void) &pvPortMalloc;
/// pvPortCalloc()                            分配并清零
(void) &pvPortCalloc;
/// vPortFree()                               释放
(void) &vPortFree;
/// xPortGetFreeHeapSize()                    当前空闲堆字节数
(void) &xPortGetFreeHeapSize;
/// xPortGetMinimumEverFreeHeapSize()         历史最小空闲堆
(void) &xPortGetMinimumEverFreeHeapSize;
/// vPortGetHeapStats()                       取堆统计（块数/最大块等）
(void) &vPortGetHeapStats;
/// vPortInitialiseBlocks()                   初始化堆块
(void) &vPortInitialiseBlocks;
/// vPortHeapResetState()                     复位堆状态（调试用）
(void) &vPortHeapResetState;
/// xPortResetHeapMinimumEverFreeHeapSize()   复位“历史最小空闲堆”记录
(void) &xPortResetHeapMinimumEverFreeHeapSize;

}

// ============================================================================
// FreeRTOS 常用 API 速查（按使用频率）
// ============================================================================

// ----------------------------------------------------------------------------
// 1. 任务与调度
// ----------------------------------------------------------------------------
// xTaskCreate()                                                         动态创建任务，自动分配栈和 TCB                    ★★★
// vTaskDelay()                                                          相对延时，单位 tick，通常配合 pdMS_TO_TICKS(ms)  ★★★
// xTaskDelayUntil() / vTaskDelayUntil()                                 固定周期延时，周期任务首选                        ★★
// vTaskDelete()                                                         删除任务，传 NULL 删除当前任务                   ★★
// vTaskStartScheduler()                                                 启动调度器，正常不返回                          ★★★
// xTaskGetTickCount()                                                   获取当前 tick                            ★★
// xTaskGetCurrentTaskHandle()                                           获取当前任务句柄                             ★★
// vTaskSuspend() / vTaskResume()                                        挂起 / 恢复任务                            ★
// vTaskPrioritySet() / uxTaskPriorityGet()                              设置 / 读取任务优先级                         ★
// uxTaskGetStackHighWaterMark()                                         查看任务栈历史最小剩余，调试常用                     ★★

// ----------------------------------------------------------------------------
// 2. 临界区与调度器控制
// ----------------------------------------------------------------------------
// taskENTER_CRITICAL() / taskEXIT_CRITICAL()                            进入 / 退出临界区，保护短小关键代码                  ★★★
// vTaskSuspendAll() / xTaskResumeAll()                                  挂起 / 恢复调度器，可嵌套，不关中断                  ★★
// taskYIELD()                                                           主动请求任务切换                             ★★
// portYIELD_FROM_ISR()                                                  ISR 中请求切换，常配合 FromISR 使用             ★★★

// ----------------------------------------------------------------------------
// 3. 队列
// ----------------------------------------------------------------------------
// xQueueCreate()                                                        动态创建队列                               ★★★
// xQueueSend() / xQueueSendToBack()                                     从队尾入队                                ★★★
// xQueueReceive()                                                       出队，读取并移除                             ★★★
// xQueueSendFromISR()                                                   ISR 中入队                              ★★★
// xQueueReceiveFromISR()                                                ISR 中出队                              ★★
// uxQueueMessagesWaiting()                                              当前队列消息数                              ★★
// xQueueReset()                                                         清空队列                                 ★
// xQueuePeek()                                                          读取但不移除                               ★

// ----------------------------------------------------------------------------
// 4. 信号量 / 互斥量
// ----------------------------------------------------------------------------
// xSemaphoreCreateBinary()                                              创建二值信号量，初始为空，需先 Give                 ★★★
// xSemaphoreCreateCounting()                                            创建计数信号量                              ★★
// xSemaphoreCreateMutex()                                               创建互斥量，带优先级继承，保护共享资源                  ★★★
// xSemaphoreTake()                                                      获取信号量 / 互斥量，带超时                      ★★★
// xSemaphoreGive()                                                      释放信号量 / 互斥量                          ★★★
// xSemaphoreGiveFromISR()                                               ISR 中释放信号量                           ★★★
// xSemaphoreTakeFromISR()                                               ISR 中获取信号量                           ★★
// xSemaphoreCreateRecursiveMutex() / TakeRecursive() / GiveRecursive()  递归互斥量，同任务可多次获取                       ★

// ----------------------------------------------------------------------------
// 5. 事件组
// ----------------------------------------------------------------------------
// xEventGroupCreate()                                                   动态创建事件组                              ★★★
// xEventGroupSetBits()                                                  置位事件位                                ★★★
// xEventGroupWaitBits()                                                 等待一个或多个事件位满足                         ★★★
// xEventGroupClearBits()                                                清除事件位                                ★★
// xEventGroupSetBitsFromISR()                                           ISR 中置位，经守护任务                        ★★

// ----------------------------------------------------------------------------
// 6. 任务通知
// ----------------------------------------------------------------------------
// xTaskNotifyGive()                                                     轻量计数型通知，自增                           ★★★
// ulTaskNotifyTake()                                                    接收计数型通知，可清零                          ★★★
// vTaskNotifyGiveFromISR()                                              ISR 中轻量通知                            ★★★
// xTaskNotify()                                                         发送通知，可覆盖 / 置位 / 自增                   ★★
// xTaskNotifyWait()                                                     等待通知，可进出时清位                          ★★
// xTaskNotifyFromISR()                                                  ISR 中发送通知                            ★★
// xTaskNotifyStateClear()                                               清除“有通知待取”状态                          ★

// ----------------------------------------------------------------------------
// 7. 软件定时器
// ----------------------------------------------------------------------------
// xTimerCreate()                                                        动态创建软件定时器                            ★★★
// xTimerStart()                                                         启动 / 重新开始计时                          ★★★
// xTimerStop()                                                          停止定时器                                ★★
// xTimerReset()                                                         复位，重新计时                              ★★
// xTimerChangePeriod()                                                  修改周期                                 ★★
// xTimerStartFromISR()                                                  ISR 中启动定时器                           ★★
// pvTimerGetTimerID() / vTimerSetTimerID()                              读写定时器 ID                             ★★

// ----------------------------------------------------------------------------
// 8. 流缓冲 / 消息缓冲
// ----------------------------------------------------------------------------
// xMessageBufferCreate()                                                创建消息缓冲，适合变长消息                        ★★
// xMessageBufferSend() / xMessageBufferReceive()                        发送 / 接收一条消息                          ★★
// xStreamBufferCreate()                                                 创建字节流缓冲                              ★★
// xStreamBufferSend() / xStreamBufferReceive()                          发送 / 接收字节流                           ★★

// ----------------------------------------------------------------------------
// 9. 内存管理
// ----------------------------------------------------------------------------
// pvPortMalloc()                                                        从 RTOS 堆分配内存                         ★★★
// vPortFree()                                                           释放 RTOS 堆内存                          ★★★
// xPortGetFreeHeapSize()                                                当前空闲堆大小                              ★★
// xPortGetMinimumEverFreeHeapSize()                                     历史最小空闲堆，调试内存                         ★★

// ----------------------------------------------------------------------------
// 10. 常用宏
// ----------------------------------------------------------------------------
// pdMS_TO_TICKS(ms)                                                     毫秒转 tick，延时 / 超时常用
// portMAX_DELAY                                                         永久等待
// pdTRUE / pdFALSE                                                      布尔值
// pdPASS / pdFAIL                                                       返回状态

// ============================================================================
// 快速上手顺序
// ============================================================================
// 任务：xTaskCreate() -> vTaskDelay() -> vTaskStartScheduler()
// 队列：xQueueCreate() -> xQueueSend() -> xQueueReceive() -> xQueueSendFromISR()
// 信号量：xSemaphoreCreateBinary() / xSemaphoreCreateMutex() -> xSemaphoreTake() -> xSemaphoreGive() -> xSemaphoreGiveFromISR()
// 事件组 / 任务通知：xEventGroupSetBits() / xEventGroupWaitBits()；xTaskNotifyGive() / ulTaskNotifyTake() / vTaskNotifyGiveFromISR()
// 定时器：xTimerCreate() -> xTimerStart() -> xTimerStop()
// 内存与临界区：pvPortMalloc() / vPortFree()；taskENTER_CRITICAL() / taskEXIT_CRITICAL()