/**
 * @file    protocol_string.cpp
 * @brief   字符串指令协议 —— handler / 命令表 / 解析引擎
 *
 * 仅当 ACTIVE_PROTOCOL == PROTOCOL_STRING_CMD 时编译。
 *
 * 使用方式:
 *   串口工具设置: 115200-8N1，勾选"追加回车换行"
 *   发送框输入:   ina228_bus_voltage（不要手动加 \r\n）
 */

#include "protocol.hpp"
#include "debug_port.hpp"
#include "usart_port.hpp"
#include "ina228.hpp"

#include <cstring>
#include <cstdio>

#if ACTIVE_PROTOCOL == PROTOCOL_STRING_CMD

static char       g_resp_buf[512];
extern INA228     INA228_2;

typedef void (*cmd_handler_t)(void);

typedef struct {
    const char   *cmd;
    uint8_t       cmd_len;
    cmd_handler_t handler;
} str_cmd_t;

/* ============================================================
 *  INA228 Handler —— 全部使用 %ld（newlib-nano 不支持 %lld）
 * ============================================================ */

#define RESP(fmt, ...) \
    snprintf(g_resp_buf, sizeof(g_resp_buf), fmt "\r\n", ##__VA_ARGS__)

static void h_bus_voltage(void) {
    long v = (long)INA228_2.getBusVoltage_mV() * 1000;
    RESP("INA228_BusVoltage=%ld_uV", v);
}
static void h_shunt_voltage(void) {
    RESP("INA228_ShuntVoltage=%ld_uV", (long)INA228_2.getShuntVoltage_uV());
}
static void h_current(void) {
    RESP("INA228_Current=%ld_mA", (long)INA228_2.getCurrent_mA());
}
static void h_power(void) {
    RESP("INA228_Power=%ld_mW", (long)INA228_2.getPower_mW());
}
static void h_temperature(void) {
    RESP("INA228_Temperature=%ld_mC", (long)INA228_2.getTemperature_mC());
}
static void h_energy(void) {
    RESP("INA228_Energy=%ld_mJ", (long)INA228_2.getEnergy_mJ());
}
static void h_charge(void) {
    RESP("INA228_Charge=%ld_mC", (long)INA228_2.getCharge_mC());
}
static void h_mfr_id(void) {
    RESP("INA228_MfrID=0x%04X", INA228_2.getManufacturerID());
}
static void h_device_id(void) {
    RESP("INA228_DeviceID=0x%04X", INA228_2.getDeviceID());
}
static void h_alert(void) {
    INA228_AlertFlags f = INA228_2.readAlertFlags();
    snprintf(g_resp_buf, sizeof(g_resp_buf),
        "INA228_Alert=0x%04X OV:%d UV:%d BOV:%d BUV:%d PWR:%d TMP:%d RDY:%d\r\n",
        INA228_2.readDiagAlertRaw(), f.shuntOverLimit, f.shuntUnderLimit,
        f.busOverLimit, f.busUnderLimit, f.powerOverLimit, f.tempOverLimit, f.conversionReady);
}
static void h_clear_alert(void) {
    INA228_2.clearAlertLatch();
    RESP("INA228_Alert_Cleared");
}
static void h_all(void) {
    long vm = INA228_2.getBusVoltage_mV();
    long sv = INA228_2.getShuntVoltage_uV();
    long im = INA228_2.getCurrent_mA();
    long pm = INA228_2.getPower_mW();
    long tm = INA228_2.getTemperature_mC();
    long em = (long)INA228_2.getEnergy_mJ();
    long cm = (long)INA228_2.getCharge_mC();
    snprintf(g_resp_buf, sizeof(g_resp_buf),
        "=== INA228 ===\r\n"
        "Bus=%ldmV(%lduV)\r\nShunt=%lduV\r\nCur=%ldmA\r\n"
        "Pwr=%ldmW\r\nTemp=%ldmC\r\nEnergy=%ldmJ\r\nCharge=%ldmC\r\n"
        "Mfr=0x%04X Dev=0x%04X Alert=0x%04X\r\n",
        vm, vm * 1000, sv, im, pm, tm, em, cm,
        INA228_2.getManufacturerID(), INA228_2.getDeviceID(), INA228_2.readDiagAlertRaw());
}

static void ch_all_open(void)  {}
static void ch_all_close(void) {}
static void ch_open(uint8_t c)  { (void)c; }
static void ch_close(uint8_t c) { (void)c; }

#undef RESP

/* ============================================================
 *  命令表
 * ============================================================ */

#define CMD(str, handler)  {str, (uint8_t)(sizeof(str)-1), handler}

static const str_cmd_t cmd_table[] = {
    CMD("ina228_bus_voltage\r\n",      h_bus_voltage),
    CMD("ina228_shunt_voltage\r\n",    h_shunt_voltage),
    CMD("ina228_current\r\n",          h_current),
    CMD("ina228_power\r\n",            h_power),
    CMD("ina228_temperature\r\n",      h_temperature),
    CMD("ina228_energy\r\n",           h_energy),
    CMD("ina228_charge\r\n",           h_charge),
    CMD("ina228_mfr_id\r\n",           h_mfr_id),
    CMD("ina228_device_id\r\n",        h_device_id),
    CMD("ina228_alert\r\n",            h_alert),
    CMD("ina228_clear_alert\r\n",      h_clear_alert),
    CMD("ina228_all\r\n",              h_all),
};

#undef CMD

/* ============================================================
 *  命令解析
 * ============================================================ */

static bool parse_channel_command(const char *cmd, char *resp, size_t sz)
{
    if (strncmp(cmd, "Channel", 7)) return false;
    const char *p = cmd + 7;
    if (!strncmp(p, "_all_open\r\n", 11))  { ch_all_open();  snprintf(resp, sz, "Channel_all_open\r\n");  return true; }
    if (!strncmp(p, "_all_close\r\n", 12)) { ch_all_close(); snprintf(resp, sz, "Channel_all_close\r\n"); return true; }
    if (p[0]>='0'&&p[0]<='9'&&p[1]>='0'&&p[1]<='9') {
        int id = (p[0]-'0')*10 + (p[1]-'0');
        if (id>=1 && id<=16) {
            p+=2;
            if (!strncmp(p,"_open",5))  { ch_open((uint8_t)id);  snprintf(resp,sz,"Ch%02d_open\r\n",id);  return true; }
            if (!strncmp(p,"_close",6)) { ch_close((uint8_t)id); snprintf(resp,sz,"Ch%02d_close\r\n",id); return true; }
        }
    }
    return false;
}

/* ============================================================
 *  主轮询
 * ============================================================ */

/** @brief 字符串指令协议初始化（当前无需操作，保留接口供后续扩展） */
void StringCmd_Init(void) {}

void StringCmd_Task(void)
{
    uart_buffer_t *buf = debug_uart.buffer();
    if (buf->rx_flag != 1) return;

    uint16_t len = buf->rx_len;
    if (len < 4) { buf->rx_len = 0; buf->rx_flag = 0; return; }

    char recv[256];
    uint16_t n = (len < sizeof(recv) - 1) ? len : (uint16_t)(sizeof(recv) - 1);
    memcpy(recv, buf->rx_buf, n);
    recv[n] = '\0';
    buf->rx_len  = 0;
    buf->rx_flag = 0;

    bool ok = false;
    char resp[128] = {0};

    ok = parse_channel_command(recv, resp, sizeof(resp));

    if (!ok) {
        for (size_t j = 0; j < sizeof(cmd_table) / sizeof(cmd_table[0]); j++) {
            if (!memcmp(recv, cmd_table[j].cmd, cmd_table[j].cmd_len)) {
                cmd_table[j].handler();
                ok = true; break;
            }
        }
    }

    if (ok) {
        const char *reply = resp[0] ? resp : g_resp_buf;
        debug_uart.send_data((const uint8_t *)reply, (uint16_t)strlen(reply));
    }

    memset(g_resp_buf, 0, sizeof(g_resp_buf));
}

#endif
