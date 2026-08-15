#include "device_ina228.hpp"
#include "device_serial.hpp"
#include "founction.hpp"
#include "inter_i2c_bus.hpp"
#include "inter_i2c_dev.hpp"
#include "inter_io_ctrl.hpp"
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

io_ctrl led1(GPIOC, GPIO_PIN_4);
io_ctrl led2(GPIOC, GPIO_PIN_5);
io_ctrl led3(GPIOA, GPIO_PIN_7);

/* ── 初始化 ──────────────────────────────────────────── */
static void ina228_selftest(void);   // 定义见下方
static void w25q64_selftest(void);   // 定义见下方
void function_init(void)
{
    // ⚠️ 测试模式：硬件 debug_uart 与软串口同引脚(PA9/10)，临时禁用硬件
    // DebugPort_Init();
    suart.init();
    i2c_bus.init();
    INA228_2.init();
    led1.init(GPIO_MODE_OUTPUT, GPIO_PUPD_NONE, GPIO_OTYPE_PP, GPIO_OSPEED_50MHZ);
    led2.init(GPIO_MODE_OUTPUT, GPIO_PUPD_NONE, GPIO_OTYPE_PP, GPIO_OSPEED_50MHZ);
    led3.init(GPIO_MODE_OUTPUT, GPIO_PUPD_NONE, GPIO_OTYPE_PP, GPIO_OSPEED_50MHZ);
    Protocol_Init();
    ina228_selftest();
    // ⚠️ 自检会覆盖/复位阈值寄存器（第 4/5/7 步），业务阈值必须在自检之后设置
    INA228_2.set_bovl_limit_mV(5000);
    w25q64_selftest();
    (void)suart.send_data((const uint8_t *)"\r\n[BOOT] Ready\r\n", 16);
    // INA228_2.
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
#if ACTIVE_PROTOCOL == PROTOCOL_MODBUS
    // INA228 数据每 100ms 刷新一次（软 I2C 读取较慢，若每循环刷新会把
    // 主循环拖到 ~1ms，导致软串口 poll 错过字节起始位）
    static uint32_t s_mb_last = 0;
    if (get_tick() - s_mb_last >= 100)
    {
        s_mb_last = get_tick();
        UpdateModbusRegisters();
    }
#endif
    led1.toggle();
    led2.toggle();

    // ── 软串口接收轮询（Modbus 帧填充 + 3.5 字符空闲判帧） ──
    suart.poll();

    // ── INA228 周期采样（每秒一次，测试观察用） ──
    static uint32_t s_last_ms = 0;
    if (get_tick() - s_last_ms >= 1000)
    {
        s_last_ms = get_tick();

        char    buf[96];
        int32_t vbus = INA228_2.getBusVoltage_mV();
        int32_t cur  = INA228_2.getCurrent_mA();
        int32_t pwr  = INA228_2.getPower_mW();
        int32_t tmp  = INA228_2.getTemperature_mC();
        bool    alert = INA228_2.isAlertAsserted();

        int len = snprintf(buf, sizeof(buf),
                           "[INA228] Vbus=%ld.%03ldV I=%ld.%03ldA P=%ld.%03ldW T=%ld.%ldC ALERT=%s DIAG=0x%04X\r\n",
                           (long)vbus / 1000, (long)vbus % 1000,
                           (long)cur / 1000, (long)cur % 1000,
                           (long)pwr / 1000, (long)pwr % 1000,
                           (long)tmp / 1000, (long)(tmp % 1000) / 100,
                           alert ? "LOW" : "HIGH", INA228_2.readDiagAlertRaw());
        (void)suart.send_data((const uint8_t *)buf, len);
    }
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
