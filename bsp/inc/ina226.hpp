#pragma once

#ifdef __cplusplus

#include "device_i2c_dev.hpp"
#include "io_ctrl.hpp"
#include <cstdint>


// ============================================================
// INA226 Snapshot（寄存器快照，用于调试 dump）
// ============================================================

struct INA226_Snapshot
{
    uint16_t config;
    uint16_t shuntVoltage;
    uint16_t busVoltage;
    uint16_t power;
    uint16_t current;
    uint16_t calibration;
    uint16_t maskEnable;
    uint16_t alertLimit;
    uint16_t manufacturerID;
    uint16_t dieID;
};


class INA226
{
public:

    // ============================================================
    // Address（7-bit I2C 地址，由 A1/A0 引脚决定）
    // ============================================================

    enum Addr : uint8_t
    {
        ADDR_0x40 = 0x40,   // A1=0, A0=0
        ADDR_0x41 = 0x41,   // A1=0, A0=1
        ADDR_0x44 = 0x44,   // A1=1, A0=0
        ADDR_0x45 = 0x45,   // A1=1, A0=1
    };

    // ============================================================
    // Registers
    // ============================================================

    enum Reg : uint8_t
    {
        REG_CONFIG           = 0x00,   // 配置寄存器          (R/W)
        REG_SHUNT_VOLTAGE    = 0x01,   // 分流电压寄存器      (R)
        REG_BUS_VOLTAGE      = 0x02,   // 总线电压寄存器      (R)
        REG_POWER            = 0x03,   // 功率寄存器          (R)
        REG_CURRENT          = 0x04,   // 电流寄存器          (R)
        REG_CALIBRATION      = 0x05,   // 校准寄存器          (R/W)
        REG_MASK_ENABLE      = 0x06,   // 掩码/使能寄存器     (R/W)
        REG_ALERT_LIMIT      = 0x07,   // 报警阈值寄存器      (R/W)
        REG_MANUFACTURER_ID  = 0xFE,   // 制造商 ID 寄存器    (R)
        REG_DIE_ID           = 0xFF,   // 芯片 ID 寄存器      (R)
    };

    // ============================================================
    // Enums
    // ============================================================

    enum class Reset : uint8_t
    {
        Normal    = 0,
        SoftReset = 1,
    };

    enum class AvgSample : uint8_t
    {
        N1    = 0x00,   // 1    次采样平均
        N4    = 0x01,   // 4    次采样平均
        N16   = 0x02,   // 16   次采样平均
        N64   = 0x03,   // 64   次采样平均
        N128  = 0x04,   // 128  次采样平均
        N256  = 0x05,   // 256  次采样平均
        N512  = 0x06,   // 512  次采样平均
        N1024 = 0x07,   // 1024 次采样平均
    };

    enum class ConvTime : uint8_t
    {
        Us140  = 0x00,   //  140 us
        Us204  = 0x01,   //  204 us
        Us332  = 0x02,   //  332 us
        Us588  = 0x03,   //  588 us
        Us1100 = 0x04,   // 1100 us (1.1 ms)
        Us2116 = 0x05,   // 2116 us (2.116 ms)
        Us4156 = 0x06,   // 4156 us (4.156 ms)
        Us8244 = 0x07,   // 8244 us (8.244 ms)
    };

    enum class OperatingMode : uint8_t
    {
        PowerDown          = 0x00,   // 关断
        ShuntTrigger       = 0x01,   // 分流电压触发
        BusTrigger         = 0x02,   // 总线电压触发
        ShuntBusTrigger    = 0x03,   // 分流+总线触发
        Shutdown           = 0x04,   // 关断（另一定义）
        ShuntContinuous    = 0x05,   // 分流电压连续
        BusContinuous      = 0x06,   // 总线电压连续
        ShuntBusContinuous = 0x07,   // 分流+总线连续
    };

    // ============================================================
    // Config
    //
    // 校准计算说明：
    //   Current_LSB = I_MAX / 32768          (I_MAX 为预期最大电流，单位 A)
    //   CAL         = 0.00512 / (Current_LSB × R_SHUNT)
    //
    //   例: R_SHUNT = 0.1Ω, I_MAX = 0.8192A
    //       Current_LSB = 0.8192 / 32768 = 25 uA
    //       CAL = 0.00512 / (0.000025 × 0.1) = 2048
    //
    //   currentLSB_uA 由 maxCurrent_mA 自动推导：
    //       currentLSB_uA = maxCurrent_mA * 1000 / 32768
    // ============================================================

    struct Config
    {
        Reset         reset          = Reset::Normal;
        AvgSample     averaging      = AvgSample::N1;
        ConvTime      busConvTime    = ConvTime::Us1100;
        ConvTime      shuntConvTime  = ConvTime::Us1100;
        OperatingMode mode           = OperatingMode::ShuntBusContinuous;

        uint16_t calibration = 0x0800;   // CAL 寄存器值
        uint16_t maskEnable  = 0x0000;   // Mask/Enable 寄存器值
        uint16_t alertLimit  = 0x0000;   // Alert Limit 寄存器值

        // 预期最大电流（mA），用于推导 Current LSB
        int32_t maxCurrent_mA = 81;
    };

public:

    // ============================================================
    // Constructors
    // ============================================================

    explicit INA226(device_i2c_bus* bus,
                    uint8_t addr = ADDR_0x40);

    explicit INA226(device_i2c_bus* bus,
                    uint8_t addr,
                    const Config& cfg);

    INA226(device_i2c_bus* bus,
           uint8_t addr,
           const Config& cfg,
           io_ctrl* alertPin);

    // 禁止拷贝
    INA226(const INA226&) = delete;
    INA226& operator=(const INA226&) = delete;

    ~INA226();

    // ============================================================
    // Init
    // ============================================================

    void init();

public:

    // ============================================================
    // Measurement API（返回值单位见函数名后缀）
    // ============================================================

    /** @brief  读取总线电压，返回值单位: mV */
    int32_t getBusVoltage_mV();

    /** @brief  读取分流电压，返回值单位: uV */
    int32_t getShuntVoltage_uV();

    /** @brief  读取电流，返回值单位: mA */
    int32_t getCurrent_mA();

    /** @brief  读取功率，返回值单位: mW */
    int32_t getPower_mW();

public:

    // ============================================================
    // Device Info
    // ============================================================

    /** @brief  读取制造商 ID（应为 0x5449 = "TI"） */
    uint16_t getManufacturerID();

    /** @brief  读取芯片 Die ID */
    uint16_t getDieID();

public:

    // ============================================================
    // Alert
    // ============================================================

    /**
     * @brief  检测 Alert 引脚状态
     * @return true = 有报警触发（引脚低电平），
     *         false = 无报警或未配置引脚
     */
    bool isAlertActive();

    /**
     * @brief  解锁报警锁存
     *
     *  读取 Mask/Enable 寄存器以清除锁存报警标志。
     */
    void unlockAlert();

public:

    // ============================================================
    // Runtime Config
    // ============================================================

    /** @brief  获取当前配置的只读引用 */
    const Config& config() const { return _cfg; }

    /** @brief  批量更新全部配置并写入芯片 */
    void setConfig(const Config& cfg);

    /** @brief  修改采样平均模式 */
    void setAveraging(AvgSample mode);

    /** @brief  修改总线电压转换时间 */
    void setBusConvTime(ConvTime time);

    /** @brief  修改分流电压转换时间 */
    void setShuntConvTime(ConvTime time);

    /** @brief  修改工作模式 */
    void setOperatingMode(OperatingMode mode);

    /** @brief  修改校准寄存器值 */
    void setCalibration(uint16_t cal);

    /** @brief  修改报警阈值 */
    void setAlertLimit(uint16_t limit);

    /** @brief  修改 Mask/Enable 寄存器 */
    void setMaskEnable(uint16_t mask);

public:

    // ============================================================
    // Dump（调试用）
    // ============================================================

    void dumpRegisters();

private:

    // ============================================================
    // I2C IO
    // ============================================================

    void writeRaw(uint8_t reg, uint16_t data);

    uint16_t readRaw(uint8_t reg);

private:

    // ============================================================
    // Conversion
    // ============================================================

    template<uint8_t Bits>
    static constexpr int64_t signExtend(uint64_t raw)
    {
        constexpr int shift = 64 - Bits;
        return (static_cast<int64_t>(raw << shift)) >> shift;
    }

    // 分流电压: LSB = 2.5 uV = 5/2 uV
    static int32_t rawToShuntVoltage_uV(uint16_t raw);

    // 总线电压: LSB = 1.25 mV = 5/4 mV
    static int32_t rawToBusVoltage_mV(uint16_t raw);

    // 电流: LSB = currentLSB_uA (uA)
    // 返回 mA
    int32_t rawToCurrent_mA(uint16_t raw) const;

    // 功率: Power_LSB = 25 × Current_LSB
    // 返回 mW
    int32_t rawToPower_mW(uint16_t raw) const;

private:

    // ============================================================
    // Register Build
    // ============================================================

    uint16_t buildConfigReg() const;

private:

    // ============================================================
    // Dump Helpers
    // ============================================================

    void captureSnapshot(INA226_Snapshot& snap);
    void dumpRawRegisters(const INA226_Snapshot& snap);
    void dumpEngineeringData(const INA226_Snapshot& snap);

private:

    device_i2c_dev _dev;
    Config         _cfg;
    io_ctrl*       _alertPin;

    // Current LSB（uA）
    int32_t _currentLSB_uA;
};

#endif /* __cplusplus */
