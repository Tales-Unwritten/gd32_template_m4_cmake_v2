#pragma once

/* Standard includes */
#include "gd32f4xx.h"
#include "systick.h"

/* C++ compatibility */
#ifdef __cplusplus
extern "C" {
#endif

/* Configuration macros */
#define DIP_SWITCH_4_8BIT 0  //0 = 4位拨码开关，1 = 8位拨码开关

/* Hardware pin definitions */
#define DIP_PERIPH_CLOCK0       RCU_GPIOB
#define DIP_GPIO_PORT0          GPIOB
#define DIP_GPIO_PIN0           GPIO_PIN_0

#define DIP_PERIPH_CLOCK1       RCU_GPIOB
#define DIP_GPIO_PORT1          GPIOB
#define DIP_GPIO_PIN1           GPIO_PIN_1

#define DIP_PERIPH_CLOCK2       RCU_GPIOB
#define DIP_GPIO_PORT2          GPIOB
#define DIP_GPIO_PIN2           GPIO_PIN_2

#define DIP_PERIPH_CLOCK3       RCU_GPIOB
#define DIP_GPIO_PORT3          GPIOB
#define DIP_GPIO_PIN3           GPIO_PIN_3

#define DIP_PERIPH_CLOCK4       RCU_GPIOB
#define DIP_GPIO_PORT4          GPIOB
#define DIP_GPIO_PIN4           GPIO_PIN_4

#define DIP_PERIPH_CLOCK5       RCU_GPIOB
#define DIP_GPIO_PORT5          GPIOB
#define DIP_GPIO_PIN5           GPIO_PIN_5

#define DIP_PERIPH_CLOCK6       RCU_GPIOB
#define DIP_GPIO_PORT6          GPIOB
#define DIP_GPIO_PIN6           GPIO_PIN_6

#define DIP_PERIPH_CLOCK7       RCU_GPIOB
#define DIP_GPIO_PORT7          GPIOB
#define DIP_GPIO_PIN7           GPIO_PIN_7

/* Function declarations */
void    dip_switch_init  (void);
uint8_t dip_switch_read  (void);
void    verify_dip_switch(void);

#ifdef __cplusplus
}
#endif
