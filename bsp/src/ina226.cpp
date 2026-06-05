#include "ina226.hpp"
#include "link.h"
#include "usart_port.hpp"

extern usart_port debug_uart;

#include <cstdio>
#include <new>

// ============================================================
//  构造
// ============================================================

INA226::INA226(device_i2c_bus* bus, uint8_t addr)
    : _dev(bus, addr)
    , _alertPin(nullptr)
{
    _updateCurrentLSB();
}

INA226::INA226(device_i2c_bus* bus, uint8_t addr, const Config& cfg)
    : _dev(bus, addr)
    , _cfg(cfg)
    , _alertPin(nullptr)
{
    _updateCurrentLSB();
}

INA226::~INA226()
{
}

// ============================================================
//  Current LSB
// ============================================================

void INA226::_updateCurrentLSB()
{
    // Current_LSB = I_MAX / 2^15
    // currentLSB_uA = maxCurrent_mA × 1000 / 32768
    _currentLSB_uA = _cfg.maxCurrent_mA * 1000 / 32768;
}

// ============================================================
//  init
// ============================================================

void INA226::init()
{
    // 1. 配置寄存器 (0x00)
    writeRaw(REG_CONFIG,      buildConfigReg());

    // 2. 校准寄存器 (0x05)
    writeRaw(REG_CALIBRATION, _cfg.calibration);

    // 3. Mask/Enable (0x06) — 写配置位
    writeRaw(REG_MASK_ENABLE, buildMaskEnable());

    // 4. Alert Limit (0x07)
    writeRaw(REG_ALERT_LIMIT, _cfg.alertLimit);

    // 5. ⚠️ 上电后 AFF 等状态位可能已置位，与寄存器写入顺序无关。
    //    INA226 开漏 ALERT 引脚在锁存模式下只要状态位非零就持续拉低。
    //    读取 Mask/Enable 是唯一清除锁存标志的手段。
    //    修复前缺少此读操作，与 INA228 同根因。
    (void)readRaw(REG_MASK_ENABLE);

    // 6. 自动配置报警引脚（Config 中指定了 GPIO 端口和引脚时）
    //    零堆分配：直接用 GPIO 寄存器，不创建 io_ctrl 对象
    if (_cfg.alert_port != 0 && _cfg.alert_pin != 0)
    {
        _alertPort    = _cfg.alert_port;
        _alertPinMask = _cfg.alert_pin;

        // 使能 GPIO 时钟
        if      (_alertPort == GPIOA) rcu_periph_clock_enable(RCU_GPIOA);
        else if (_alertPort == GPIOB) rcu_periph_clock_enable(RCU_GPIOB);
        else if (_alertPort == GPIOC) rcu_periph_clock_enable(RCU_GPIOC);
        else if (_alertPort == GPIOD) rcu_periph_clock_enable(RCU_GPIOD);
        else if (_alertPort == GPIOE) rcu_periph_clock_enable(RCU_GPIOE);
        else if (_alertPort == GPIOF) rcu_periph_clock_enable(RCU_GPIOF);
        else if (_alertPort == GPIOG) rcu_periph_clock_enable(RCU_GPIOG);

        // 开漏 ALERT → 输入 + 上拉
        gpio_mode_set(_alertPort, GPIO_MODE_INPUT, GPIO_PUPD_PULLUP, _alertPinMask);
        _alertCfg = true;
    }
}

// ============================================================
//  I2C IO（INA226 全部 16-bit 寄存器）
// ============================================================

void INA226::writeRaw(uint8_t reg, uint16_t data)
{
    _dev.i2c_write_16bit(reg, data);
}

uint16_t INA226::readRaw(uint8_t reg)
{
    return _dev.i2c_read_16bit(reg);
}

// ============================================================
//  寄存器构建
// ============================================================

uint16_t INA226::buildConfigReg() const
{
    // Config Register (0x00):
    //   [15]    RST
    //   [14:12] 保留 = 0b100
    //   [11:9]  AVG
    //   [8:6]   VBUSCT
    //   [5:3]   VSHCT
    //   [2:0]   MODE
    uint16_t r = 0;
    r |= static_cast<uint16_t>(_cfg.reset)         << 15;
    r |= 0x04u                                     << 12;  // 保留位 0b100
    r |= static_cast<uint16_t>(_cfg.averaging)     << 9;
    r |= static_cast<uint16_t>(_cfg.busConvTime)   << 6;
    r |= static_cast<uint16_t>(_cfg.shuntConvTime) << 3;
    r |= static_cast<uint16_t>(_cfg.mode);
    return r;
}

uint16_t INA226::buildMaskEnable() const
{
    // Mask/Enable Register (0x06):
    //   [15:5] 保留 / 状态（读时有效）
    //   [4:2]  状态标志（只读）
    //   [1]    APOL  — 报警极性
    //   [0]    ALATCH — 报警锁存
    uint16_t r = 0;
    r |= static_cast<uint16_t>(_cfg.apol)   << 1;
    r |= static_cast<uint16_t>(_cfg.alatch) << 0;
    return r;
}

// ============================================================
//  转换
// ============================================================

int32_t INA226::rawToShuntVoltage_uV(uint16_t raw)
{
    // 2.5 µV/LSB = 5/2 µV/LSB
    int32_t val = static_cast<int32_t>(signExtend<16>(raw));
    return (val * 5) / 2;
}

int32_t INA226::rawToBusVoltage_mV(uint16_t raw)
{
    // 1.25 mV/LSB = 5/4 mV/LSB
    int32_t val = static_cast<int32_t>(raw);
    return (val * 5) / 4;
}

int32_t INA226::rawToCurrent_mA(uint16_t raw) const
{
    int32_t val = static_cast<int32_t>(signExtend<16>(raw));
    return (val * _currentLSB_uA) / 1000;
}

int32_t INA226::rawToPower_mW(uint16_t raw) const
{
    // Power_LSB = 25 × Current_LSB
    // Power(mW) = raw × 25 × currentLSB_uA / 1000
    int64_t val = static_cast<int64_t>(raw);
    return static_cast<int32_t>((val * 25 * _currentLSB_uA) / 1000);
}

// ============================================================
//  测量 API
// ============================================================

int32_t INA226::getBusVoltage_mV()
    { return rawToBusVoltage_mV(readRaw(REG_BUS_VOLTAGE)); }

int32_t INA226::getShuntVoltage_uV()
    { return rawToShuntVoltage_uV(readRaw(REG_SHUNT_VOLTAGE)); }

int32_t INA226::getCurrent_mA()
    { return rawToCurrent_mA(readRaw(REG_CURRENT)); }

int32_t INA226::getPower_mW()
    { return rawToPower_mW(readRaw(REG_POWER)); }

// ============================================================
//  报警
// ============================================================

void INA226::bindAlertPin(io_ctrl *pin)
{
    _alertPin = pin;
}

bool INA226::isAlertAsserted()
{
    // 优先使用 bindAlertPin 绑定的外部引脚
    if (_alertPin)
        return _alertPin->read() == RESET;

    // 否则使用自动配置的引脚
    if (_alertCfg)
        return gpio_input_bit_get(_alertPort, _alertPinMask) == RESET;

    return false;
}

INA226_AlertFlags INA226::readAlertFlags()
{
    INA226_AlertFlags f = {};
    uint16_t raw = readRaw(REG_MASK_ENABLE);

    f.mathOverflow    = (raw & 0x0004) != 0;  // bit 2: OVF
    f.conversionReady = (raw & 0x0008) != 0;  // bit 3: CVRF
    f.alertFunction   = (raw & 0x0010) != 0;  // bit 4: AFF

    return f;
}

void INA226::clearAlertLatch()
{
    // 读取 Mask/Enable 清除锁存标志，重写配置恢复报警逻辑
    (void)readRaw(REG_MASK_ENABLE);
    writeRaw(REG_MASK_ENABLE, buildMaskEnable());
}

uint16_t INA226::readMaskEnableRaw()
{
    return readRaw(REG_MASK_ENABLE);
}

// ============================================================
//  运行时配置
// ============================================================

void INA226::setConfig(const Config& cfg)
{
    _cfg = cfg;
    _updateCurrentLSB();

    writeRaw(REG_CONFIG,      buildConfigReg());
    writeRaw(REG_CALIBRATION, _cfg.calibration);
    writeRaw(REG_MASK_ENABLE, buildMaskEnable());
    writeRaw(REG_ALERT_LIMIT, _cfg.alertLimit);
}

void INA226::setAveraging(AvgSample mode)
    { _cfg.averaging = mode; writeRaw(REG_CONFIG, buildConfigReg()); }

void INA226::setBusConvTime(ConvTime time)
    { _cfg.busConvTime = time; writeRaw(REG_CONFIG, buildConfigReg()); }

void INA226::setShuntConvTime(ConvTime time)
    { _cfg.shuntConvTime = time; writeRaw(REG_CONFIG, buildConfigReg()); }

void INA226::setOperatingMode(OperatingMode mode)
    { _cfg.mode = mode; writeRaw(REG_CONFIG, buildConfigReg()); }

void INA226::setCalibration(uint16_t cal)
    { _cfg.calibration = cal; writeRaw(REG_CALIBRATION, cal); }

void INA226::setAlertLimit(uint16_t limit)
    { _cfg.alertLimit = limit; writeRaw(REG_ALERT_LIMIT, limit); }

// ============================================================
//  设备信息
// ============================================================

uint16_t INA226::getManufacturerID()
    { return readRaw(REG_MANUFACTURER_ID); }

uint16_t INA226::getDieID()
    { return readRaw(REG_DIE_ID); }

// ============================================================
//  调试
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
                   "\r\n========== INA226 RAW ==========\r\n");
    (void)debug_uart.send_data((const uint8_t*)buf, len);

    len = snprintf(buf, sizeof(buf), "CONFIG   : 0x%04X\r\n", s.config);
    (void)debug_uart.send_data((const uint8_t*)buf, len);

    len = snprintf(buf, sizeof(buf), "VSHUNT   : 0x%04X\r\n", s.shuntVoltage);
    (void)debug_uart.send_data((const uint8_t*)buf, len);

    len = snprintf(buf, sizeof(buf), "VBUS     : 0x%04X\r\n", s.busVoltage);
    (void)debug_uart.send_data((const uint8_t*)buf, len);

    len = snprintf(buf, sizeof(buf), "CURRENT  : 0x%04X\r\n", s.current);
    (void)debug_uart.send_data((const uint8_t*)buf, len);

    len = snprintf(buf, sizeof(buf), "POWER    : 0x%04X\r\n", s.power);
    (void)debug_uart.send_data((const uint8_t*)buf, len);

    len = snprintf(buf, sizeof(buf), "CAL      : 0x%04X\r\n", s.calibration);
    (void)debug_uart.send_data((const uint8_t*)buf, len);

    len = snprintf(buf, sizeof(buf), "MASK/EN  : 0x%04X\r\n", s.maskEnable);
    (void)debug_uart.send_data((const uint8_t*)buf, len);

    len = snprintf(buf, sizeof(buf), "ALERTLIM : 0x%04X\r\n", s.alertLimit);
    (void)debug_uart.send_data((const uint8_t*)buf, len);
}

void INA226::dumpEngineeringData(const INA226_Snapshot& s)
{
    char buf[128];
    int  len;

    int32_t shunt_uV   = rawToShuntVoltage_uV(s.shuntVoltage);
    int32_t bus_mV     = rawToBusVoltage_mV(s.busVoltage);
    int32_t current_mA = rawToCurrent_mA(s.current);
    int32_t power_mW   = rawToPower_mW(s.power);

    len = snprintf(buf, sizeof(buf),
                   "\r\n========== INA226 ENGINEERING ==========\r\n");
    (void)debug_uart.send_data((const uint8_t*)buf, len);

    len = snprintf(buf, sizeof(buf), "Bus Voltage    : %ld.%03ld V\r\n",
                   (long)bus_mV / 1000, (long)bus_mV % 1000);
    (void)debug_uart.send_data((const uint8_t*)buf, len);

    len = snprintf(buf, sizeof(buf), "Shunt Voltage  : %ld.%03ld mV\r\n",
                   (long)shunt_uV / 1000, (long)shunt_uV % 1000);
    (void)debug_uart.send_data((const uint8_t*)buf, len);

    len = snprintf(buf, sizeof(buf), "Current        : %ld.%03ld A\r\n",
                   (long)current_mA / 1000, (long)current_mA % 1000);
    (void)debug_uart.send_data((const uint8_t*)buf, len);

    len = snprintf(buf, sizeof(buf), "Power          : %ld.%03ld W\r\n",
                   (long)power_mW / 1000, (long)power_mW % 1000);
    (void)debug_uart.send_data((const uint8_t*)buf, len);
}

void INA226::dumpRegisters()
{
    INA226_Snapshot snap;
    captureSnapshot(snap);
    dumpRawRegisters(snap);
    dumpEngineeringData(snap);
}
