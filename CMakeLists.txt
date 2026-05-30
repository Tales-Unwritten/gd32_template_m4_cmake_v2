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

    # ── 启动 / 系统 ──────────────────────────────────
    ${STARTUP_FILE}
    ${SYSCALLS_FILE}
    ${CMSIS_DIR}/GD/GD32F4xx/Source/system_gd32f4xx.c

    # ── GD32 标准外设库 ─────────────────────────────
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
    ${LIB_DIR}/Source/gd32f4xx_spi.c
    ${LIB_DIR}/Source/gd32f4xx_can.c
    ${LIB_DIR}/Source/gd32f4xx_i2c.c

    # ── 内核 / 调度 ─────────────────────────────────
    code/src/main.c
    code/src/gd32f4xx_it.c
    code/src/systick.c
    code/src/link.c

    # ── 硬件层（旧 C 驱动，逐步迁移中） ─────────────
    Hardware/src/bsp_led.c
    Hardware/src/eeprom.c
    # Hardware/src/debug_uart.c  # 已迁移至 interface/usart_port

    # ── 接口抽象层（C++ 核心驱动） ───────────────────
    interface/src/io_ctrl.cpp           # GPIO 引脚管理
    interface/src/usart_async_port.cpp  # 异步串口驱动（含 ISR，零堆分配）
    interface/src/spi_port.cpp          # SPI 主机驱动（阻塞全双工）
    interface/src/timer_port.cpp        # 定时器驱动（基本定时 + PWM）
    interface/src/can_port.cpp          # CAN 总线驱动（阻塞收发）
    interface/src/i2c_hw_port.cpp       # 硬件 I2C 驱动（阻塞读写）
    interface/src/adc_port.cpp          # ADC 驱动（阻塞单通道 + 扫描）
    interface/src/dac_port.cpp          # DAC 驱动（双通道电压输出）
    interface/src/dma_port.cpp          # DMA 通道驱动（M2M + 外设）
    interface/src/exti_port.cpp         # 外部中断驱动
    interface/src/wdt_port.cpp          # 看门狗驱动（IWDG + WWDG）
    interface/src/device_i2c_bus.cpp    # I2C 总线
    interface/src/device_i2c_dev.cpp    # I2C 设备基类

    # ── BSP 设备驱动 ────────────────────────────────
    bsp/src/ina226.cpp                  # INA226 电压/电流传感器
    bsp/src/ina228.cpp                  # INA228 高精度传感器
    bsp/src/debug_port.cpp              # Debug 串口 (USART0)
    bsp/src/rs485_port.cpp              # RS485 串口 (USART1, DE=PG13)
    bsp/src/rs232_port.cpp              # RS232 串口 (USART2)
    bsp/src/w25q128_flash.cpp           # W25Q128 Flash (SPI4)

    # ── Modbus 协议栈 ───────────────────────────────
    middleware/modbus/src/modbus_crc.cpp         # CRC 校验
    middleware/modbus/src/modbus_map.cpp         # 寄存器映射表
    middleware/modbus/src/modbus_port_cfg.cpp    # 端口配置（平台胶水）
    middleware/modbus/src/modbus.cpp             # 协议核心（帧解析/功能码）

    # ── 应用协议层 ─────────────────────────────────
    App/src/private.cpp                 # 波特率存储管理
    App/src/protocol.cpp                # 多协议调度 + Modbus 寄存器刷新
    App/src/protocol_string.cpp         # 字符串指令解析器
    App/src/pc_task.cpp                 # 任务入口（薄调度）

    # ── 用户应用 ───────────────────────────────────
    user/src/founction.cpp              # 主循环 / 初始化
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
    middleware/modbus/inc
    App/inc
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
