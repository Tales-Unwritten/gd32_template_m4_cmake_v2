#pragma once

#ifdef __cplusplus

#include "device_i2c_dev.hpp"
#include <cstdint>



struct Snapshot
{
    uint16_t config;          // 0x00 配置
    uint16_t adcConfig;       // 0x01 ADC 配置
    uint16_t shuntCal;        // 0x02 分流校准
    uint16_t shunt_tempco;    // 0x03 分流温度系数

    uint64_t vshunt;          // 0x04 分流电压 (24bit)
    uint64_t vbus;            // 0x05 总线电压 (24bit)
    uint16_t temp;            // 0x06 芯片温度 (16bit)
    uint64_t current;         // 0x07 电流结果 (24bit)
    uint64_t power;           // 0x08 功率结果 (24bit)
    uint64_t energy;          // 0x09 电能累积 (40bit)
    uint64_t charge;          // 0x0A 电荷累积 (40bit)
    uint16_t diagAlert;       // 0x0B 诊断与报警标志

    uint16_t sovl;            // 0x0C 分流过压阈值
    uint16_t suvl;            // 0x0D 分流欠压阈值
    uint16_t bovl;            // 0x0E 总线过压阈值
    uint16_t buvl;            // 0x0F 总线欠压阈值
    uint16_t temp_limit;      // 0x10 温度过限阈值
    uint16_t pwr_limit;       // 0x11 功率过限阈值

    uint16_t manufacturerID;  // 0x3E 制造商 ID
    uint16_t deviceID;        // 0x3F 器件 ID
};

class INA228
{
public:

    // ============================================================
    // Address
    // ============================================================

    enum Addr : uint8_t
    {
        ADDR_0x40 = 0x40,
        ADDR_0x41 = 0x41,
        ADDR_0x44 = 0x44,
        ADDR_0x45 = 0x45,
    };


    //config reg=================================================
    enum class Reset : uint8_t
    {
        //15 Bit
        Normal = 0,
        SoftReset = 1,
    };

    enum class Retacc : uint8_t
    {
        //14 Bit
        Normal = 0x00,
        Clear = 0x01,
    };


    enum class Convdly : uint8_t
    {
        //6-13 Bit
        Ms0   = 0x00,
        Ms2   = 0x01,
        Ms510 = 0xFF,
    };

    enum class Tempcomp : uint8_t
    {
        //5 Bit
        Disable = 0x00,
        Enable = 0x01,
    };

    enum class Adcrange : uint8_t
    {
        //4 Bit
        Range_163mv = 0x00,
        Range_40mv = 0x01,
    };

    enum class Reserved : uint8_t
    {
        //0-3 Bit
        Reserved_0 = 0x00,
    };
    //=============================================================


    //Diag Alrt ========================================================
    enum class Alatch : uint8_t
    {
        //15 Bit
        Alatch_Enable = 0x01,
        Alatch_Disable = 0x00,
    };

    enum class Cnvr : uint8_t
    {
        //14 Bit
        Cnvr_Enable = 0x01,
        Cnvr_Disable = 0x00,
    };

    enum class Slowalert : uint8_t
    {
        //13 Bit
        Slowalert_Enable = 0x01,
        Slowalert_Disable = 0x00,
    };


    enum class Apol : uint8_t
    {
        //12 Bit
        Apol_L = 0x00, //低电平有效，开漏
        Apol_H = 0x01, //高电平有效，开漏
    };

    enum class Energyof : uint8_t
    {
        //11 Bit
        Energyof_Normal = 0x00,
        Energyof_Overflow = 0x01,
    };

    enum class Chargeof : uint8_t
    {
        //10 Bit
        Chargeof_Normal = 0x00,
        Chargeof_Overflow = 0x01,
    };

    enum class Mathof : uint8_t
    {
        //9 Bit
        Mathof_Normal = 0x00,
        Mathof_Overflow = 0x01,
    };


    enum class Dig_Reserved : uint8_t
    {
        //8 Bit
        Reserved_0 = 0x00,
    };

    enum class Tmp_Ol : uint8_t
    {
        //7 Bit
        Tmpol_Normal = 0x00,
        Tmpol_Overflow = 0x01,
    };




    enum class Shnt_Ol : uint8_t
    {
        //6 Bit
        Shntol_Normal = 0x00,
        Shntol_Overflow = 0x01,
    };


    enum class Shnt_Ul : uint8_t
    {
        //5 Bit
        Shntul_Normal = 0x00,
        Shntul_Overflow = 0x01,
    };

    enum class Vbus_Ol : uint8_t
    {
        //4 Bit
        Vbus_Normal = 0x00,
        Vbus_Overflow = 0x01,
    };

    enum class Vbus_Ul : uint8_t
    {
        //3 Bit
        Vbus_Normal = 0x00,
        Vbus_Overflow = 0x01,
    };


    enum class Pow_Ol : uint8_t
    {
        //2 Bit
        Pow_Normal = 0x00,
        Pow_Overflow = 0x01,
    };


    enum class Cnvrf : uint8_t
    {
        //1 Bit
        Normal = 0x00,
        Conversion_complete = 0x01,
    };


    enum class Menstat  : uint8_t
    {
        //0 Bit
        Check = 0x00,
        Normal = 0x01,
    };

    // ============================================================
    // Registers
    // ============================================================

    enum Reg : uint8_t
    {
        REG_CONFIG           = 0x00, // 配置
        REG_ADC_CONFIG       = 0x01, // ADC 配置
        REG_SHUNT_CAL        = 0x02, // 分流校准
        REG_VSHUNT           = 0x04, // 分流电压 (24bit)
        REG_VBUS             = 0x05, // 总线电压 (24bit)
        REG_DIETEMP          = 0x06, // 芯片温度
        REG_CURRENT          = 0x07, // 电流结果 (24bit)
        REG_POWER            = 0x08, // 功率结果 (24bit)
        REG_ENERGY           = 0x09, // 电能累积 (40bit)
        REG_CHARGE           = 0x0A, // 电荷累积 (40bit)
        REG_DIAG_ALERT       = 0x0B, // 诊断与报警标志
        REG_SOVL             = 0x0C, // 分流过压阈值
        REG_SUVL             = 0x0D, // 分流欠压阈值
        REG_BOVL             = 0x0E, // 总线过压阈值
        REG_BUVL             = 0x0F, // 总线欠压阈值
        REG_TEMP_LIMIT       = 0x10, // 温度限制
        REG_PWR_LIMIT        = 0x11, // 功率限制
        REG_MANUFACTURER_ID  = 0x3E, // 制造商 ID (TI: 0x5449)
        REG_DEVICE_ID        = 0x3F, // 器件 ID
    };
    // ============================================================
    // ENUM
    // ============================================================


    enum class ADCMode : uint8_t
    {
        Shutdown                              = 0x00,  // 关断
        Trig_BusV                             = 0x01,  // 触发总线电压，单次
        Trig_ShuntV                           = 0x02,  // 触发分流电压，单次
        Trig_BusV_ShuntV                      = 0x03,  // 触发总线电压和分流电压，单次
        Trig_Temp                             = 0x04,  // 触发温度，单次
        Trig_BusV_Temp                        = 0x05,  // 触发总线电压和温度，单次
        Trig_ShuntV_Temp                      = 0x06,  // 触发分流电压和温度，单次
        Trig_BusV_ShuntV_Temp                 = 0x07,  // 触发总线电压、分流电压和温度，单次
        Shutdown2                             = 0x08,  // 关断（0x00 的重复值）
        Cont_BusV                             = 0x09,  // 仅连续总线电压
        Cont_ShuntV                           = 0x0A,  // 仅连续分流电压
        Cont_BusV_ShuntV                      = 0x0B,  // 连续总线电压和分流电压
        Cont_Temp                             = 0x0C,  // 仅连续温度
        Cont_BusV_Temp                        = 0x0D,  // 连续总线电压和温度
        Cont_ShuntV_Temp                      = 0x0E,  // 连续温度和分流电压
        Cont_BusV_ShuntV_Temp                 = 0x0F,  // 连续总线电压、分流电压和温度
    };


    enum class ConvTime : uint8_t
    {
        Us50   = 0x00,
        Us84   = 0x01,
        Us150  = 0x02,
        Us280  = 0x03,
        Us540  = 0x04,
        Us1052 = 0x05,
        Us2074 = 0x06,
        Us4120 = 0x07,
    };

    enum class AvgSample : uint8_t
    {
        N1    = 0x00,
        N4    = 0x01,
        N16   = 0x02,
        N64   = 0x03,
        N128  = 0x04,
        N256  = 0x05,
        N512  = 0x06,
        N1024 = 0x07,
    };

    // ============================================================
    // Config
    // ============================================================

    struct Config
    {

        //CONFIG REG
        Reset reset = Reset::Normal;
        Retacc retacc = Retacc::Normal;
        Convdly convdly = Convdly::Ms0;
        Tempcomp tempcomp = Tempcomp::Disable;
        Adcrange adcrange = Adcrange::Range_163mv;
        int32_t maxCurrent_mA = 163840;
        Reserved reserved = Reserved::Reserved_0;

        // ADC REG
        ADCMode adcMode = ADCMode::Cont_BusV_ShuntV_Temp;
        ConvTime busConvTime   = ConvTime::Us1052;
        ConvTime shuntConvTime = ConvTime::Us1052;
        ConvTime tempConvTime  = ConvTime::Us280;
        AvgSample averaging = AvgSample::N128;

        //DIG_ALERT REG
        Alatch alatch = Alatch::Alatch_Enable;
        Cnvr cnvr = Cnvr::Cnvr_Disable;
        Slowalert slowalert = Slowalert::Slowalert_Disable;
        Apol apol = Apol::Apol_L;
        Energyof energyof = Energyof::Energyof_Normal;
        Chargeof chargeof = Chargeof::Chargeof_Normal;
        Mathof mathof = Mathof::Mathof_Normal;
        Dig_Reserved dig_reserved = Dig_Reserved::Reserved_0;
        Tmp_Ol tmpol = Tmp_Ol::Tmpol_Normal;
        Shnt_Ol shntol = Shnt_Ol::Shntol_Overflow;
        Shnt_Ul shntul = Shnt_Ul::Shntul_Normal;
        Vbus_Ol vbus_ol = Vbus_Ol::Vbus_Overflow;
        Vbus_Ul vbus_ul = Vbus_Ul::Vbus_Overflow;
        Pow_Ol pow_ol = Pow_Ol::Pow_Normal;
        Cnvrf cnvrf = Cnvrf::Normal;
        Menstat menstat = Menstat::Normal;

        uint16_t shuntCal = 0x1000;
        uint16_t r_shunt_uOhm=1000;


        uint32_t bovl_limit = 0xFFFF;

        // int32_t maxCurrent_mA = 40960;
    };

public:

    explicit INA228(device_i2c_bus* bus,
                    uint8_t addr = ADDR_0x40);

    explicit INA228(device_i2c_bus* bus,
                    uint8_t addr,
                    const Config& cfg);

    void init();

public:

    // ============================================================
    // Measurement API
    // ============================================================

    int32_t getBusVoltage_mV();
    int32_t getShuntVoltage_uV();
    int32_t getCurrent_mA();
    int32_t getTemperature_mC();
    int32_t getPower_mW();
    int64_t getEnergy_mJ();
    int64_t getCharge_mC();

public:

    void set_sovl_limit_mA(int32_t sovl_mA);
    void set_suvl_limit_mA(int32_t suvl_mA);
    void set_bovl_limit_mV(uint16_t bovl_mV);
    void set_buvl_limit_mV(uint16_t buvl_mV);
    void set_temp_limit_C(uint16_t temp_C);
    void set_pwr_limit_mW(uint32_t pwr_mW);

    uint16_t getManufacturerID();
    uint16_t getDeviceID();
    void unlock();

public:

    void dumpRegisters();

private:

    device_i2c_dev _dev;

    Config _cfg;

    // current LSB (uA)
    int32_t _currentLSB_uA;

private:

    void writeRaw(uint8_t reg,
                  uint64_t data,
                  uint8_t len);

    uint64_t readRaw(uint8_t reg);

    static uint8_t regLen(uint8_t reg);

private:

    template<uint8_t Bits>
    static constexpr int64_t signExtend(uint64_t raw)
    {
        constexpr int shift = 64 - Bits;
        return (static_cast<int64_t>(raw << shift)) >> shift;
    }

private:

    static int32_t rawToBusVoltage_mV(uint64_t raw);

    static int32_t rawToShuntVoltage_uV(uint64_t raw);

    int32_t rawToCurrent_mA(uint64_t raw) const;

    static int32_t rawToTemperature_mC(uint64_t raw);

    int32_t rawToPower_mW(uint64_t raw) const;

    int64_t rawToEnergy_mJ(uint64_t raw) const;

    int64_t rawToCharge_mC(uint64_t raw) const;

    void captureSnapshot(Snapshot& snap);
    void dumpRawRegisters(const Snapshot& snap);
    void dumpEngineeringData(const Snapshot& snap);


private:

    uint16_t buildConfigReg() const;

    uint16_t buildADCConfigReg() const;

    uint16_t buildDiagConfigReg() const;
};

#endif
