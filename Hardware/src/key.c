#include "../Hardware/inc/key.h"

 /**
  * @brief       基本定时器TIMER6定时中断初始化
  * @note
  *              基本定时器的时钟来自APB1,当PPRE1 ≥ 2分频的时候
  *              基本定时器的时钟为APB1时钟的2倍, 而APB1为60M, 所以定时器TIMER6时钟 = 120Mhz
  *              定时器溢出时间计算方法: Tout = ((arr + 1) * (psc + 1)) / Ft us.
  *              Ft=定时器工作频率,单位:Mhz
  *
  * @param       arr: 自动重装值
  * @param       psc: 时钟预分频数
  * @retval      无
  **/

/* ===================== 按键对象 ===================== */

Key_t g_key1;
Key_t g_key2;

/* ===================== 底层初始化 ===================== */

static void timer6_int_init(uint16_t arr, uint16_t psc)
{
    timer_parameter_struct timer_initpara;

    rcu_periph_clock_enable(RCU_TIMER6);
    timer_deinit(TIMER6);
    timer_struct_para_init(&timer_initpara);

    timer_initpara.prescaler = psc;
    timer_initpara.counterdirection = TIMER_COUNTER_UP;
    timer_initpara.period = arr;
    timer_initpara.clockdivision = TIMER_CKDIV_DIV1;

    timer_init(TIMER6, &timer_initpara);

    timer_interrupt_enable(TIMER6, TIMER_INT_UP);
    nvic_irq_enable(TIMER6_IRQn, 1, 3);
    timer_enable(TIMER6);
}

static void Key_GPIO_Init(void)
{
    rcu_periph_clock_enable(RCU_GPIOA);
    rcu_periph_clock_enable(RCU_GPIOC);

    gpio_mode_set(GPIOA, GPIO_MODE_INPUT, GPIO_PUPD_PULLDOWN, GPIO_PIN_0);
    gpio_mode_set(GPIOC, GPIO_MODE_INPUT, GPIO_PUPD_PULLUP,   GPIO_PIN_13);
}



/* ===================== 硬件读取 ===================== */

uint8_t key1_get_status(void)
{
    return gpio_input_bit_get(GPIOA, GPIO_PIN_0);
}

uint8_t key2_get_status(void)
{
    return !gpio_input_bit_get(GPIOC, GPIO_PIN_13);
}

/* ===================== 驱动核心 ===================== */

void Key_Init(Key_t *key)
{
    key->history = 0;
    key->press_cnt = 0;
    key->release_cnt = 0;
    key->double_cnt = 0;
    key->long_repeat_cnt = 0;
    key->stable_level = 0;
    key->long_reported = 0;
    key->state = KEY_STATE_IDLE;
    key->event = KEY_EVENT_NONE;
}

/* 1ms 调用一次 */
void Key_Scan(Key_t *key, uint8_t level)
{
	key->event = KEY_EVENT_NONE;                                   /* 初始化事件为无事件 */
	// key->history = (key->history << 1) | (level & 0x01);        /* 移位保存按键历史状态 */
	key->history = (key->history << 1) | level;                    /* 移位保存按键历史状态 */
	key->history &= 0xFFFFFFFF;                                    /* 限制历史状态在有效范围内 */
	

	if (key->history == 0xFFFFFFFF) key->stable_level = 1;       /* 按键按下稳定 */
	if (key->history == 0x00000000) key->stable_level = 0;       /* 按键释放稳定 */

	switch (key->state)
	{
	case KEY_STATE_IDLE:                                         /* 空闲状态 */
		if (key->history == 0xFFFFFFFF)
		{
			key->state = KEY_STATE_PRESSED;                      /* 转换为按下状态 */
			key->press_cnt = 0;                                  /* 重置按下计时器 */
			key->long_reported = 0;                              /* 重置长按标志 */
		}
		break;

	case KEY_STATE_PRESSED:                                      /* 按下状态 */
		key->press_cnt++;                                        /* 按下计数增加 */

		if (key->press_cnt >= KEY_LONG_TIME && !key->long_reported)
		{
			key->state = KEY_STATE_LONG;                         /* 转换为长按状态 */
			key->event = KEY_EVENT_LONG_PRESS;                   /* 设置长按事件 */
			key->long_reported   = 1;                            /* 标记长按已上报 */
			key->long_repeat_cnt = 0;                            /* 重置长按重复计数 */
		}

		if (key->history == 0x00000000)
		{
			key->state = KEY_STATE_WAIT_SECOND;                  /* 转换为等待第二次按下状态 */
			key->double_cnt  = 0;                                /* 重置双击计时器 */
			key->release_cnt = 0;                                /* 重置释放计数 */
		}
		break;

	case KEY_STATE_LONG:                                         /* 长按状态 */
		key->long_repeat_cnt++;                                  /* 长按重复计数增加 */

		if (key->long_repeat_cnt >= KEY_LONG_REPEAT_PERIOD)
		{
			key->long_repeat_cnt = 0;                            /* 重置长按重复计数 */
			key->event = KEY_EVENT_LONG_REPEAT;                  /* 设置长按重复事件 */
		}

		if (key->history == 0x00000000)
		{
			key->state = KEY_STATE_IDLE;                         /* 转换为空闲状态 */
			key->long_repeat_cnt = 0;                            /* 重置长按重复计数 */
		}
		break;

	case KEY_STATE_WAIT_SECOND:                                  /* 等待第二次按下状态 */
		key->double_cnt++;                                       /* 双击计时器增加 */

		if (key->history == 0x00000000)
			key->release_cnt++;                                  /* 释放计数增加 */

		if (key->double_cnt >= KEY_DOUBLE_TIME)
		{
			key->state = KEY_STATE_IDLE;                         /* 转换为空闲状态 */
			key->event = KEY_EVENT_SINGLE_CLICK;                 /* 设置单击事件 */
		}

		if (key->history == 0xFFFFFFFF &&
			key->release_cnt >= KEY_RELEASE_COUNT)
		{
			key->state = KEY_STATE_SECOND_PRESSED;               /* 转换为第二次按下状态 */
		}
		break;

	case KEY_STATE_SECOND_PRESSED:                               /* 第二次按下状态 */
		if (key->history == 0x00000000)
		{
			key->state = KEY_STATE_IDLE;                         /* 转换为空闲状态 */
			key->event = KEY_EVENT_DOUBLE_CLICK;                 /* 设置双击事件 */
		}
		break;
	}
}

KeyEvent_t Key_GetEvent(Key_t *key)
{
    KeyEvent_t evt = key->event;
    key->event = KEY_EVENT_NONE;
    return evt;
}

/* ===================== 初始化接口 ===================== */

static void Key_TimerInit(void)
{
    timer6_int_init(1000 - 1, 120 - 1);   // 1ms
}

void Key_BSP_Init(void)
{
    Key_GPIO_Init();
    Key_Init(&g_key1);
    Key_Init(&g_key2);
	Key_TimerInit();
}

/* ===================== 定时器中断 ===================== */

void TIMER6_IRQHandler(void)
{
    if (timer_interrupt_flag_get(TIMER6, TIMER_INT_FLAG_UP) == SET)
    {
        Key_Scan(&g_key1, key1_get_status());
        Key_Scan(&g_key2, key2_get_status());
		// printf("Key1 history Level: %x\r\n", g_key1.history);
		// printf("Key2 history Level: %x\r\n", g_key2.history);
        timer_interrupt_flag_clear(TIMER6, TIMER_INT_FLAG_UP);
    }
}

void example_usage(void)
{
	KeyEvent_t events[] = {Key_GetEvent(&g_key1), Key_GetEvent(&g_key2)};
	const char *key_names[] = { "Key1", "Key2" };

	for (int i = 0; i < sizeof(events) / sizeof(events[0]); i++)
	{
		switch (events[i])
		{
		case KEY_EVENT_SINGLE_CLICK:
			printf("%s Single Click Detected\r\n", key_names[i]);
			led_on(i == 0 ? 1 : 3);
			break;

		case KEY_EVENT_DOUBLE_CLICK:
			printf("%s Double Click Detected\r\n", key_names[i]);
			led_off(1);
			led_off(2);
			led_off(3);
			break;

		case KEY_EVENT_LONG_PRESS:
			printf("%s Long Press Detected\r\n", key_names[i]);
			led_on(2);
			break;

		case KEY_EVENT_LONG_REPEAT:
			printf("%s Long Repeat Detected\r\n", key_names[i]);
			if (i == 0) led_on(3);
			break;

		default:
			break;
		}
	}
}
