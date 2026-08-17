#include "device_ina228.hpp"
#include "device_w25q128.hpp"
#include "device_serial.hpp"
#include "founction.hpp"
#include "inter_i2c_bus.hpp"
#include "inter_i2c_dev.hpp"
#include "inter_io_ctrl.hpp"
#include "inter_exti.hpp"
#include "inter_can.hpp"
#include "inter_adc.hpp"
#include "inter_spi.hpp"
#include "inter_soft_spi.hpp"
#include "inter_soft_uart.hpp"
#include "inter_usart.hpp"
#include "pc_task.hpp"
#include "protocol_conf.hpp"
#include "systick.h"

#include <cstdio>
#include <cstring>

/* ── 外设 ──────────────────────────────────────────── */

inter_i2c_bus i2c_bus({GPIOE, GPIOE, GPIO_PIN_2, GPIO_PIN_3}, 1);

/* ── 软串口（测试模式：PA9/PA10，与硬件 debug_uart 同引脚，测试期间禁用硬件） ── */
soft_uart_port suart({
    GPIOA, GPIO_PIN_9,        // TX
    GPIOA, GPIO_PIN_10,       // RX
    soft_uart_port::BAUD_115200
});
/* ── INA228 配置：报警引脚 PG14（ALERT 开漏低有效，内部上拉） ── */
/* C++20 designated initializer：只配置需要的字段，其余用默认值 */
static const INA228::Config ina228_cfg = {
    .alert_port = GPIOG,
    .alert_pin  = GPIO_PIN_14,
};

INA228 INA228_2(&i2c_bus, INA228::ADDR_0x40, ina228_cfg);

/* ── CAN 总线（CAN1 @ PB12/RX PB13/TX，双板互联 NORMAL 模式） ── */
[[maybe_unused]] static can_port can_bus({
    .periph      = can_id::can1,
    .tx_port     = GPIOB, .tx_pin = GPIO_PIN_13,
    .rx_port     = GPIOB, .rx_pin = GPIO_PIN_12,
    .af          = GPIO_AF_9,
    .baudrate    = 1000000,   // 1Mbps 极限速率实测（12TQ: 60MHz/1M/12 = prescaler 5）
    .mode        = CAN_NORMAL_MODE,
    .filter_bank = 15,     // CAN1 标准起始 bank
});
[[maybe_unused]] static uint32_t s_can_tx_count = 0;

io_ctrl led1(GPIOC, GPIO_PIN_4);
io_ctrl led2(GPIOC, GPIO_PIN_5);
io_ctrl led3(GPIOA, GPIO_PIN_7);

/* ── 初始化 ──────────────────────────────────────────── */
[[maybe_unused]] static void ina228_selftest(void);   // 定义见下方（测试固件暂未调用）
[[maybe_unused]] static void w25q64_selftest(void);   // 定义见下方
[[maybe_unused]] static void w25q128_hw_selftest(void); // 定义见下方
[[maybe_unused]] static void exti_selftest(void);     // 定义见下方
[[maybe_unused]] static void adc_selftest(void);      // 定义见下方
[[maybe_unused]] static void suart_baud_test(void);   // 定义见下方
void function_init(void)
{
    // ⚠️ 测试模式：硬件 debug_uart 与软串口同引脚(PA9/10)，临时禁用硬件
    // DebugPort_Init();
    suart.init();
    // ⚠️ 双板 CAN 实测：仅初始化 CAN，跳过其他自检
    // i2c_bus.init();
    // INA228_2.init();
    led1.init(GPIO_MODE_OUTPUT, GPIO_PUPD_NONE, GPIO_OTYPE_PP, GPIO_OSPEED_50MHZ);
    led2.init(GPIO_MODE_OUTPUT, GPIO_PUPD_NONE, GPIO_OTYPE_PP, GPIO_OSPEED_50MHZ);
    led3.init(GPIO_MODE_OUTPUT, GPIO_PUPD_NONE, GPIO_OTYPE_PP, GPIO_OSPEED_50MHZ);
    // Protocol_Init();

    // ⚠️ W25Q128 HW-SPI 验证：临时禁用双板 CAN 会话测试与 ADC 自检
    // bool ok = can_bus.init();
    // char buf[64];
    // int len = snprintf(buf, sizeof(buf), "\r\n[CAN1] NORMAL dual-board test init=%d (1Mbps, PB12/13)\r\n", ok);
    // (void)suart.send_data((const uint8_t *)buf, len);
    // // 诊断：过滤器使能位全值 + FIFO0 长度
    // len = snprintf(buf, sizeof(buf), "[CAN] DBG FW=0x%08lX FCTL=0x%08lX FIFO0=%lu\r\n",
    //                (unsigned long)CAN_FW(CAN0),
    //                (unsigned long)CAN_FCTL(CAN0),
    //                (unsigned long)can_receive_message_length_get(CAN1, CAN_FIFO0));
    // (void)suart.send_data((const uint8_t *)buf, len);
    // if (!ok)
    //     (void)suart.send_data((const uint8_t *)"[CAN1] init FAILED\r\n", 22);

    w25q128_hw_selftest();
}

/* ── INA228 全功能自检（一次性，init() 后调用） ───────── */
static void ina228_selftest(void)
{
    char buf[160];
    int  len;

    auto tx = [&](const char *s, int n) { (void)suart.send_data((const uint8_t *)s, (uint16_t)n); };
    auto txln = [&](const char *s) {
        (void)suart.send_data((const uint8_t *)s, (uint16_t)strlen(s));
        (void)suart.send_data((const uint8_t *)"\r\n", 2);
    };

    txln("\r\n========== INA228 SELF TEST ==========");

    // 1. 连接 / 芯片身份
    txln("-- 1. Connection & ID --");
    if (INA228_2.isConnected()) {
        len = snprintf(buf, sizeof(buf), "isConnected   : OK  (MFR=0x%04X, DEV=0x%04X)\r\n",
                       INA228_2.getManufacturerID(), INA228_2.getDeviceID());
        tx(buf, len);
    } else {
        len = snprintf(buf, sizeof(buf), "isConnected   : FAIL (err=%d)\r\n", INA228_2.getLastError());
        tx(buf, len);
    }

    // 2. 校准（2A @ 1mΩ）
    txln("-- 2. Calibration (2A @ 1mΩ) --");
    int err = INA228_2.setMaxCurrentShunt(2000, 1000);
    // 注：newlib-nano 的 snprintf 不支持 %lld，64 位值分段打印
    len = snprintf(buf, sizeof(buf), "setMaxCurrent : err=%d  calibrated=%d  LSB=%ldnA\r\n",
                   err, (int)INA228_2.isCalibrated(), (long)INA228_2.getCurrentLSB_nA());
    tx(buf, len);

    // 3. 测量（等待 1~2 组转换完成）
    txln("-- 3. Measurements --");
    delay_ms(400);
    int32_t vbus = INA228_2.getBusVoltage_mV();
    int32_t vsh  = INA228_2.getShuntVoltage_uV();
    int32_t cur  = INA228_2.getCurrent_mA();
    int32_t pwr  = INA228_2.getPower_mW();
    int32_t tmp  = INA228_2.getTemperature_mC();
    int64_t eng  = INA228_2.getEnergy_mJ();
    int64_t chg  = INA228_2.getCharge_mC();
    len = snprintf(buf, sizeof(buf), "Vbus = %ld.%03ld V | Vshunt = %ld.%03ld mV\r\n",
                   (long)vbus / 1000, (long)vbus % 1000, (long)vsh / 1000, (long)vsh % 1000);
    tx(buf, len);
    len = snprintf(buf, sizeof(buf), "I    = %ld.%03ld A | P = %ld.%03ld W | T = %ld.%ld C\r\n",
                   (long)cur / 1000, (long)cur % 1000, (long)pwr / 1000, (long)pwr % 1000,
                   (long)tmp / 1000, (long)(tmp % 1000) / 100);
    tx(buf, len);
    len = snprintf(buf, sizeof(buf), "E = %ld.%03ld J | Q = %ld.%03ld C\r\n",
                   (long)(eng / 1000), (long)(eng % 1000),
                   (long)(chg / 1000), (long)(chg % 1000));
    tx(buf, len);

    // 4. 阈值写入（先设为不触发值）
    txln("-- 4. Limit setters (non-trigger values) --");
    INA228_2.set_sovl_limit_mA(5000);
    INA228_2.set_suvl_limit_mA(0);
    INA228_2.set_bovl_limit_mV(20000);
    INA228_2.set_buvl_limit_mV(0);      // VBUS=0 时任何正阈值都会触发欠压，用 0 禁用
    INA228_2.set_temp_limit_C(125);
    INA228_2.set_pwr_limit_mW(100000);
    txln("6 limits written OK (SOVL=5A SUV=0 BOV=20V BUV=disabled TEMP=125C PWR=100W)");

    // 5. 报警触发 / 清除测试（PG14 引脚 + DIAG_ALRT 标志）
    txln("-- 5. Alert trigger & clear (PG14) --");

    // 5a. 总线过压触发（阈值 100mV < 实际总线电压）
    INA228_2.set_bovl_limit_mV(100);
    delay_ms(400);
    bool pin  = INA228_2.isAlertAsserted();
    auto fl1  = INA228_2.readAlertFlags();
    INA228_2.clearAlertLatch();
    INA228_2.set_bovl_limit_mV(20000);   // 恢复
    bool pin2 = INA228_2.isAlertAsserted();
    len = snprintf(buf, sizeof(buf),
                   "BUS-OV  : pin=%s busOverLimit=%s  (after clear: pin=%s)\r\n",
                   pin ? "LOW" : "HIGH", fl1.busOverLimit ? "SET" : "no", pin2 ? "LOW" : "HIGH");
    tx(buf, len);

    // 5b. 总线欠压触发（阈值 20V > 实际总线电压）
    INA228_2.set_buvl_limit_mV(20000);
    delay_ms(400);
    pin = INA228_2.isAlertAsserted();
    fl1 = INA228_2.readAlertFlags();
    INA228_2.clearAlertLatch();
    INA228_2.set_buvl_limit_mV(0);      // 恢复：禁用（VBUS=0 时任何正阈值都会持续触发）
    pin2 = INA228_2.isAlertAsserted();
    len = snprintf(buf, sizeof(buf),
                   "BUS-UN  : pin=%s busUnderLimit=%s  (after clear: pin=%s)\r\n",
                   pin ? "LOW" : "HIGH", fl1.busUnderLimit ? "SET" : "no", pin2 ? "LOW" : "HIGH");
    tx(buf, len);

    // 5c. 温度过限触发（阈值 0°C < 芯片温度）
    INA228_2.set_temp_limit_C(0);
    delay_ms(400);
    pin = INA228_2.isAlertAsserted();
    fl1 = INA228_2.readAlertFlags();
    INA228_2.clearAlertLatch();
    INA228_2.set_temp_limit_C(125);      // 恢复
    pin2 = INA228_2.isAlertAsserted();
    len = snprintf(buf, sizeof(buf),
                   "TEMP-OV : pin=%s tempOverLimit=%s  (after clear: pin=%s)\r\n",
                   pin ? "LOW" : "HIGH", fl1.tempOverLimit ? "SET" : "no", pin2 ? "LOW" : "HIGH");
    tx(buf, len);

    // 5d. 功率过限触发（阈值 1mW < 实际功率）
    INA228_2.set_pwr_limit_mW(1);
    delay_ms(400);
    pin = INA228_2.isAlertAsserted();
    fl1 = INA228_2.readAlertFlags();
    INA228_2.clearAlertLatch();
    INA228_2.set_pwr_limit_mW(100000);   // 恢复
    pin2 = INA228_2.isAlertAsserted();
    len = snprintf(buf, sizeof(buf),
                   "PWR-OV  : pin=%s powerOverLimit=%s  (after clear: pin=%s)\r\n",
                   pin ? "LOW" : "HIGH", fl1.powerOverLimit ? "SET" : "no", pin2 ? "LOW" : "HIGH");
    tx(buf, len);

    // 5e. 分流过压触发（阈值 1mA，取决于负载电流，如实报告）
    INA228_2.set_sovl_limit_mA(1);
    delay_ms(400);
    pin = INA228_2.isAlertAsserted();
    fl1 = INA228_2.readAlertFlags();
    INA228_2.clearAlertLatch();
    INA228_2.set_sovl_limit_mA(5000);    // 恢复
    len = snprintf(buf, sizeof(buf),
                   "SHUNT-OV: pin=%s shuntOverLimit=%s  (current-dependent)\r\n",
                   pin ? "LOW" : "HIGH", fl1.shuntOverLimit ? "SET" : "no");
    tx(buf, len);

    // 6. 寄存器快照
    txln("-- 6. Register dump --");
    INA228_2.dumpRegisters();

    // 7. 软复位测试（复位后重新 init + 校准恢复）
    txln("-- 7. Soft reset & restore --");
    INA228_2.reset();
    delay_ms(200);
    INA228_2.init();
    (void)INA228_2.setMaxCurrentShunt(2000, 1000);
    len = snprintf(buf, sizeof(buf), "after reset+init: connected=%d LSB=%ldnA\r\n",
                   (int)INA228_2.isConnected(), (long)INA228_2.getCurrentLSB_nA());
    tx(buf, len);

    txln("========== INA228 SELF TEST DONE ==========\r\n");
}

/* ── INA228 → Modbus Input Registers ────────────────── */
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
    int32_t vm = INA228_2.getBusVoltage_mV(), sv = INA228_2.getShuntVoltage_uV();
    int32_t im = INA228_2.getCurrent_mA(), pm = INA228_2.getPower_mW(), tm = INA228_2.getTemperature_mC();
    int64_t em = INA228_2.getEnergy_mJ(), cm = INA228_2.getCharge_mC(), vu = (int64_t)vm * 1000;
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

/* ── 主循环 ──────────────────────────────────────────── */
void function_loop(void)
{
    APP_PC_Task();
    led1.toggle();
    led2.toggle();

    // ── 软串口接收轮询 ──
    suart.poll();

    // ⚠️ W25Q128 HW-SPI 验证：临时禁用 CAN 周期收发
    // ── CAN 周期发送（500ms，ID=0x100，数据=递增计数器） ──
    // static uint32_t s_last_tx = 0;
    // if (get_tick() - s_last_tx >= 500)
    // {
    //     s_last_tx = get_tick();
    //     s_can_tx_count++;
    //
    //     uint8_t data[8] = {
    //         (uint8_t)(s_can_tx_count >> 24),
    //         (uint8_t)(s_can_tx_count >> 16),
    //         (uint8_t)(s_can_tx_count >> 8),
    //         (uint8_t)(s_can_tx_count),
    //         0, 0, 0, 0
    //     };
    //     bool ok = can_bus.send(0x100, false, data, 8);
    //
    //     char buf[64];
    //     int len = snprintf(buf, sizeof(buf), "[CAN] TX id=0x100 cnt=%lu %s\r\n",
    //                        (unsigned long)s_can_tx_count, ok ? "OK" : "FAIL");
    //     (void)suart.send_data((const uint8_t *)buf, len);
    // }
    //
    // // ── CAN 接收：打印所有收到的帧 ──
    // uint8_t  rx_data[8];
    // uint32_t rx_id;
    // bool     rx_ext;
    // uint8_t  rx_len;
    // while (can_bus.recv(rx_id, rx_ext, rx_data, rx_len))
    // {
    //     char buf[64];
    //     int len = snprintf(buf, sizeof(buf), "[CAN] RX id=0x%03lX len=%d %02X%02X%02X%02X %02X%02X%02X%02X\r\n",
    //                        (unsigned long)rx_id, rx_len,
    //                        rx_data[0], rx_data[1], rx_data[2], rx_data[3],
    //                        rx_data[4], rx_data[5], rx_data[6], rx_data[7]);
    //     (void)suart.send_data((const uint8_t *)buf, len);
    // }
}

/* ── W25Q64 软 SPI 测试（inter_soft_spi 实测） ───────────────── */
/* 板级接线：CS=PF6, CLK=PF7, MISO=PF8, MOSI=PF9（MODE0, MSB first, 最快） */
static soft_spi_bus w25q_spi({
    GPIOF, GPIO_PIN_7,      // SCK
    GPIOF, GPIO_PIN_9,      // MOSI
    GPIOF, GPIO_PIN_8,      // MISO
    GPIOF, GPIO_PIN_6,      // CS
    RESET,                  // CS 低有效
    0, 0, 0                 // MODE0, MSB first, delay=0
});

static void w25q_xfer(soft_spi_bus &spi, const uint8_t *tx, uint8_t *rx, uint16_t len)
{
    spi.cs_select();
    spi.transfer(tx, rx, len);
    spi.cs_deselect();
}

static bool w25q_wait_idle(soft_spi_bus &spi, uint32_t timeout_ms)
{
    uint32_t start = get_tick();
    while (get_tick() - start <= timeout_ms)
    {
        uint8_t cmd[2] = {0x05, 0x00};
        uint8_t rsp[2];
        w25q_xfer(spi, cmd, rsp, 2);
        if ((rsp[1] & 0x01) == 0)
            return true;
        delay_ms(1);
    }
    return false;
}

static void w25q64_selftest(void)
{
    char buf[160];
    int  len;

    auto tx = [&](const char *s, int n) { (void)suart.send_data((const uint8_t *)s, (uint16_t)n); };
    auto txln = [&](const char *s) {
        (void)suart.send_data((const uint8_t *)s, (uint16_t)strlen(s));
        (void)suart.send_data((const uint8_t *)"\r\n", 2);
    };

    txln("\r\n========== W25Q64 SOFT-SPI TEST ==========");
    w25q_spi.init();

    // 1. JEDEC ID（0x9F → EF 70 18 = W25Q128JV-IM；W25Q64 为 EF 40 17）
    {
        uint8_t cmd[4] = {0x9F, 0, 0, 0};
        uint8_t rsp[4];
        w25q_xfer(w25q_spi, cmd, rsp, 4);
        len = snprintf(buf, sizeof(buf), "JEDEC ID    : %02X %02X %02X  (%s)\r\n",
                       rsp[1], rsp[2], rsp[3],
                       (rsp[1] == 0xEF && rsp[3] == 0x18) ? "W25Q128 OK" : "UNKNOWN");
        tx(buf, len);
    }

    // 2. 状态寄存器（0x05）
    {
        uint8_t cmd[2] = {0x05, 0x00};
        uint8_t rsp[2];
        w25q_xfer(w25q_spi, cmd, rsp, 2);
        len = snprintf(buf, sizeof(buf), "Status Reg  : 0x%02X (WIP=%d)\r\n", rsp[1], rsp[1] & 0x01);
        tx(buf, len);
    }

    // 3. 读 0x000000 处 16 字节（芯片出厂默认 0xFF）
    {
        uint8_t cmd[20];
        uint8_t rsp[20];
        cmd[0] = 0x03; cmd[1] = 0; cmd[2] = 0; cmd[3] = 0;
        memset(&cmd[4], 0, 16);
        w25q_xfer(w25q_spi, cmd, rsp, 20);
        len = snprintf(buf, sizeof(buf), "Read 0x0000 : ");
        tx(buf, len);
        for (int i = 4; i < 20; i++)
        {
            len = snprintf(buf, sizeof(buf), "%02X ", rsp[i]);
            tx(buf, len);
        }
        txln("");
    }

    // 4. 擦写回环（最后扇区 0xFF0000，⚠️ 会擦除该扇区原有数据）
    txln("-- Erase/Write/Readback @ 0xFF0000 --");
    bool pass = true;
    const uint8_t wr_cmd = 0x06;   // 写使能

    // 4a. 扇区擦除（0x20 + 地址）
    {
        uint8_t cmd[4] = {0x20, 0xFF, 0x00, 0x00};
        w25q_xfer(w25q_spi, &wr_cmd, nullptr, 1);
        w25q_xfer(w25q_spi, cmd, nullptr, 4);
        if (!w25q_wait_idle(w25q_spi, 1000))
        { txln("Erase       : TIMEOUT"); pass = false; }
        else
        { txln("Erase       : OK"); }
    }

    // 4b. 页编程（0x02 + 地址 + 32 字节递增 pattern）
    const uint8_t test_len = 32;
    uint8_t pattern[32];
    uint8_t pcmd[4 + 32];
    if (pass)
    {
        for (int i = 0; i < test_len; i++) pattern[i] = (uint8_t)i;
        pcmd[0] = 0x02; pcmd[1] = 0xFF; pcmd[2] = 0x00; pcmd[3] = 0x00;
        memcpy(&pcmd[4], pattern, test_len);
        w25q_xfer(w25q_spi, &wr_cmd, nullptr, 1);
        w25q_xfer(w25q_spi, pcmd, nullptr, 4 + test_len);
        if (!w25q_wait_idle(w25q_spi, 1000))
        { txln("Program     : TIMEOUT"); pass = false; }
        else
        { txln("Program     : OK"); }
    }

    // 4c. 读回比对
    if (pass)
    {
        uint8_t rcmd[4 + 32];
        uint8_t rdata[4 + 32];
        rcmd[0] = 0x03; rcmd[1] = 0xFF; rcmd[2] = 0x00; rcmd[3] = 0x00;
        memset(&rcmd[4], 0, test_len);
        w25q_xfer(w25q_spi, rcmd, rdata, 4 + test_len);

        bool match = true;
        for (int i = 0; i < test_len; i++)
            if (rdata[4 + i] != pattern[i]) { match = false; break; }

        len = snprintf(buf, sizeof(buf), "Readback    : %s\r\n", match ? "MATCH (32B verified)" : "MISMATCH");
        tx(buf, len);
        if (!match)
        {
            txln("  written: ");
            for (int i = 0; i < test_len; i++) { len = snprintf(buf, sizeof(buf), "%02X ", pattern[i]); tx(buf, len); }
            txln("");
            txln("  read:    ");
            for (int i = 0; i < test_len; i++) { len = snprintf(buf, sizeof(buf), "%02X ", rdata[4 + i]); tx(buf, len); }
            txln("");
        }
        pass = match;
    }

    len = snprintf(buf, sizeof(buf), "RESULT      : %s\r\n", pass ? "PASS" : "FAIL");
    tx(buf, len);
    txln("========== W25Q64 SOFT-SPI TEST DONE ==========\r\n");
}

/* ── W25Q128 硬件 SPI 验证（device 驱动 spi_port + w25q128 实测） ── */
/* SPI4: CS=PF6, CLK=PF7, MISO=PF8, MOSI=PF9, AF5, MODE3, PSC2, MSB, 8bit
   与旧 C 驱动 hard_w25q128 相同配置；软 SPI 测试为 MODE0（W25Q 两种模式均支持） */
static spi_port w25q_hw_spi({
    spi_id::spi4,
    GPIOF, GPIO_PIN_7,        // SCK
    GPIOF, GPIO_PIN_9,        // MOSI
    GPIOF, GPIO_PIN_8,        // MISO
    GPIOF, GPIO_PIN_6,        // CS
    RESET,                    // CS 低有效
    GPIO_AF_5,
    SPI_PSC_2,
    SPI_CK_PL_HIGH_PH_2EDGE,  // MODE3 (CPOL=1, CPHA=1)
    SPI_ENDIAN_MSB,
    SPI_FRAMESIZE_8BIT
});
static w25q128 w25q_dev(w25q_hw_spi);

static void w25q128_hw_selftest(void)
{
    char buf[160];
    int  len;

    auto tx   = [&](const char *s, int n) { (void)suart.send_data((const uint8_t *)s, (uint16_t)n); };
    auto txln = [&](const char *s) {
        (void)suart.send_data((const uint8_t *)s, (uint16_t)strlen(s));
        (void)suart.send_data((const uint8_t *)"\r\n", 2);
    };

    txln("\r\n========== W25Q128 HARDWARE-SPI TEST ==========");
    w25q_hw_spi.init();
    w25q_dev.init();

    // 1. JEDEC ID（0x9F → EF 70 18 = W25Q128JV-IM；W25Q64 为 EF 40 17）
    {
        uint32_t id = w25q_dev.jedec_id();
        len = snprintf(buf, sizeof(buf), "JEDEC ID    : %02X %02X %02X  (%s)\r\n",
                       (uint8_t)(id >> 16), (uint8_t)(id >> 8), (uint8_t)(id & 0xFF),
                       (id == 0xEF7018u) ? "W25Q128 OK" : "UNKNOWN");
        tx(buf, len);
    }

    // 2. 状态寄存器（0x05）
    {
        uint8_t st = w25q_dev.read_status();
        len = snprintf(buf, sizeof(buf), "Status Reg  : 0x%02X (WIP=%d)\r\n", st, st & 0x01);
        tx(buf, len);
    }

    // 3. 读 0x000000 处 16 字节（芯片出厂默认 0xFF）
    {
        uint8_t rdata[16];
        w25q_dev.read(0x000000, rdata, 16);
        txln("Read 0x0000 : ");
        for (int i = 0; i < 16; i++) { len = snprintf(buf, sizeof(buf), "%02X ", rdata[i]); tx(buf, len); }
        txln("");
    }

    // 4. 擦写回环（最后扇区 0xFF0000，⚠️ 会擦除该扇区原有数据）
    txln("-- Erase/Write/Readback @ 0xFF0000 --");
    const uint8_t test_len = 32;
    uint8_t pattern[32];
    uint8_t rdata2[32];
    for (int i = 0; i < test_len; i++) pattern[i] = (uint8_t)i;

    w25q_dev.sector_erase(0xFF0000);
    w25q_dev.write(0xFF0000, pattern, test_len);
    w25q_dev.read(0xFF0000, rdata2, test_len);

    bool match = true;
    for (int i = 0; i < test_len; i++)
        if (rdata2[i] != pattern[i]) { match = false; break; }

    len = snprintf(buf, sizeof(buf), "Readback    : %s\r\n", match ? "MATCH (32B verified)" : "MISMATCH");
    tx(buf, len);
    if (!match)
    {
        txln("  written: ");
        for (int i = 0; i < test_len; i++) { len = snprintf(buf, sizeof(buf), "%02X ", pattern[i]); tx(buf, len); }
        txln("");
        txln("  read:    ");
        for (int i = 0; i < test_len; i++) { len = snprintf(buf, sizeof(buf), "%02X ", rdata2[i]); tx(buf, len); }
        txln("");
    }

    len = snprintf(buf, sizeof(buf), "RESULT      : %s\r\n", match ? "PASS" : "FAIL");
    tx(buf, len);
    txln("========== W25Q128 HARDWARE-SPI TEST DONE ==========\r\n");
}

/* ── EXTI 自测（软件触发验证 ISR 链路 + 线路互斥 + 系统线） ── */
static void exti_selftest(void)
{
    char buf[128];
    int  len;
    static volatile uint32_t s_fire_count = 0;

    auto tx = [&](const char *s, int n) { (void)suart.send_data((const uint8_t *)s, (uint16_t)n); };
    auto txln = [&](const char *s) {
        (void)suart.send_data((const uint8_t *)s, (uint16_t)strlen(s));
        (void)suart.send_data((const uint8_t *)"\r\n", 2);
    };

    txln("\r\n========== EXTI SELF TEST ==========");

    // 1. GPIO 线：PA0 上升沿
    static exti_port exti_pa0({.port = GPIOA, .pin = GPIO_PIN_0,
                               .trigger = exti_trigger::rising});
    bool ok1 = exti_pa0.init();
    exti_pa0.on_interrupt([](void*) { s_fire_count = s_fire_count + 1; });
    exti_pa0.enable();

    // 2. 线路互斥：PB0 与 PA0 同为 EXTI 线 0，应注册失败
    static exti_port exti_pb0({.port = GPIOB, .pin = GPIO_PIN_0});
    bool ok2 = exti_pb0.init();

    len = snprintf(buf, sizeof(buf), "init PA0=%d  init PB0(same line)=%d  (expect 1,0)\r\n", ok1, ok2);
    tx(buf, len);

    // 3. 软件触发 3 次，验证 ISR 回调执行
    for (int i = 0; i < 3; i++)
    {
        exti_pa0.software_trigger();
        delay_ms(50);
    }
    len = snprintf(buf, sizeof(buf), "software trigger x3: fired=%lu (expect 3)\r\n",
                   (unsigned long)s_fire_count);
    tx(buf, len);

    // 4. LVD 系统线：配置成功性（不实际触发）
    static exti_port exti_lvd({.line_type = exti_line_id::lvd});
    bool ok3 = exti_lvd.init();
    len = snprintf(buf, sizeof(buf), "LVD line init=%d (expect 1)\r\n", ok3);
    tx(buf, len);

    txln("========== EXTI SELF TEST DONE ==========\r\n");
}

/* ── ADC 自测（温度传感器 CH16 + 内部参考 CH17，无需外部接线） ── */
static adc_port adc_temp({
    .periph           = adc_id::adc0,
    .scan_enable      = true,
    .enable_temp_vref = true,
    .channels         = {{.channel = ADC_CHANNEL_16, .sample_time = ADC_SAMPLETIME_480},
                         {.channel = ADC_CHANNEL_17, .sample_time = ADC_SAMPLETIME_480}},
    .channel_count    = 2,
    .use_dma          = true,   // DMA 连续采集（GD32F470: ADC0 -> DMA1 CH0 SUBPERI0，官方例程映射）
});

static void adc_selftest(void)
{
    char buf[128];
    int  len;

    auto tx = [&](const char *s, int n) { (void)suart.send_data((const uint8_t *)s, (uint16_t)n); };
    auto txln = [&](const char *s) {
        (void)suart.send_data((const uint8_t *)s, (uint16_t)strlen(s));
        (void)suart.send_data((const uint8_t *)"\r\n", 2);
    };

    txln("\r\n========== ADC SELF TEST (temp/vref channels) ==========");
    adc_temp.init();
    // 诊断：init 后查看 SYNCCTL（TSVREN 应置位）+ 序列寄存器 RSQ2（SQ1/SQ2）
    char dbg[96];
    int dlen = snprintf(dbg, sizeof(dbg), "[ADC] SYNCCTL=0x%08lX RSQ2=0x%08lX\r\n",
                        (unsigned long)ADC_SYNCCTL,
                        (unsigned long)ADC_RSQ2(ADC0));
    tx(dbg, dlen);

    // 连续 5 次扫描：温度通道原始值 + 内部参考（应约 1.2V）
    for (int i = 0; i < 5; i++)
    {
        uint32_t mv[2];
        adc_temp.scan_mv(mv);
        // GD32F470: Vtemp@25°C=1.43V, 斜率 -4.3mV/°C → T = (1430 - mv)/4.3 + 25
        int temp = (int)((1430 - (int)mv[0]) * 10 / 43) + 25;
        len = snprintf(buf, sizeof(buf),
                       "scan%d: TEMP=%ldmV(%dC)  VREFINT=%ldmV (expect ~1200)\r\n",
                       i + 1, (long)mv[0], temp, (long)mv[1]);
        tx(buf, len);
        delay_ms(50);
    }

    // 单通道读取兼容性（CH16 + CH17 分别读）
    uint16_t raw = adc_temp.read_raw(0);
    uint16_t raw17 = adc_temp.read_raw(1);
    len = snprintf(buf, sizeof(buf), "read_raw(CH16) = %u, read_raw(CH17) = %u\r\n", raw, raw17);
    tx(buf, len);

    // scan 原始值打印（应 [0]=CH16~1700, [1]=CH17~1470）
    uint16_t sraw[2];
    adc_temp.scan_raw(sraw);
    len = snprintf(buf, sizeof(buf), "scan_raw: [0]=%u (CH16), [1]=%u (CH17)\r\n", sraw[0], sraw[1]);
    tx(buf, len);

    // DMA 连续采集（对齐师傅：连续转换 + DMA 循环搬运）

    // DMA 连续采集（对齐师傅：连续转换 + DMA 循环搬运）
    static uint16_t dma_buf[2 * 10];   // 2 通道 × 10 帧
    bool dma_ok = adc_temp.dma_start(dma_buf, 10);
    if (dma_ok)
    {
        delay_ms(50);
        // 诊断：DMA1 CH0 CTL（PERIEN=bit25-27, CHEN=bit0）、PADDR/M0ADDR、ADC0 CTL1（DMA=bit8）
        len = snprintf(buf, sizeof(buf),
                       "DMA DBG: CHCTL=0x%08lX CNT=0x%08lX PADDR=0x%08lX M0ADDR=0x%08lX\r\n"
                       "DMA DBG2: ADC_CTL1=0x%08lX SYNCCTL=0x%08lX\r\n",
                       (unsigned long)DMA_CHCTL(DMA1, DMA_CH0),
                       (unsigned long)DMA_CHCNT(DMA1, DMA_CH0),
                       (unsigned long)DMA_CHPADDR(DMA1, DMA_CH0),
                       (unsigned long)DMA_CHM0ADDR(DMA1, DMA_CH0),
                       (unsigned long)ADC_CTL1(ADC0),
                       (unsigned long)ADC_SYNCCTL);
        tx(buf, len);
        // 等首轮完成 + 多等几轮（连续模式持续刷新）
        uint32_t t0 = get_tick();
        while (!adc_temp.dma_done() && get_tick() - t0 < 500) {}
        delay_ms(100);   // 让循环模式多搬几轮
        len = snprintf(buf, sizeof(buf),
                       "DMA: done=%d frame0=[%u,%u] frame9=[%u,%u] (expect ~1700/~1470)\r\n",
                       adc_temp.dma_done(),
                       dma_buf[0], dma_buf[1], dma_buf[18], dma_buf[19]);
        tx(buf, len);
        adc_temp.dma_stop();
    }
    else
    {
        txln("DMA: start FAILED");
    }
    txln("========== ADC SELF TEST DONE ==========\r\n");
}

/* ── 软串口最高波特率测试（一次性，自检后运行） ──────────── */
static const uint32_t s_baud_list[] = {
    9600, 19200, 38400, 57600, 115200, 230400, 460800,
    921600, 1382400, 1843200, 2000000, 2400000, 3000000, 4000000
};

static void suart_baud_test(void)
{
    char buf[64];

    // 阶段 1：115200 下逐波特率测量实际位率（DWT 计时 1 字节 = 10 bit）
    suart.set_baud(115200);
    suart.send_data((const uint8_t *)"\r\n-- SUART BAUD MEASURE --\r\n", 26);
    for (auto baud : s_baud_list)
    {
        suart.set_baud(baud);
        uint32_t t0 = DWT->CYCCNT;
        suart.putc(0xA5);
        uint32_t t1 = DWT->CYCCNT;
        uint32_t actual = (uint32_t)(10ULL * SystemCoreClock / (t1 - t0));

        suart.set_baud(115200);
        int len = snprintf(buf, sizeof(buf), "B%-8lu: actual=%lu baud\r\n",
                           (unsigned long)baud, (unsigned long)actual);
        suart.send_data((const uint8_t *)buf, len);
    }

    // 阶段 2：逐波特率输出扫描标记 × 3 轮（PC 端任意时刻加入，每波特率 1.8s 窗口必命中）
    suart.send_data((const uint8_t *)"\r\nSCAN START\r\n", 14);
    delay_ms(1000);
    for (int round = 0; round < 3; round++)
    {
        for (auto baud : s_baud_list)
        {
            suart.set_baud(baud);
            for (int i = 0; i < 3; i++)
            {
                int len = snprintf(buf, sizeof(buf), "SCAN B%lu\r\n", (unsigned long)baud);
                suart.send_data((const uint8_t *)buf, len);
                delay_ms(200);
            }
        }
    }
    suart.set_baud(115200);
    suart.send_data((const uint8_t *)"\r\nSCAN DONE\r\n", 14);
}
