#include "ina228.hpp"
#include "link.h"

#include <cstdint>
#include <cstdio>
#include <set>

// ============================================================
// Constructor
// ============================================================

INA228::INA228(device_i2c_bus* bus,
               uint8_t addr)
    : _dev(bus, addr)
{
    _currentLSB_uA = _cfg.maxCurrent_mA * 1000 >> 19;
}

INA228::INA228(device_i2c_bus* bus,
               uint8_t addr,
               const Config& cfg)
    : _dev(bus, addr)
    , _cfg(cfg)
{
    _currentLSB_uA = _cfg.maxCurrent_mA * 1000 >> 19;
}

// ============================================================
// Init
// ============================================================

void INA228::init()
{
    writeRaw(REG_CONFIG,
             buildConfigReg(),
             2);

    writeRaw(REG_ADC_CONFIG,
             buildADCConfigReg(),
             2);
    writeRaw(REG_DIAG_ALERT,
             buildDiagConfigReg(),
             2);

    writeRaw(REG_SHUNT_CAL,
             _cfg.shuntCal,
             2);
    set_sovl_limit_mA(100);
    // set_suvl_limit_mA(70);
    set_bovl_limit_mV(12000);
    set_buvl_limit_mV(8000);
}

// ============================================================
// Register Length
// ============================================================

uint8_t INA228::regLen(uint8_t reg)
{
    switch (reg)
    {
    case REG_VSHUNT:
    case REG_VBUS:
    case REG_CURRENT:
    case REG_POWER:
        return 3;

    case REG_ENERGY:
    case REG_CHARGE:
        return 5;

    default:
        return 2;
    }
}

// ============================================================
// Raw IO
// ============================================================

void INA228::writeRaw(uint8_t reg,
                      uint64_t data,
                      uint8_t len)
{
    _dev.i2c_freedom_write(reg, data, len);
}

uint64_t INA228::readRaw(uint8_t reg)
{
    uint64_t data = 0;

    _dev.i2c_freedom_read(reg,
                          &data,
                          regLen(reg));

    return data;
}

// ============================================================
// Register Build
// ============================================================

uint16_t INA228::buildConfigReg() const
{
    uint16_t r = 0;

    r |= static_cast<uint16_t>(_cfg.reset) << 15;
    r |= static_cast<uint16_t>(_cfg.retacc) << 14;
    r |= static_cast<uint16_t>(_cfg.convdly) << 6;
    r |= static_cast<uint16_t>(_cfg.tempcomp) << 5;
    r |= static_cast<uint16_t>(_cfg.adcrange) << 4;
    r |= static_cast<uint16_t>(_cfg.reserved) << 0;

    return r;
}

uint16_t INA228::buildADCConfigReg() const
{
    uint16_t r = 0;

    r |= static_cast<uint16_t>(_cfg.adcMode) << 12;

    r |= static_cast<uint16_t>(_cfg.busConvTime) << 9;

    r |= static_cast<uint16_t>(_cfg.shuntConvTime) << 6;

    r |= static_cast<uint16_t>(_cfg.tempConvTime) << 3;

    r |= static_cast<uint16_t>(_cfg.averaging);

    return r;
}

uint16_t INA228::buildDiagConfigReg() const
{
    uint16_t r = 0;

    r |= static_cast<uint16_t>(_cfg.alatch) << 15;
    r |= static_cast<uint16_t>(_cfg.cnvr) << 14;
    r |= static_cast<uint16_t>(_cfg.slowalert) << 13;
    r |= static_cast<uint16_t>(_cfg.apol) << 12;
    r |= static_cast<uint16_t>(_cfg.energyof) << 11;
    r |= static_cast<uint16_t>(_cfg.chargeof) << 10;
    r |= static_cast<uint16_t>(_cfg.mathof) << 9;
    r |= static_cast<uint16_t>(_cfg.dig_reserved) << 8;
    r |= static_cast<uint16_t>(_cfg.tmpol) << 7;
    r |= static_cast<uint16_t>(_cfg.shntol) << 6;
    r |= static_cast<uint16_t>(_cfg.shntul) << 5;
    r |= static_cast<uint16_t>(_cfg.vbus_ol) << 4;
    r |= static_cast<uint16_t>(_cfg.vbus_ul) << 3;
    r |= static_cast<uint16_t>(_cfg.pow_ol) << 2;
    r |= static_cast<uint16_t>(_cfg.cnvrf) << 1;
    r |= static_cast<uint16_t>(_cfg.menstat) << 0;
    return r;
}

// ============================================================
// Convert
// ============================================================

int32_t INA228::rawToBusVoltage_mV(uint64_t raw)
{
    /*
        Bus Voltage LSB:195.3125uV = 1953125 / 10000 uV
        转 mV: value * 1953125 / 10000000
        近似优化：195.3125uV = 25 / 128 mV
        所以：mV = value * 25 >> 7
    */
    int32_t val =
        signExtend<20>(raw >> 4);

    return (val * 25) >> 7;
}

int32_t INA228::rawToShuntVoltage_uV(uint64_t raw)
{
    /*
        312.5nV = 5 / 16 uV
    */

    int32_t val =
        signExtend<20>(raw >> 4);

    return (val * 5) >> 4;
}

int32_t INA228::rawToCurrent_mA(uint64_t raw) const
{
    int32_t val =
        signExtend<20>(raw >> 4);

    /*
        current_uA =
            raw * currentLSB_uA

        转 mA:
            /1000
    */

    return (val * _currentLSB_uA) / 1000;
}

int32_t INA228::rawToTemperature_mC(uint64_t raw)
{
    /*
        7.8125m°C

        = 1/128 °C

        转 m°C:

        1000 / 128
    */

    uint16_t val =
        static_cast<uint16_t>(raw);

    return (val * 1000) >> 7;
}

int32_t INA228::rawToPower_mW(uint64_t raw) const {
  /*
      Power LSB:Power_LSB(W) = 3.2 × Current_LSB(A)
      3.2 = 16/5
      power_W  = raw × 16/5 × Current_LSB_A
               = raw × 16/5 × (currentLSB_uA / 1000000)
      power_mW = power_W × 1000
               = raw × 16 × currentLSB_uA / 5000
  */

  int64_t val = static_cast<int64_t>(raw);

  return static_cast<int32_t>((val * 16 * _currentLSB_uA) / 5000);
}

int64_t INA228::rawToEnergy_mJ(uint64_t raw) const
{
    /*
        Energy[J] = 16 × 3.2 × CURRENT_LSB × ENERGY
                  = 51.2 × Current_LSB_A × ENERGY_reg
        51.2 = 256/5
        Energy_mJ = Energy_J × 1000
                  = ENERGY_reg × 256 × currentLSB_uA / 5000
                  = ENERGY_reg × 32 × currentLSB_uA / 625
    */

    int64_t val = signExtend<40>(raw);

    return (val * 32 * _currentLSB_uA) / 625;
}

int64_t INA228::rawToCharge_mC(uint64_t raw) const
{
    /*
        Charge[C] = CURRENT_LSB × CHARGE
        Charge_mC = Charge_C × 1000
                  = CHARGE_reg × currentLSB_uA / 1000
    */

    int64_t val = signExtend<40>(raw);

    return (val * _currentLSB_uA) / 1000;
}

// ============================================================
// Public API
// ============================================================

int32_t INA228::getBusVoltage_mV()
{
    return rawToBusVoltage_mV(
        readRaw(REG_VBUS));
}

int32_t INA228::getShuntVoltage_uV()
{
    return rawToShuntVoltage_uV(
        readRaw(REG_VSHUNT));
}

int32_t INA228::getCurrent_mA()
{
    return rawToCurrent_mA(
        readRaw(REG_CURRENT));
}

int32_t INA228::getTemperature_mC()
{
    return rawToTemperature_mC(
        readRaw(REG_DIETEMP));
}

int32_t INA228::getPower_mW()
{
    return rawToPower_mW(
        readRaw(REG_POWER));
}

int64_t INA228::getEnergy_mJ()
{
    return rawToEnergy_mJ(
        readRaw(REG_ENERGY));
}

int64_t INA228::getCharge_mC()
{
    return rawToCharge_mC(
        readRaw(REG_CHARGE));
}


/*
 * 设置分流过压保护阈值 (SOVL)
 *
 * sovl_mA:   过流阈值，单位 mA
 * 依赖:      _cfg.r_shunt_uOhm（采样电阻，单位 µΩ）
 *
 * 转换流程:  mA × µΩ = nV → µV → ÷LSB → 寄存器编码
 */
void INA228::set_sovl_limit_mA(int32_t sovl_mA)
{
    /*
     * 第一步：电流(mA) → 分流电压(µV)
     *
     * 单位推导：mA × µΩ = 10^-3 A × 10^-6 Ω = 10^-9 V = nV
     *          nV / 1000 = µV
     * 示例：10000mA × 2000µΩ = 20000000nV = 20000µV (即20mV)
     *
     * 使用 int64_t 防止乘法溢出
     * （极端情况：200000mA × 5000µΩ = 10^9，接近 int32 上限）
     */
    int64_t voltage_uV =                                          // 分流电压，µV
        (static_cast<int64_t>(sovl_mA) *
         static_cast<int64_t>(_cfg.r_shunt_uOhm)) / 1000;        // mA×µΩ=nV, ÷1000→µV

    /*
     * 第二步：µV → SOVL 寄存器编码
     *
     * reg = 电压 / LSB
     * ADCRANGE=0: LSB = 5µV,    除法即可
     * ADCRANGE=1: LSB = 1.25µV, 等价 ×4÷5（避免浮点，M0/M3 通常无 FPU）
     */
    int32_t reg_value = 0;

    if (_cfg.adcrange == Adcrange::Range_163mv) {
        reg_value = voltage_uV / 5;                               // 5µV/LSB
    } else {
        reg_value = (voltage_uV * 4) / 5;                         // 1.25µV/LSB = 5/4µV
    }

    // 钳位到 int16 范围 (-32768 ~ 32767)，防止写入时高位截断
    if (reg_value > INT16_MAX)  reg_value = INT16_MAX;
    if (reg_value < INT16_MIN)  reg_value = INT16_MIN;

    // 写入寄存器（16-bit 二进制补码，cast 保留原始位模式）
    writeRaw(REG_SOVL, static_cast<uint16_t>(reg_value), 2);
}


/*
 * 设置分流欠压保护阈值 (SUVL)
 *
 * suvl_mA:   欠流阈值，单位 mA
 * 依赖:      _cfg.r_shunt_uOhm（采样电阻，单位 µΩ）
 *
 * 转换流程:  mA × µΩ = nV → µV → ÷LSB → 寄存器编码
 *
 * 注：SUVL 复位值 0x8000（int16 最小值），即默认关闭欠压检测
 */
void INA228::set_suvl_limit_mA(int32_t suvl_mA)
{
    // 第一步：电流(mA) → 分流电压(µV)
    int64_t voltage_uV =                                          // 分流电压，µV
        (static_cast<int64_t>(suvl_mA) *
         static_cast<int64_t>(_cfg.r_shunt_uOhm)) / 1000;        // mA×µΩ=nV, ÷1000→µV

    // 第二步：µV → SUVL 寄存器编码
    int32_t reg_value = 0;

    if (_cfg.adcrange == Adcrange::Range_163mv) {
        reg_value = voltage_uV / 5;                               // 5µV/LSB
    } else {
        reg_value = (voltage_uV * 4) / 5;                         // 1.25µV/LSB = 5/4µV
    }

    // 钳位到 int16 范围 (-32768 ~ 32767)
    if (reg_value > INT16_MAX)  reg_value = INT16_MAX;
    if (reg_value < INT16_MIN)  reg_value = INT16_MIN;

    // 写入寄存器（16-bit 二进制补码）
    writeRaw(REG_SUVL, static_cast<uint16_t>(reg_value), 2);
}


/*
 * 设置总线过压保护阈值 (BOVL)
 *
 * bovl_mV:   过压阈值，单位 mV
 *
 * BOVL 寄存器：15-bit 无符号（bit15 保留为0），仅限正值
 * 转换因子：3.125mV/LSB = 25/8 mV/LSB
 * 为避免浮点：÷3.125 = ×8÷25
 * 钳位上限：0x7FFF（15-bit 无符号最大值，对应约 102.4V）
 */
void INA228::set_bovl_limit_mV(uint16_t bovl_mV)
{
    // mV → 寄存器编码：÷3.125 = ×8÷25
    int32_t reg_value = (static_cast<int32_t>(bovl_mV) * 8) / 25;

    // 钳位到 15-bit 无符号范围 (0 ~ 0x7FFF)，bit15 保留必须为 0
    if (reg_value > 0x7FFF)  reg_value = 0x7FFF;
    if (reg_value < 0)       reg_value = 0;

    writeRaw(REG_BOVL, static_cast<uint16_t>(reg_value), 2);
}

void INA228::set_buvl_limit_mV(uint16_t buvl_mV)
{
    // mV → 寄存器编码：÷3.125 = ×8÷25
    int32_t reg_value = (static_cast<int32_t>(buvl_mV) * 8) / 25;

    // 钳位到 15-bit 无符号范围 (0 ~ 0x7FFF)，bit15 保留必须为 0
    if (reg_value > 0x7FFF)  reg_value = 0x7FFF;
    if (reg_value < 0)       reg_value = 0;

    writeRaw(REG_BUVL, static_cast<uint16_t>(reg_value), 2);
}



void INA228::set_temp_limit_C(uint16_t temp_mC)
{
}

void INA228::set_pwr_limit_mW(uint32_t pwr_mW)
{
}

void INA228::unlock()
{
    readRaw(REG_DIAG_ALERT);
}

// ============================================================
// Device Info
// ============================================================

uint16_t INA228::getManufacturerID()
{
    return static_cast<uint16_t>(
        readRaw(REG_MANUFACTURER_ID));
}

uint16_t INA228::getDeviceID()
{
    return static_cast<uint16_t>(
        readRaw(REG_DEVICE_ID));
}

// ============================================================
// Dump
// ============================================================

void INA228::captureSnapshot(Snapshot& s)
{
    s.config        = readRaw(REG_CONFIG);
    s.adcConfig     = readRaw(REG_ADC_CONFIG);
    s.shuntCal      = readRaw(REG_SHUNT_CAL);
    s.diagAlert     = readRaw(REG_DIAG_ALERT);
    s.vshunt        = readRaw(REG_VSHUNT);
    s.vbus          = readRaw(REG_VBUS);
    s.temp          = readRaw(REG_DIETEMP);
    s.current       = readRaw(REG_CURRENT);
    s.power         = readRaw(REG_POWER);
    s.energy        = readRaw(REG_ENERGY);
    s.charge        = readRaw(REG_CHARGE);
    s.sovl          = static_cast<uint16_t>(readRaw(REG_SOVL));
    s.bovl          = static_cast<uint16_t>(readRaw(REG_BOVL));
    s.manufacturerID = static_cast<uint16_t>(readRaw(REG_MANUFACTURER_ID));
    s.deviceID       = static_cast<uint16_t>(readRaw(REG_DEVICE_ID));
}

void INA228::dumpRawRegisters(const Snapshot& s)
{
    char buf[128];
    int  len;

    // ---- Header ----
    len = snprintf(buf, sizeof(buf),
                   "\r\n========== INA228 RAW ==========\r\n");
    debug_send_data((uint8_t*)buf, len);

    // ---- 16-bit Registers ----
    len = snprintf(buf, sizeof(buf),
                   "CONFIG         : 0x%04X\r\n", s.config);
    debug_send_data((uint8_t*)buf, len);

    len = snprintf(buf, sizeof(buf),
                   "ADC_CONFIG     : 0x%04X\r\n", s.adcConfig);
    debug_send_data((uint8_t*)buf, len);

    len = snprintf(buf, sizeof(buf),
                   "SHUNT_CAL      : 0x%04X\r\n", s.shuntCal);
    debug_send_data((uint8_t*)buf, len);

    len = snprintf(buf, sizeof(buf),
                   "DIAG_ALERT     : 0x%04X\r\n", s.diagAlert);
    debug_send_data((uint8_t*)buf, len);

    len = snprintf(buf, sizeof(buf),
                   "DIETEMP        : 0x%04X\r\n", s.temp);
    debug_send_data((uint8_t*)buf, len);

    len = snprintf(buf, sizeof(buf),
                   "SOVL           : 0x%04X\r\n", s.sovl);
    debug_send_data((uint8_t*)buf, len);

    len = snprintf(buf, sizeof(buf),
                   "BOVL           : 0x%04X\r\n", s.bovl);
    debug_send_data((uint8_t*)buf, len);

    // ---- 24-bit Registers (stored in uint64_t, low 24 bits) ----
    len = snprintf(buf, sizeof(buf),
                   "VSHUNT         : 0x%06lX\r\n",
                   (unsigned long)(s.vshunt & 0xFFFFFF));
    debug_send_data((uint8_t*)buf, len);

    len = snprintf(buf, sizeof(buf),
                   "VBUS           : 0x%06lX\r\n",
                   (unsigned long)(s.vbus & 0xFFFFFF));
    debug_send_data((uint8_t*)buf, len);

    len = snprintf(buf, sizeof(buf),
                   "CURRENT        : 0x%06lX\r\n",
                   (unsigned long)(s.current & 0xFFFFFF));
    debug_send_data((uint8_t*)buf, len);

    len = snprintf(buf, sizeof(buf),
                   "POWER          : 0x%06lX\r\n",
                   (unsigned long)(s.power & 0xFFFFFF));
    debug_send_data((uint8_t*)buf, len);

    // ---- 40-bit Registers (split into high 8-bit + low 32-bit) ----
    len = snprintf(buf, sizeof(buf),
                   "ENERGY         : 0x%02lX%08lX\r\n",
                   (unsigned long)((s.energy >> 32) & 0xFF),
                   (unsigned long)(s.energy & 0xFFFFFFFF));
    debug_send_data((uint8_t*)buf, len);

    len = snprintf(buf, sizeof(buf),
                   "CHARGE         : 0x%02lX%08lX\r\n",
                   (unsigned long)((s.charge >> 32) & 0xFF),
                   (unsigned long)(s.charge & 0xFFFFFFFF));
    debug_send_data((uint8_t*)buf, len);

    // ---- Device Info (16-bit) ----
    len = snprintf(buf, sizeof(buf),
                   "MANUFACTURER_ID: 0x%04X\r\n", s.manufacturerID);
    debug_send_data((uint8_t*)buf, len);

    len = snprintf(buf, sizeof(buf),
                   "DEVICE_ID      : 0x%04X\r\n", s.deviceID);
    debug_send_data((uint8_t*)buf, len);
}

void INA228::dumpEngineeringData(const Snapshot& s)
{
    char buf[128];
    int  len;

    int32_t vshunt_uV = rawToShuntVoltage_uV(s.vshunt);
    int32_t vbus_mV   = rawToBusVoltage_mV(s.vbus);
    int32_t temp_mC   = rawToTemperature_mC(s.temp);
    int32_t cur_mA    = rawToCurrent_mA(s.current);
    int32_t pwr_mW    = rawToPower_mW(s.power);
    int64_t eng_mJ    = rawToEnergy_mJ(s.energy);
    int64_t chg_mC    = rawToCharge_mC(s.charge);

    // ---- Header ----
    len = snprintf(buf, sizeof(buf),
                   "\r\n========== INA228 ENGINEERING ==========\r\n");
    debug_send_data((uint8_t*)buf, len);

    // ---- Bus Voltage ----
    len = snprintf(buf, sizeof(buf),
                   "Bus Voltage    : %ld.%03ld V\r\n",
                   vbus_mV / 1000,
                   vbus_mV % 1000);
    debug_send_data((uint8_t*)buf, len);

    // ---- Shunt Voltage ----
    len = snprintf(buf, sizeof(buf),
                   "Shunt Voltage  : %ld uV\r\n",
                   vshunt_uV);
    debug_send_data((uint8_t*)buf, len);

    // ---- Temperature ----
    len = snprintf(buf, sizeof(buf),
                   "Temperature    : %ld.%03ld C\r\n",
                   temp_mC / 1000,
                   temp_mC % 1000);
    debug_send_data((uint8_t*)buf, len);

    // ---- Current ----
    len = snprintf(buf, sizeof(buf),
                   "Current        : %ld.%03ld A\r\n",
                   cur_mA / 1000,
                   cur_mA % 1000);
    debug_send_data((uint8_t*)buf, len);

    // ---- Power ----
    len = snprintf(buf, sizeof(buf),
                   "Power          : %ld.%03ld W\r\n",
                   pwr_mW / 1000,
                   pwr_mW % 1000);
    debug_send_data((uint8_t*)buf, len);

    // ---- Energy (mJ → split as J.mJ) ----
    len = snprintf(buf, sizeof(buf),
                   "Energy         : %ld.%03ld J\r\n",
                   (long)(eng_mJ / 1000),
                   (long)((eng_mJ % 1000) < 0 ? -(eng_mJ % 1000) : (eng_mJ % 1000)));
    debug_send_data((uint8_t*)buf, len);

    // ---- Charge (mC → split as C.mC) ----
    len = snprintf(buf, sizeof(buf),
                   "Charge         : %ld.%03ld C\r\n",
                   (long)(chg_mC / 1000),
                   (long)((chg_mC % 1000) < 0 ? -(chg_mC % 1000) : (chg_mC % 1000)));
    debug_send_data((uint8_t*)buf, len);
}

void INA228::dumpRegisters()
{
    Snapshot snap;

    captureSnapshot(snap);

    dumpRawRegisters(snap);

    dumpEngineeringData(snap);
}
