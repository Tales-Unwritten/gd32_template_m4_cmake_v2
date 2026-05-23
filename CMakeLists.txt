cmake_minimum_required(VERSION 3.20)

#
add_compile_options($<$<COMPILE_LANGUAGE:CXX>:-Wno-register>)


# ============================================================
# Toolchain
# ============================================================

set(CMAKE_TOOLCHAIN_FILE
    ${CMAKE_CURRENT_SOURCE_DIR}/cmake/arm-none-eabi-gcc.cmake
)

# ============================================================
# Project
# ============================================================

project(gd32f470_template
    LANGUAGES C CXX ASM
)

# ============================================================
# Export compile_commands.json
# ============================================================

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# ============================================================
# Build Type
# ============================================================

if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Debug)
endif()

# ============================================================
# Language Standard
# ============================================================

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# ============================================================
# MCU
# ============================================================

set(MCU_FLAGS
    -mcpu=cortex-m4
    -mthumb
    -mfpu=fpv4-sp-d16
    -mfloat-abi=hard
)

# ============================================================
# Paths
# ============================================================

set(FW_DIR      ${CMAKE_CURRENT_SOURCE_DIR}/Firmware)
set(CMSIS_DIR   ${FW_DIR}/CMSIS)
set(LIB_DIR     ${FW_DIR}/library)

# ============================================================
# Startup / Linker
# ============================================================

set(STARTUP_FILE
    ${CMSIS_DIR}/GD/GD32F4xx/Source/GCC/startup_gd32f450_470.S
)

set(LINKER_SCRIPT
    ${CMSIS_DIR}/GD/GD32F4xx/Source/GCC/Ld/gd32f470xI_flash.ld
)

set(SYSCALLS_FILE
    ${CMSIS_DIR}/GD/GD32F4xx/Source/GCC/newlib/syscalls.c
)

# ============================================================
# Executable
# ============================================================

add_executable(${PROJECT_NAME}.elf)

# ============================================================
# Sources
# ============================================================

target_sources(${PROJECT_NAME}.elf PRIVATE

    # startup
    ${STARTUP_FILE}
    ${SYSCALLS_FILE}

    # system
    ${CMSIS_DIR}/GD/GD32F4xx/Source/system_gd32f4xx.c

    # code
    code/src/main.c
    code/src/gd32f4xx_it.c
    code/src/systick.c
    code/src/link.c

    # hardware
    Hardware/src/debug_uart.c
    Hardware/src/bsp_led.c

    # interface
    interface/src/io_ctrl.cpp
    interface/src/device_i2c_bus.cpp
    interface/src/device_i2c_dev.cpp

    # bsp
    bsp/src/ina226.cpp
    bsp/src/ina228.cpp

    # user
    user/src/founction.cpp

    # gd32 lib
    ${LIB_DIR}/Source/gd32f4xx_rcu.c
    ${LIB_DIR}/Source/gd32f4xx_gpio.c
    ${LIB_DIR}/Source/gd32f4xx_misc.c
    ${LIB_DIR}/Source/gd32f4xx_usart.c
    ${LIB_DIR}/Source/gd32f4xx_fmc.c
    ${LIB_DIR}/Source/gd32f4xx_timer.c
    ${LIB_DIR}/Source/gd32f4xx_adc.c
    ${LIB_DIR}/Source/gd32f4xx_dac.c
    ${LIB_DIR}/Source/gd32f4xx_dma.c
    ${LIB_DIR}/Source/gd32f4xx_exti.c
    ${LIB_DIR}/Source/gd32f4xx_syscfg.c
)

# ============================================================
# Include Directories
# ============================================================

target_include_directories(${PROJECT_NAME}.elf PRIVATE

    # cmsis
    ${CMSIS_DIR}
    ${CMSIS_DIR}/GD/GD32F4xx/Include

    # gd32 library
    ${LIB_DIR}/Include

    # project
    code/inc
    Hardware/inc
    interface/inc
    bsp/inc
    user/inc
)

# ============================================================
# Defines
# ============================================================

target_compile_definitions(${PROJECT_NAME}.elf PRIVATE

    GD32F470
    USE_STDPERIPH_DRIVER
    HSE_VALUE=25000000U
)

# ============================================================
# Compile Options
# ============================================================

target_compile_options(${PROJECT_NAME}.elf PRIVATE

    ${MCU_FLAGS}

    -Wall
    -Wextra

    -fdata-sections
    -ffunction-sections

    -fno-common

    $<$<CONFIG:Debug>:-Og>
    $<$<CONFIG:Debug>:-g3>
    $<$<CONFIG:Debug>:-DDEBUG>

    $<$<CONFIG:Release>:-O2>

    # C++
    $<$<COMPILE_LANGUAGE:CXX>:-fno-exceptions>
    $<$<COMPILE_LANGUAGE:CXX>:-fno-rtti>
    $<$<COMPILE_LANGUAGE:CXX>:-fno-threadsafe-statics>
)

# ============================================================
# Link Options
# ============================================================

target_link_options(${PROJECT_NAME}.elf PRIVATE

    ${MCU_FLAGS}

    -T${LINKER_SCRIPT}

    --specs=nano.specs
    --specs=nosys.specs

    -Wl,--gc-sections
    -Wl,--print-memory-usage

    -Wl,-Map=${CMAKE_BINARY_DIR}/${PROJECT_NAME}.map
)


# ============================================================
# Post Build
# ============================================================

add_custom_command(TARGET ${PROJECT_NAME}.elf POST_BUILD

    COMMAND ${CMAKE_OBJCOPY}
        -O ihex
        $<TARGET_FILE:${PROJECT_NAME}.elf>
        ${PROJECT_NAME}.hex

    COMMAND ${CMAKE_OBJCOPY}
        -O binary
        $<TARGET_FILE:${PROJECT_NAME}.elf>
        ${PROJECT_NAME}.bin

    COMMAND ${CMAKE_SIZE}
        $<TARGET_FILE:${PROJECT_NAME}.elf>

    COMMENT "Generating HEX/BIN"
)

# ============================================================
# OpenOCD
# ============================================================

set(OCD openocd)

set(OCD_DIR
    ${CMAKE_CURRENT_SOURCE_DIR}/openocd
)

# ============================================================
# Flash DAPLink
# ============================================================

add_custom_target(flash-daplink

    COMMAND ${OCD}
        -f ${OCD_DIR}/interface-daplink.cfg
        -f ${OCD_DIR}/target-gd32f4xx.cfg
        -c "program ${CMAKE_BINARY_DIR}/${PROJECT_NAME}.elf verify reset exit"

    DEPENDS ${PROJECT_NAME}.elf

    USES_TERMINAL
)

# ============================================================
# Flash JLink
# ============================================================

add_custom_target(flash-jlink

    COMMAND ${OCD}
        -f ${OCD_DIR}/interface-jlink.cfg
        -f ${OCD_DIR}/target-gd32f4xx.cfg
        -c "program ${CMAKE_BINARY_DIR}/${PROJECT_NAME}.elf verify reset exit"

    DEPENDS ${PROJECT_NAME}.elf

    USES_TERMINAL
)

# ============================================================
# Debug Server DAPLink
# ============================================================

add_custom_target(debug

    COMMAND ${OCD}
        -f ${OCD_DIR}/interface-daplink.cfg
        -f ${OCD_DIR}/target-gd32f4xx.cfg

    USES_TERMINAL
)

# ============================================================
# Debug Server JLink
# ============================================================

add_custom_target(debug-jlink

    COMMAND ${OCD}
        -f ${OCD_DIR}/interface-jlink.cfg
        -f ${OCD_DIR}/target-gd32f4xx.cfg

    USES_TERMINAL
)

# ============================================================
# GDB
# ============================================================

add_custom_target(gdb

    COMMAND arm-none-eabi-gdb
        ${CMAKE_BINARY_DIR}/${PROJECT_NAME}.elf
        -ex "target remote localhost:3333"
        -ex "monitor reset halt"
        -ex "load"
        -ex "break main"
        -ex "continue"

    DEPENDS ${PROJECT_NAME}.elf

    USES_TERMINAL
)
