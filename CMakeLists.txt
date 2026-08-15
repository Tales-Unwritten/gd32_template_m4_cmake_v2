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

# 在项目根目录创建 compile_commands.json 符号链接，
# 供 clangd / clang-tidy 等工具进行代码导航和诊断
set(_cc_json "${CMAKE_SOURCE_DIR}/compile_commands.json")
if(EXISTS "${_cc_json}")
    file(REMOVE "${_cc_json}")
endif()
file(CREATE_LINK "${CMAKE_BINARY_DIR}/compile_commands.json"
                 "${_cc_json}" SYMBOLIC)

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

# C++20：支持 designated initializers，Config 可只配置部分字段
set(CMAKE_CXX_STANDARD 20)
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
    ${LIB_DIR}/Source/gd32f4xx_rtc.c
    ${LIB_DIR}/Source/gd32f4xx_pmu.c

    # ── 内核 / 调度 ─────────────────────────────────
    code/src/main.c
    code/src/gd32f4xx_it.c
    code/src/systick.c
    code/src/link.c

    # ── 硬件层（旧 C 驱动，逐步迁移中） ─────────────
    hardware/src/hard_bsp_led.c
    hardware/src/hard_eeprom.c
    # hardware/src/hard_debug_uart.c  # 已迁移至 interface/inter_usart

    # ── 接口抽象层（C++ 核心驱动） ───────────────────
    interface/src/inter_io_ctrl.cpp           # GPIO 引脚管理
    interface/src/inter_usart.cpp        # 串口通用驱动（含 ISR）
    interface/src/inter_spi.cpp          # SPI 主机驱动（阻塞全双工）
    interface/src/inter_timer.cpp        # 定时器驱动（基本定时 + PWM）
    interface/src/inter_can.cpp          # CAN 总线驱动（阻塞收发）
    interface/src/inter_i2c_hw.cpp       # 硬件 I2C 驱动（阻塞读写）
    interface/src/inter_adc.cpp          # ADC 驱动（阻塞单通道 + 扫描）
    interface/src/inter_dac.cpp          # DAC 驱动（双通道电压输出）
    interface/src/inter_dma.cpp          # DMA 通道驱动（M2M + 外设）
    interface/src/inter_exti.cpp         # 外部中断驱动
    interface/src/inter_wdt.cpp          # 看门狗驱动（IWDG + WWDG）
    interface/src/inter_rtc.cpp          # RTC 驱动（日历 + 闹钟）
    interface/src/inter_flash.cpp        # 内部 Flash 读写
    interface/src/inter_i2c_bus.cpp    # I2C 总线
    interface/src/inter_i2c_dev.cpp    # I2C 设备基类
    interface/src/inter_soft_spi.cpp   # 软件 SPI 总线（GPIO 位操作）
    interface/src/inter_soft_uart.cpp  # 软件串口（位反转 + DWT 时序）

    # ── device 设备驱动 ────────────────────────────────
    device/src/device_serial.cpp                  # 串口设备（debug/rs232/rs485 + DE）
    device/src/device_ina226.cpp                  # INA226 电压/电流传感器
    device/src/device_ina228.cpp                  # INA228 高精度传感器
    device/src/device_ina219.cpp                  # INA219 电压/电流传感器
    device/src/device_ina260.cpp                  # INA260 内置分流传感器
    device/src/device_ina3221.cpp                 # INA3221 3 通道传感器

    # ── Modbus 协议栈 ───────────────────────────────
    protocol/modbus/src/modbus_crc.cpp         # CRC 校验
    protocol/modbus/src/modbus_map.cpp         # 寄存器映射表
    # protocol/modbus/src/modbus_port_cfg.cpp  # 已内聚到 modbus.cpp
    protocol/modbus/src/modbus.cpp             # 协议核心（帧解析/功能码）

    protocol/string/src/string_pro.cpp         # 字符串指令解析器
    protocol/string/src/str_cmd.cpp             # 字符串命令框架


    protocol/hex/src/hex.cpp         # hex指令解析器

    # ── 应用协议层 ─────────────────────────────────
    app/src/private.cpp                 # 波特率存储管理
    app/src/protocol_conf.cpp                # 多协议调度 + Modbus 寄存器刷新
    app/src/pc_task.cpp                 # 任务入口（薄调度）

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
    hardware/inc
    interface/inc
    device/inc
    user/inc
    protocol/modbus/inc
    protocol/string/inc
    protocol/hex/inc
    app/inc
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
