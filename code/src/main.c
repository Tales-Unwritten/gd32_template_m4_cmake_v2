#include "gd32f4xx_rcu.h"
#include "gd32f4xx_gpio.h"
#include "systick.h"
#include <stdio.h>
#include "main.h"

#include "founction.hpp"

int main(void)
{
    systick_config();
    function_init();

    while (1)
    {
        function_loop();
    }
}
