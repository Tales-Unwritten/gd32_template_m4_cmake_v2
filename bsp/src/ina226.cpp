#include "ina226.hpp"
#include "link.h"

#include <cstdio>

// ============================================================
// Constructor
// ============================================================

INA226::INA226(device_i2c_bus* bus,
               uint8_t addr)
    : _dev(bus, addr)
    , _alertPin(nullptr)
{
    // INA226: Current_LSB = I_MAX / 32768
    // currentLSB_uA = maxCurrent_mA * 1000 / 32768
    _currentLSB_uA = _cfg.maxCurrent_mA * 1000 / 32768;
}

INA226::INA226(device_i2c_bus* bus,
               uint8_t addr,
               const Config& cfg)
    : _dev(bus, addr)
    , _cfg(cfg)
    , _alertPin(nullptr)
{
    _currentLSB_uA = _cfg.maxCurrent_mA * 1000 / 32768;
}

INA226::INA226(device_i2c_bus* bus,
               uint8_t addr,
               const Config& cfg,
               io_ctrl* alertPin)
    : _dev(bus, addr)
    , _cfg(cfg)
    , _alertPin(alertPin)
{
    _currentLSB_uA = _cfg.maxCurrent_mA * 1000 / 32768;
}

INA226::~INA226()
{
    _alertPin = nullptr;
}

// ============================================================
// Init
// ============================================================

void INA226::init()
{
    // 步骤 1: 写入配置寄存器 (0x00)
    writeRaw(REG_CONFIG,
             buildConfigReg());

    // 步骤 2: 写入校准寄存器 (0x05)
    writeRaw(REG_CALIBRATION,
             _cfg.calibration);

    // 步骤 3: 写入 Mask/Enable 寄存器 (0x06)
    writeRaw(REG_MASK_ENABLE,
             _cfg.maskEnable);

    // 步骤 4: 写入 Alert Limit 寄存器 (0x07)
    writeRaw(REG_ALERT_LIMIT,
             _cfg.alertLimit);

    // 步骤 5: 如果 Alert 引脚已绑定，初始化为上拉输入
    if (_alertPin)
    {
        _alertPin->init(GPIO_MODE_INPUT,
                        GPIO_PUPD_PULLUP,
                        GPIO_OTYPE_PP,
                        GPIO_OSPEED_50MHZ);
    }
}

// ============================================================
// I2C IO（INA226 所有寄存器均为 16-bit）
// ============================================================

void INA226::writeRaw(uint8_t reg,
                      uint16_t data)
{
    _dev.i2c_write_16bit(reg, data);
}

uint16_t INA226::readRaw(uint8_t reg)
{
    return _dev.i2c_read_16bit(reg);
}

// ============================================================
// Register Build
// ============================================================

uint16_t INA226::buildConfigReg() const
{
    /*
        Config Register (0x00) bit layout:

        [15]    RST      - 软件复位
        [14:12] 保留     - 固定 0b100 = 0x04
        [11:9]  AVG      - 采样平均模式
        [8:6]   VBUSCT   - 总线电压转换时间
        [5:3]   VSHCT    - 分流电压转换时间
        [2:0]   MODE     - 工作模式
    */

    uint16_t r = 0;

    r |= static_cast<uint16_t>(_cfg.reset) << 15;

    r |= 0x04 << 12;   // 保留位固定 0x04

    r |= static_cast<uint16_t>(_cfg.averaging) << 9;

    r |= static_cast<uint16_t>(_cfg.busConvTime) << 6;

    r |= static_cast<uint16_t>(_cfg.shuntConvTime) << 3;

    r |= static_cast<uint16_t>(_cfg.mode);

    return r;
}

// ============================================================
// Conversion
// ============================================================

int32_t INA226::rawToShuntVoltage_uV(uint16_t raw)
{
    /*
        Shunt Voltage LSB:

        2.5 uV

        = 5 / 2 uV

        所以:

        uV = value * 5 / 2
    */

    int32_t val =
        static_cast<int32_t>(
            signExtend<16>(raw));

    return (val * 5) / 2;
}

int32_t INA226::rawToBusVoltage_mV(uint16_t raw)
{
    /*
        Bus Voltage LSB:

        1.25 mV

        = 5 / 4 mV

        所以:

        mV = value * 5 / 4
    */

    // 总线电压寄存器为无符号值，直接使用
    int32_t val = static_cast<int32_t>(raw);

    return (val * 5) / 4;
}

int32_t INA226::rawToCurrent_mA(uint16_t raw) const
{
    /*
        Current LSB = currentLSB_uA (uA)

        转 mA:

        mA = raw * currentLSB_uA / 1000
    */

    int32_t val =
        static_cast<int32_t>(
            signExtend<16>(raw));

    return (val * _currentLSB_uA) / 1000;
}

int32_t INA226::rawToPower_mW(uint16_t raw) const
{
    /*
        Power LSB:

        Power_LSB = 25 × Current_LSB

        所以:

        power_W  = raw × 25 × Current_LSB_A
        power_mW = raw × 25 × currentLSB_uA / 1000
    */

    // 功率寄存器为无符号值
    int64_t val = static_cast<int64_t>(raw);

    return static_cast<int32_t>(
        (val * 25 * _currentLSB_uA) / 1000);
}

// ============================================================
// Measurement API
// ============================================================

int32_t INA226::getBusVoltage_mV()
{
    return rawToBusVoltage_mV(
        readRaw(REG_BUS_VOLTAGE));
}

int32_t INA226::getShuntVoltage_uV()
{
    return rawToShuntVoltage_uV(
        readRaw(REG_SHUNT_VOLTAGE));
}

int32_t INA226::getCurrent_mA()
{
    return rawToCurrent_mA(
        readRaw(REG_CURRENT));
}

int32_t INA226::getPower_mW()
{
    return rawToPower_mW(
        readRaw(REG_POWER));
}

// ============================================================
// Device Info
// ============================================================

uint16_t INA226::getManufacturerID()
{
    return readRaw(REG_MANUFACTURER_ID);
}

uint16_t INA226::getDieID()
{
    return readRaw(REG_DIE_ID);
}

// ============================================================
// Alert
// ============================================================

bool INA226::isAlertActive()
{
    if (!_alertPin)
        return false;

    // Alert 引脚在报警时输出低电平
    return (_alertPin->read() == RESET);
}

void INA226::unlockAlert()
{
    // 读取 Mask/Enable 寄存器即可清除锁存报警标志
    readRaw(REG_MASK_ENABLE);
}

// ============================================================
// Runtime Config
// ============================================================

void INA226::setConfig(const Config& cfg)
{
    _cfg = cfg;

    // 重新计算 Current LSB
    _currentLSB_uA = _cfg.maxCurrent_mA * 1000 / 32768;

    // 写入所有配置寄存器
    writeRaw(REG_CONFIG,
             buildConfigReg());

    writeRaw(REG_CALIBRATION,
             _cfg.calibration);

    writeRaw(REG_MASK_ENABLE,
             _cfg.maskEnable);

    writeRaw(REG_ALERT_LIMIT,
             _cfg.alertLimit);
}

void INA226::setAveraging(AvgSample mode)
{
    _cfg.averaging = mode;
    writeRaw(REG_CONFIG,
             buildConfigReg());
}

void INA226::setBusConvTime(ConvTime time)
{
    _cfg.busConvTime = time;
    writeRaw(REG_CONFIG,
             buildConfigReg());
}

void INA226::setShuntConvTime(ConvTime time)
{
    _cfg.shuntConvTime = time;
    writeRaw(REG_CONFIG,
             buildConfigReg());
}

void INA226::setOperatingMode(OperatingMode mode)
{
    _cfg.mode = mode;
    writeRaw(REG_CONFIG,
             buildConfigReg());
}

void INA226::setCalibration(uint16_t cal)
{
    _cfg.calibration = cal;
    writeRaw(REG_CALIBRATION,
             _cfg.calibration);
}

void INA226::setAlertLimit(uint16_t limit)
{
    _cfg.alertLimit = limit;
    writeRaw(REG_ALERT_LIMIT,
             _cfg.alertLimit);
}

void INA226::setMaskEnable(uint16_t mask)
{
    _cfg.maskEnable = mask;
    writeRaw(REG_MASK_ENABLE,
             _cfg.maskEnable);
}

// ============================================================
// Dump
// ============================================================

void INA226::captureSnapshot(INA226_Snapshot& s)
{
    s.config         = readRaw(REG_CONFIG);
    s.shuntVoltage   = readRaw(REG_SHUNT_VOLTAGE);
    s.busVoltage     = readRaw(REG_BUS_VOLTAGE);
    s.power          = readRaw(REG_POWER);
    s.current        = readRaw(REG_CURRENT);
    s.calibration    = readRaw(REG_CALIBRATION);
    s.maskEnable     = readRaw(REG_MASK_ENABLE);
    s.alertLimit     = readRaw(REG_ALERT_LIMIT);
    s.manufacturerID = readRaw(REG_MANUFACTURER_ID);
    s.dieID          = readRaw(REG_DIE_ID);
}

void INA226::dumpRawRegisters(const INA226_Snapshot& s)
{
    char buf[128];
    int  len;

    len = snprintf(buf, sizeof(buf),
                   "\r\n========== RAW ==========\r\n");
    debug_send_data((uint8_t*)buf, len);

    len = snprintf(buf, sizeof(buf),
                   "CONFIG   : 0x%04X\r\n",
                   s.config);
    debug_send_data((uint8_t*)buf, len);

    len = snprintf(buf, sizeof(buf),
                   "VSHUNT   : 0x%04X\r\n",
                   s.shuntVoltage);
    debug_send_data((uint8_t*)buf, len);

    len = snprintf(buf, sizeof(buf),
                   "VBUS     : 0x%04X\r\n",
                   s.busVoltage);
    debug_send_data((uint8_t*)buf, len);

    len = snprintf(buf, sizeof(buf),
                   "CURRENT  : 0x%04X\r\n",
                   s.current);
    debug_send_data((uint8_t*)buf, len);

    len = snprintf(buf, sizeof(buf),
                   "POWER    : 0x%04X\r\n",
                   s.power);
    debug_send_data((uint8_t*)buf, len);

    len = snprintf(buf, sizeof(buf),
                   "CAL      : 0x%04X\r\n",
                   s.calibration);
    debug_send_data((uint8_t*)buf, len);

    len = snprintf(buf, sizeof(buf),
                   "MASK/EN  : 0x%04X\r\n",
                   s.maskEnable);
    debug_send_data((uint8_t*)buf, len);

    len = snprintf(buf, sizeof(buf),
                   "ALERTLIM : 0x%04X\r\n",
                   s.alertLimit);
    debug_send_data((uint8_t*)buf, len);
}

void INA226::dumpEngineeringData(const INA226_Snapshot& s)
{
    char buf[128];
    int  len;

    int32_t shunt_uV =
        rawToShuntVoltage_uV(s.shuntVoltage);

    int32_t bus_mV =
        rawToBusVoltage_mV(s.busVoltage);

    int32_t current_mA =
        rawToCurrent_mA(s.current);

    int32_t power_mW =
        rawToPower_mW(s.power);

    len = snprintf(buf, sizeof(buf),
                   "\r\n========== ENGINEERING ==========\r\n");
    debug_send_data((uint8_t*)buf, len);

    len = snprintf(buf, sizeof(buf),
                   "Bus Voltage    : %ld.%03ld V\r\n",
                   bus_mV / 1000,
                   bus_mV % 1000);
    debug_send_data((uint8_t*)buf, len);

    len = snprintf(buf, sizeof(buf),
                   "Shunt Voltage  : %ld.%03ld mV\r\n",
                   shunt_uV / 1000,
                   shunt_uV % 1000);
    debug_send_data((uint8_t*)buf, len);

    len = snprintf(buf, sizeof(buf),
                   "Current        : %ld.%03ld A\r\n",
                   current_mA / 1000,
                   current_mA % 1000);
    debug_send_data((uint8_t*)buf, len);

    len = snprintf(buf, sizeof(buf),
                   "Power          : %ld.%03ld W\r\n",
                   power_mW / 1000,
                   power_mW % 1000);
    debug_send_data((uint8_t*)buf, len);
}

void INA226::dumpRegisters()
{
    INA226_Snapshot snap;

    captureSnapshot(snap);

    dumpRawRegisters(snap);

    dumpEngineeringData(snap);
}
