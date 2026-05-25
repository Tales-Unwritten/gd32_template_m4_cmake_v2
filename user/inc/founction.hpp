#pragma once

#include "link.h"

#ifdef __cplusplus
#include "usart_port.hpp"
#include "debug_port.hpp"
#endif

#ifdef __cplusplus
extern "C" {
#endif

extern void function_init(void);
extern void function_loop(void);

#ifdef __cplusplus
}
#endif
