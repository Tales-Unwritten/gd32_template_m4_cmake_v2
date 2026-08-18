#include "gd32f4xx_rcu.h"
#include "gd32f4xx_gpio.h"
#include "systick.h"
#include <stdio.h>
#include "main.h"

#include "founction.hpp"

#ifdef APP_IAP
#include "iap_app.hpp"
#endif

int main(void)
{
#ifdef APP_IAP
    /* IAP：VTOR 重定位必须先于任何中断使能（systick_config 之前） */
    Iap_AppInit();
#endif
    systick_config();
    function_init();

    while (1)
    {
        function_loop();
    }
}
