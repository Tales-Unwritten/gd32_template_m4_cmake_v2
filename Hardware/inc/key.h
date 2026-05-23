#pragma once
#include "gd32f4xx.h"
#include "systick.h"
#include "../Hardware/inc/bsp_led.h"
#include "../Hardware/inc/debug_uart.h"


// //单击 双击 长按 时间定义
// typedef enum
// {
//     key_none = 0,
//     key_single_click,
//     key_double_click,
//     key_long_press
// } key_enum_t;

// typedef struct
// {
//     uint32_t   Key1_Status;
//     uint32_t   Key2_Status;
//     uint8_t    Key1_Count;
//     uint8_t    Key2_Count;
//     key_enum_t Key1_State;
//     key_enum_t Key2_State;
// } key_struct_t;

// #define KEY1_RCU               RCU_GPIOA
// #define KEY1_PORT              GPIOA
// #define KEY1_PIN               GPIO_PIN_0
// #define KEY1_EXTI_PORT_SOURCE  EXTI_SOURCE_GPIOA
// #define KEY1_EXTI_PIN_SOURCE   EXTI_SOURCE_PIN0
// #define KEY1_EXTI_LINE         EXTI_0
// #define KEY1_EXTI_IRQn         EXTI0_IRQn
// #define KEY1_EXTI_IRQHandler   EXTI0_IRQHandler


// #define KEY2_RCU               RCU_GPIOC
// #define KEY2_PORT              GPIOC
// #define KEY2_PIN               GPIO_PIN_13

// #define KEY2_EXTI_PORT_SOURCE  EXTI_SOURCE_GPIOC
// #define KEY2_EXTI_PIN_SOURCE   EXTI_SOURCE_PIN13
// #define KEY2_EXTI_LINE         EXTI_13
// #define KEY2_EXTI_IRQn         EXTI10_15_IRQn
// #define KEY2_EXTI_IRQHandler   EXTI10_15_IRQHandler


// void configure_key_init(void);

// void key_logic_handle(void);



/* ============================================================
 * 文件: key.h / key.c
 * 功能: 工程级按键驱动（定时扫描 + 状态机）
 *
 * 支持功能:
 * 1. 32次连续采样确认按下 / 松开（硬件消抖）
 * 2. 单击
 * 3. 双击（间隔 ≤ 1s，且中间必须稳定松开）
 * 4. 长按成立（默认3s，立即上报一次）
 * 5. 长按连发（长按后周期触发）
 *
 * 状态机流程:
 *
 * IDLE
 *   ↓(32次按下)
 * PRESSED
 *   ↓3s                ↓松开
 * LONG ------------> WAIT_SECOND
 *   ↓周期连发            ↓1s超时 → SINGLE
 *   ↓松开                ↓再次按下 → SECOND_PRESSED → 松开 → DOUBLE
 * IDLE
 *
 * 说明:
 * - 定时器中断只调用 Key_Scan()
 * - 主循环调用 Key_GetEvent() 取事件
 * ============================================================ */

/* ===================== 参数配置区 ===================== */

#define KEY_SCAN_PERIOD_MS           1     // 扫描周期 1ms

#define KEY_FILTER_COUNT             32    // 连续32次确认
#define KEY_RELEASE_COUNT            32

#define KEY_LONG_TIME_MS             3000  // 长按时间阈值
#define KEY_DOUBLE_TIME_MS           100   // 双击时间阈值
#define KEY_LONG_REPEAT_PERIOD_MS    100   // 连发周期

#define KEY_LONG_TIME          (KEY_LONG_TIME_MS / KEY_SCAN_PERIOD_MS)           // 长按时间阈值
#define KEY_DOUBLE_TIME        (KEY_DOUBLE_TIME_MS / KEY_SCAN_PERIOD_MS)         // 双击时间阈值
#define KEY_LONG_REPEAT_PERIOD (KEY_LONG_REPEAT_PERIOD_MS / KEY_SCAN_PERIOD_MS)  // 长按连发周期

/* ===================== 事件定义 ===================== */

typedef enum
{
    KEY_EVENT_NONE = 0,        // 无事件
    KEY_EVENT_SINGLE_CLICK,    // 单击成立
    KEY_EVENT_DOUBLE_CLICK,    // 双击成立
    KEY_EVENT_LONG_PRESS,      // 长按成立（一次）
    KEY_EVENT_LONG_REPEAT      // 长按连发（周期）
} KeyEvent_t;

/* ===================== 状态机定义 ===================== */

typedef enum
{
    KEY_STATE_IDLE = 0,      // 空闲
    KEY_STATE_PRESSED,       // 按下确认
    KEY_STATE_LONG,          // 长按成立
    KEY_STATE_WAIT_SECOND,   // 等待第二次按下
    KEY_STATE_SECOND_PRESSED // 第二次按下确认
} KeyState_t;

/* ===================== 按键对象 ===================== */

typedef struct
{
    uint32_t history;           // 32位历史寄存器（消抖）

    uint16_t press_cnt;         // 按下持续计数
    uint16_t release_cnt;       // 松开确认计数
    uint16_t double_cnt;        // 双击时间窗口
    uint16_t long_repeat_cnt;   // 长按连发计数

    uint8_t  stable_level;      // 当前稳定电平
    uint8_t  long_reported;     // 长按是否已触发

    KeyState_t state;
    KeyEvent_t event;

} Key_t;

/* ===================== 接口函数 ===================== */

void Key_Init(Key_t *key);
void Key_Scan(Key_t *key, uint8_t level);
KeyEvent_t Key_GetEvent(Key_t *key);

/* ===================== BSP接口 ===================== */

void Key_BSP_Init(void);
uint8_t key1_get_status(void);
uint8_t key2_get_status(void);
void example_usage(void);


extern Key_t g_key1;
extern Key_t g_key2;

