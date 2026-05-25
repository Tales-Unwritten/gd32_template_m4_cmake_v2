#pragma once

#ifdef __cplusplus

#include "device_i2c_dev.hpp"
#include "io_ctrl.hpp"
#include <cstdint>

// ============================================================
// INA226 Snapshot
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

// ============================================================
//  Alert 标志位（Mask/Enable 寄存器 bit 4-0 状态字段）
//  读后自动清除（锁存模式）或实时反映（非锁存模式）
// ============================================================

struct INA226_AlertFlags
{
    bool mathOverflow;     // bit 2: OVF 数学溢出
    bool conversionReady;  // bit 3: CVRF 转换完成
    bool alertFunction;    // bit 4: AFF 报警功能激活
};

// ============================================================

class INA226
{
public:

    // ── I2C 地址 ──────────────────────────────────────────

    enum Addr : uint8_t
    {
        ADDR_0x40 = 0x40, ADDR_0x41 = 0x41,
        ADDR_0x44 = 0x44, ADDR_0x45 = 0x45,
    };

    // ── 寄存器地址 ────────────────────────────────────────

    enum Reg : uint8_t
    {
        REG_CONFIG           = 0x00,   // 配置寄存器 (R/W, reset=0x4127)
        REG_SHUNT_VOLTAGE    = 0x01,   // 分流电压 (R, 16-bit 有符号)
        REG_BUS_VOLTAGE      = 0x02,   // 总线电压 (R, 16-bit 无符号)
        REG_POWER            = 0x03,   // 功率     (R, 16-bit 无符号)
        REG_CURRENT          = 0x04,   // 电流     (R, 16-bit 有符号)
        REG_CALIBRATION      = 0x05,   // 校准     (R/W)
        REG_MASK_ENABLE      = 0x06,   // 掩码/使能 (R/W, reset=0x0000)
        REG_ALERT_LIMIT      = 0x07,   // 报警阈值 (R/W)
        REG_MANUFACTURER_ID  = 0xFE,   // 制造商 ID (R, 0x5449)
        REG_DIE_ID           = 0xFF,   // 芯片 ID   (R, 0x2260)
    };

    // ── CONFIG 寄存器枚举 ─────────────────────────────────

    enum class Reset : uint8_t    { Normal = 0, SoftReset = 1 };
    enum class AvgSample : uint8_t
    {
        N1 = 0, N4 = 1, N16 = 2, N64 = 3,
        N128 = 4, N256 = 5, N512 = 6, N1024 = 7,
    };
    enum class ConvTime : uint8_t
    {
        Us140 = 0, Us204 = 1, Us332 = 2, Us588 = 3,
        Us1100 = 4, Us2116 = 5, Us4156 = 6, Us8244 = 7,
    };
    enum class OperatingMode : uint8_t
    {
        PowerDown          = 0x00,
        ShuntTrigger       = 0x01, BusTrigger         = 0x02,
        ShuntBusTrigger    = 0x03,
        Shutdown           = 0x04,
        ShuntContinuous    = 0x05, BusContinuous      = 0x06,
        ShuntBusContinuous = 0x07,
    };

    // ── Mask/Enable 配置位 ────────────────────────────────

    enum class Alatch : uint8_t  { Transparent = 0, Latch = 1 };
    enum class Apol : uint8_t    { ActiveLow = 0, ActiveHigh = 1 };

    // ============================================================
    //  Config 结构体（用户可定制）
    // ============================================================

    struct Config
    {
        // ── CONFIG 寄存器 ──
        Reset         reset         = Reset::Normal;
        AvgSample     averaging     = AvgSample::N1;
        ConvTime      busConvTime   = ConvTime::Us1100;
        ConvTime      shuntConvTime = ConvTime::Us1100;
        OperatingMode mode          = OperatingMode::ShuntBusContinuous;

        // ── 报警配置（Mask/Enable 寄存器 bit 1-0） ──
        Alatch        alatch        = Alatch::Latch;        // 锁存使能
        Apol          apol          = Apol::ActiveLow;      // 开漏低有效

        // ── 报警引脚自动配置（0=不使用） ──
        uint32_t      alert_port    = 0;               // GPIO 端口，如 GPIOB
        uint32_t      alert_pin     = 0;               // 引脚掩码，如 GPIO_PIN_5

        // ── 校准参数 ──
        uint16_t calibration = 0x0800;   // CAL 寄存器值
        uint16_t alertLimit  = 0x0000;   // Alert Limit 寄存器值

        // ── 用户参数 ──
        int32_t maxCurrent_mA = 81;      // 预期最大电流 mA（推导 Current LSB）
    };

    // ============================================================
    //  构造
    // ============================================================

    explicit INA226(device_i2c_bus* bus, uint8_t addr = ADDR_0x40);
    explicit INA226(device_i2c_bus* bus, uint8_t addr, const Config& cfg);

    INA226(const INA226&) = delete;
    INA226& operator=(const INA226&) = delete;

    ~INA226();

    void init();

    // ============================================================
    //  测量 API
    // ============================================================

    int32_t getBusVoltage_mV();
    int32_t getShuntVoltage_uV();
    int32_t getCurrent_mA();
    int32_t getPower_mW();

    // ============================================================
    //  报警引脚 / 标志
    // ============================================================

    void bindAlertPin(io_ctrl *pin);
    [[nodiscard]] bool isAlertAsserted();

    /** @brief 便捷方法：等同 isAlertAsserted() */
    [[nodiscard]] bool CheckAlert() { return isAlertAsserted(); }

    [[nodiscard]] INA226_AlertFlags readAlertFlags();
    void clearAlertLatch();

    /** @brief 读取 Mask/Enable 原始值（调试用） */
    uint16_t readMaskEnableRaw();

    // ============================================================
    //  运行时配置
    // ============================================================

    void setConfig(const Config& cfg);
    void setAveraging(AvgSample mode);
    void setBusConvTime(ConvTime time);
    void setShuntConvTime(ConvTime time);
    void setOperatingMode(OperatingMode mode);
    void setCalibration(uint16_t cal);
    void setAlertLimit(uint16_t limit);

    // ============================================================
    //  设备信息
    // ============================================================

    uint16_t getManufacturerID();
    uint16_t getDieID();

    // ============================================================
    //  调试
    // ============================================================

    void dumpRegisters();

private:

    device_i2c_dev _dev;
    Config         _cfg;
    io_ctrl       *_alertPin    = nullptr;   // 外部报警引脚（高级用户 bindAlertPin 用）
    uint32_t       _alertPort    = 0;         // 自动配置的 GPIO 端口
    uint32_t       _alertPinMask = 0;         // 自动配置的引脚掩码
    bool           _alertCfg     = false;     // 是否已自动配置

    int32_t _currentLSB_uA;              // Current LSB (µA)

    // ── I2C 读写（INA226 全部 16-bit 寄存器） ────────────

    void     writeRaw(uint8_t reg, uint16_t data);
    uint16_t readRaw(uint8_t reg);

    // ── 寄存器构建 ────────────────────────────────────────

    uint16_t buildConfigReg()   const;
    uint16_t buildMaskEnable()  const;   // 仅配置位

    // ── 符号扩展 ──────────────────────────────────────────

    template<uint8_t Bits>
    static constexpr int64_t signExtend(uint64_t raw)
    {
        constexpr int shift = 64 - Bits;
        return (static_cast<int64_t>(raw << shift)) >> shift;
    }

    // ── 转换 ──────────────────────────────────────────────

    static int32_t rawToShuntVoltage_uV(uint16_t raw);  // 2.5 µV/LSB = 5/2
    static int32_t rawToBusVoltage_mV(uint16_t raw);    // 1.25 mV/LSB = 5/4
    int32_t        rawToCurrent_mA(uint16_t raw) const; // raw × LSB / 1000
    int32_t        rawToPower_mW(uint16_t raw) const;   // raw × 25 × LSB / 1000

    // ── 更新 Current LSB ──────────────────────────────────

    void _updateCurrentLSB();

    // ── 调试 ──────────────────────────────────────────────

    void captureSnapshot(INA226_Snapshot& snap);
    void dumpRawRegisters(const INA226_Snapshot& snap);
    void dumpEngineeringData(const INA226_Snapshot& snap);
};

#endif
