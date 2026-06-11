#ifndef _BSP_LED_H
#define _BSP_LED_H

#include "gd32f4xx.h"
#include "systick.h"


#define RCU_LED1  	RCU_GPIOC   // GPIOA的时钟
#define RCU_LED2  	RCU_GPIOC   // GPIOA的时钟
#define RCU_LED3  	RCU_GPIOA   // GPIOA的时钟

#define PORT_LED1 	GPIOC				// GPIOA的端口
#define PORT_LED2 	GPIOC				// GPIOA的端口
#define PORT_LED3 	GPIOA				// GPIOA的端口


#define PIN_LED1 		GPIO_PIN_5  // GPIOA的引脚
#define PIN_LED2 		GPIO_PIN_4  // GPIOA的引脚
#define PIN_LED3 		GPIO_PIN_7  // GPIOA的引脚

void led_gpio_config(void); 	    // led gpio引脚配置
void led_on(uint8_t led_num);       // 点亮LED
void led_off(uint8_t led_num);      // 熄灭LED
void led_toggle(uint8_t led_num);   // 切换LED状态

#endif /* BSP_LED_H */