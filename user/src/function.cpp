#include "FreeRTOS.h"
#include "device_ina228.hpp"
#include "device_oled.hpp"
#include "device_serial.hpp"
#include "device_w25q128.hpp"
#include "function.hpp"
#include "inter_adc.hpp"
#include "inter_can.hpp"
#include "inter_exti.hpp"
#include "inter_i2c_bus.hpp"
#include "inter_i2c_dev.hpp"
#include "inter_io_ctrl.hpp"
#include "inter_soft_spi.hpp"
#include "inter_soft_uart.hpp"
#include "inter_spi.hpp"
#include "inter_usart.hpp"
#include "pc_task.hpp"
#include "protocol_conf.hpp"
#include "systick.h"
#include "task.h"

#ifdef APP_IAP
#include "iap_app.hpp"
#endif

#include <cstdio>
#include <cstring>

/* ══════════════════════════════════════════════════════
 *  任务句柄
 * ══════════════════════════════════════════════════════ */
static TaskHandle_t xUserLedHand = NULL;
static TaskHandle_t xFlashLedHand = NULL;

/* ══════════════════════════════════════════════════════
 *  外设实例
 * ══════════════════════════════════════════════════════ */

/* ── 软 I2C 总线：PE2/PE3，默认 400k@240MHz ── */
inter_i2c_bus i2c_bus({portE, portE, pin2, pin3});
inter_i2c_bus oled_bus({portC, portC, pin1, pin2});

inter_i2c_dev oled_dev(&oled_bus, 0x3C);

/* ── 软串口（测试模式：PA9/PA10，与硬件 debug_uart 同引脚，测试期间禁用硬件） ── */
soft_uart_port<256> suart({portA, pin9,  // TX
                           portA, pin10, // RX
                           soft_uart_port<256>::BAUD_115200});

/* ── INA228 配置：报警引脚 PG14（ALERT 开漏低有效，内部上拉） ──
 * C++20 designated initializer：只配置需要的字段，其余用默认值 */
static const INA228::Config ina228_cfg = {
    .alert_port = portG,
    .alert_pin = pin14,
} /*  */;

INA228 INA228_2(&i2c_bus, INA228::ADDR_0x40, ina228_cfg);
oled oled1({&oled_dev, oled::Controller::SSD1306});
/* ── CAN 总线（CAN1 @ PB12/RX PB13/TX，双板互联 NORMAL 模式） ── */
[[maybe_unused]] static can_port can_bus({
    .periph = can_id::can1,
    .tx_port = portB,
    .tx_pin = pin13,
    .rx_port = portB,
    .rx_pin = pin12,
    .af = af9,
    .baudrate = 1000000, // 1Mbps 极限速率实测（12TQ: 60MHz/1M/12 = prescaler 5）
    .mode = CAN_NORMAL_MODE,
    .filter_bank = 15, // CAN1 标准起始 bank
});
[[maybe_unused]] static uint32_t s_can_tx_count = 0;

/* ── IO 控制 ── */
io_ctrl user_led(portC, pin4);
io_ctrl flash_led(portC, pin5);
io_ctrl state_led(portA, pin7);

/* ══════════════════════════════════════════════════════
 *  任务函数
 * ══════════════════════════════════════════════════════ */
static void xTuserled(void *led_task)
{
    (void)led_task;
    for (;;)
    {
        user_led.toggle();
        vTaskDelay(pdMS_TO_TICKS(300));
    }
}

static void xTflashled(void *falsh_task)
{
    (void)falsh_task;
    for (;;)
    {
        flash_led.toggle();
        vTaskDelay(pdMS_TO_TICKS(100));
    }
}

/* ══════════════════════════════════════════════════════
 *  初始化
 * ══════════════════════════════════════════════════════ */
void function_init(void)
{
#ifdef APP_IAP
    /* IAP：VTOR 已在 main() 设置；此处喂狗（boot 跳转前已使能 IWDG） */
    Iap_AppFeedWdt();
#endif

    /* ⚠️ 测试模式：硬件 debug_uart 与软串口同引脚(PA9/10)，临时禁用硬件 */
    // DebugPort_Init();
    suart.init();

    /* ⚠️ 双板 CAN 实测：仅初始化 CAN，跳过其他自检
     * ⚠️ 本轮验证：启用软 I2C 总线 + INA228 自检（I2C 抽象化上板验证） */
    i2c_bus.init();
    // INA228_2.init();   // 自检内部会 reset+init，此处无需重复

    user_led.init(mode_output, pulldown, otype_pp, speed_high);
    flash_led.init(mode_output, pulldown, otype_pp, speed_high);
    state_led.init(mode_output, pulldown, otype_pp, speed_high);

    Protocol_Init();

    if (xTaskCreate(xTuserled, "user_led", 128, NULL, tskIDLE_PRIORITY + 1, &xUserLedHand) != pdPASS)
    {
        for (;;)
        {
        }
    }
    if (xTaskCreate(xTflashled, "falsh_led", 128, NULL, tskIDLE_PRIORITY + 1, &xFlashLedHand) != pdPASS)
    {
        for (;;)
        {
        }
    }

#ifdef APP_IAP
    /* IAP 状态提示：激活确认进度 / 固化生效确认（升级后重启可见） */
    char iap_status[96];
    Iap_AppGetStatus(iap_status, sizeof(iap_status));
    suart.send_data((const uint8_t *)iap_status, (uint16_t)strlen(iap_status));
#endif

    /* ⚠️ W25Q128 HW-SPI 验证：临时禁用双板 CAN 会话测试与 ADC 自检
    bool ok = can_bus.init();
    char buf[64];
    int len = snprintf(buf, sizeof(buf),
                       "\r\n[CAN1] NORMAL dual-board test init=%d (1Mbps, PB12/13)\r\n", ok);
    (void)suart.send_data((const uint8_t *)buf, len);

    // 诊断：过滤器使能位全值 + FIFO0 长度
    len = snprintf(buf, sizeof(buf), "[CAN] DBG FW=0x%08lX FCTL=0x%08lX FIFO0=%lu\r\n",
                   (unsigned long)CAN_FW(CAN0),
                   (unsigned long)CAN_FCTL(CAN0),
                   (unsigned long)can_receive_message_length_get(CAN1, CAN_FIFO0));
    (void)suart.send_data((const uint8_t *)buf, len);

    if (!ok)
        (void)suart.send_data((const uint8_t *)"[CAN1] init FAILED\r\n", 22);
    */
}

/* ══════════════════════════════════════════════════════
 *  INA228 → Modbus Input Registers
 * ══════════════════════════════════════════════════════ */
#if ACTIVE_PROTOCOL == PROTOCOL_MODBUS
#include "modbus_map.hpp"

enum
{
    REG_BUSV_HI = 0,
    REG_BUSV_LO = 1,
    REG_SHUNTV_HI = 2,
    REG_SHUNTV_LO = 3,
    REG_CUR_HI = 4,
    REG_CUR_LO = 5,
    REG_PWR_HI = 6,
    REG_PWR_LO = 7,
    REG_TEMP_HI = 8,
    REG_TEMP_LO = 9,
    REG_ENERGY_3 = 10,
    REG_ENERGY_2 = 11,
    REG_ENERGY_1 = 12,
    REG_ENERGY_0 = 13,
    REG_CHARGE_3 = 14,
    REG_CHARGE_2 = 15,
    REG_CHARGE_1 = 16,
    REG_CHARGE_0 = 17,
    REG_MFR_ID = 18,
    REG_DEV_ID = 19,
    REG_ALERT = 20,
};

void UpdateModbusRegisters(void)
{
    extern uint16_t StatusBlock[];

    int32_t vm = INA228_2.getBusVoltage_mV();
    int32_t sv = INA228_2.getShuntVoltage_uV();
    int32_t im = INA228_2.getCurrent_mA();
    int32_t pm = INA228_2.getPower_mW();
    int32_t tm = INA228_2.getTemperature_mC();
    int64_t em = INA228_2.getEnergy_mJ();
    int64_t cm = INA228_2.getCharge_mC();
    int64_t vu = (int64_t)vm * 1000;

    StatusBlock[REG_BUSV_HI] = (uint16_t)((uint64_t)vu >> 16);
    StatusBlock[REG_BUSV_LO] = (uint16_t)(vu);

    StatusBlock[REG_SHUNTV_HI] = (uint16_t)((uint32_t)sv >> 16);
    StatusBlock[REG_SHUNTV_LO] = (uint16_t)(sv);

    StatusBlock[REG_CUR_HI] = (uint16_t)((uint32_t)im >> 16);
    StatusBlock[REG_CUR_LO] = (uint16_t)(im);

    StatusBlock[REG_PWR_HI] = (uint16_t)((uint32_t)pm >> 16);
    StatusBlock[REG_PWR_LO] = (uint16_t)(pm);

    StatusBlock[REG_TEMP_HI] = (uint16_t)((uint32_t)tm >> 16);
    StatusBlock[REG_TEMP_LO] = (uint16_t)(tm);

    StatusBlock[REG_ENERGY_3] = (uint16_t)((uint64_t)em >> 48);
    StatusBlock[REG_ENERGY_2] = (uint16_t)((uint64_t)em >> 32);
    StatusBlock[REG_ENERGY_1] = (uint16_t)((uint64_t)em >> 16);
    StatusBlock[REG_ENERGY_0] = (uint16_t)(em);

    StatusBlock[REG_CHARGE_3] = (uint16_t)((uint64_t)cm >> 48);
    StatusBlock[REG_CHARGE_2] = (uint16_t)((uint64_t)cm >> 32);
    StatusBlock[REG_CHARGE_1] = (uint16_t)((uint64_t)cm >> 16);
    StatusBlock[REG_CHARGE_0] = (uint16_t)(cm);

    StatusBlock[REG_MFR_ID] = INA228_2.getManufacturerID();
    StatusBlock[REG_DEV_ID] = INA228_2.getDeviceID();
    StatusBlock[REG_ALERT] = INA228_2.readDiagAlertRaw();
}
#endif

/* ══════════════════════════════════════════════════════
 *  主循环
 * ══════════════════════════════════════════════════════ */
void function_loop(void)
{
#ifdef APP_IAP
    /* IAP：周期喂狗 + 首圈上报 boot_ok（PENDING 确认窗口内） */
    Iap_AppFeedWdt();
    Iap_AppReportBootOk();
#endif

    APP_PC_Task();

    /* ── 软串口接收轮询 ── */
    suart.poll();
}
