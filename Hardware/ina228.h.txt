#pragma once

#include "gd32f4xx.h"
#include "systick.h"
#include "../Hardware/inc/soft_i2c.h"


#define MAX_EXPECTED_CURRENT  40.0f   // 最大预期电流，单位：A

typedef enum
{
	Ina228_7bit_address0                                       = 0x40, // A1：0 A0：0 - 7位地址：01000000
	Ina228_7bit_address1                                       = 0x41, // A1：0 A0：1 - 7位地址：01000001
	Ina228_7bit_address3                                       = 0x44, // A1：1 A0：0 - 7位地址：01000100
	Ina228_7bit_address4                                       = 0x45  // A1：1 A0：1 - 7位地址：01000101
} INA228_Addr_enum;	                                      			   // INA228 7位I2C地址枚举，由A1和A0引脚状态决定

typedef enum
{
	Cmd_Config_Register1                                       = 0x00,    // Configuration register (Read_Write) 配置寄存器
	Cmd_ADC_Config_Register                                    = 0x01,    // ADC configuration register (Read_Write) ADC配置寄存器
	Cmd_Shunt_Calibration_Register                             = 0x02,    // Shunt calibration register (Read_Write) 分流校准寄存器
	Cmd_Shunt_Temperature_Coefficient_Register                 = 0x03,    // Shunt temperature coefficient register (Read_Write) 分流温度系数寄存器
	Cmd_Voltage_Shunt_Register                                 = 0x04,    // Shunt voltage register (Only Read) 分流电压寄存器
	Cmd_Bus_Voltage_Register1                                  = 0x05,    // Bus voltage register (Only Read) 总线电压寄存器
	Cmd_Temperature_Register                                   = 0x06,    // Temperature register (Only Read) 温度寄存器
	Cmd_Current_Register1                                      = 0x07,    // Current register (Only Read) 电流寄存器
	Cmd_Power_Register1                                        = 0x08,    // Power register (Only Read) 功率寄存器
	Cmd_Energt_Register                                        = 0x09,    // Energy register (Read_Write) 能量寄存器
	Cmde_Charge_Register                                       = 0x0A,    // Charge register (Read_Write) 电荷寄存器
	Cmd_Diagnostic_Alert_Register                              = 0x0B,    // Diagnostic alert register (Read_Write) 诊断报警寄存器
	Cmd_Shunt_Overvoltage_Register                             = 0x0C,    // Shunt overvoltage threshold register (Read_Write) 分流过压阈值寄存器
	Cmd_Shunt_Undervoltage_Register                            = 0x0D,    // Shunt undervoltage threshold register (Read_Write) 分流欠压阈值寄存器
	Cmd_Bus_Overvoltage_Register                               = 0x0E,    // Bus overvoltage threshold register (Read_Write) 总线过压阈值寄存器
	Cmd_Bus_Undervoltage_Register                              = 0x0F,    // Bus undervoltage threshold register (Read_Write) 总线欠压阈值寄存器
	Cmd_Temperature_Over_Limit_Register                        = 0x10,    // Temperature over-limit threshold register (Read_Write) 温度超限阈值寄存器
	Cmd_Power_Over_Limit_Register                              = 0x11,    // Power over-limit threshold register (Read_Write) 功率超限阈值寄存器
	Cmd_Manufacturer_ID                                        = 0x3E,    // Manufacturer ID register (Only Read) 制造商ID寄存器
	Cmd_Device_ID                                              = 0x3F     // Device ID register (Only Read) 设备ID寄存器

} INA228_Cmd_enum;                                                        // INA228寄存器地址枚举


// Configuration register bit field================================================================================================

// bit 15
typedef enum
{
	Reset_No_Reset                                             = 0x00,    // No reset
	Reset_Soft_Reset                                           = 0x01     // Software reset
} INA228_Config_Reset_enum;                                               // INA228复位枚举

// bit 14
typedef enum
{
	Normal_Operation                                           = 0x00,    // Normal operation
	Clears_ACC_Values                                          = 0x01     // Clears registers to default values for ENERGY and CHARGE registers
} INA228_Config_ResACC_enum;                                              // INA228 ACC复位枚举

// Sets the Delay for initial ADC conversion in steps of 2 ms,  bit 13-6
typedef enum
{
	Init_Delay_Time_0s                                         = 0x00,    // Initial delay time 0s
	Init_Delay_Time_2ms                                        = 0x01,    // Initial delay time 2ms
	Init_Delay_Time_510ms                                      = 0xFF     // Initial delay time 510ms

} INA228_Config_Init_Delay_Conver_enum;                                   // INA228初始延迟转换枚举

// bit 5
typedef enum
{
	Temper_Compern_Disable                                     = 0x00,    // Shunt Temperature Compensation Disabled
	Temper_Comperm_Enable                                      = 0x01     // Shunt Temperature Compensation Enabled

} INA228_Config_Temperature_enum;                                         // INA228温度补偿枚举

// Shunt full scale range selection across,  bit 4
typedef enum
{
	Range_163_84mV                                             = 0x00,    // Shunt full scale range 163.84mV
	Range_40_96mV                                              = 0x01     // Shunt full scale range 40.96mV
} INA228_Config_Range_enum;                                               // INA228分流量程选择枚举

// bit 3-0
typedef enum
{
	Reserved_Config_Bit                                        = 0x00     // Reserved. Always reads 0

} INA228_Config_Reserved_enum;                                            // INA228配置保留位枚举

//=====================================================================================================================================




// ADC_Configuration register bit field================================================================================================

typedef enum
{
	Mode_Shuntdown                                			   = 0x00,    // Shunt Voltage, Power-Down
	Mode_Triggerd_Bus_Voltage                     			   = 0x01,    // Triggered Bus Voltage Measurement, single shot
	Mode_Triggerd_Shunt_Voltage                   			   = 0x02,    // Triggered Shunt Voltage Measurement, single shot
	Mode_Triggend_Shunt_Voltage_Bus_Voltage       			   = 0x03,    // Triggered Shunt and Bus Voltage Measurement, single shot
	Mode_Triggerd_Temperature                     			   = 0x04,    // Triggered Temperature Measurement, single shot
	Mode_Triggerd_Temperature_Bus_Voltage         			   = 0x05,    // Triggered Temperature and Bus Voltage Measurement, single shot
	Mode_Triggerd_Temperature_Shunt_Voltage       			   = 0x06,    // Triggered Temperature and Shunt Voltage Measurement, single shot
	Mode_Triggerd_Bus_Voltage_Shunt_Voltage_Temperature        = 0x07,    // Triggered Bus Voltage, Shunt Voltage and Temperature Measurement, single shot
	Mode_Shuntdown_Reserved                                    = 0x08,    // Shunt Voltage, Power-Down (Reserved)
	Mode_Continuous_Bus_Voltage_Only                           = 0x09,    // Continuous Bus Voltage Measurement
	Mode_Continuous_Shunt_Voltage                              = 0x0A,    // Continuous Shunt Voltage Measurement
	Mode_Continuous_Shunt_Bus_Voltage                          = 0x0B,    // Continuous Shunt and Bus Voltage Measurement
	Mode_Continuous_Temperature_Only                           = 0x0C,    // Continuous Temperature Measurement
	Mode_Continuous_Bus_Voltage_Temperature                    = 0x0D,    // Continuous Bus Voltage and Temperature Measurement
	Mode_Continuous_Shunt_Voltage_Temperature                  = 0x0E,    // Continuous Shunt Voltage and Temperature Measurement
	Mode_Continuous_Bus_Shunt_Temperature                      = 0x0F     // Continuous Bus Voltage, Shunt Voltage and Temperature Measurement

} INA228_ADC_Mode_Config_enum;                                            // INA228配置模式枚举

typedef enum
{
	Vbus_Conv_Time_50us                                        = 0x00,    // 50us    - Bus voltage conversion time
	Vbus_Conv_Time_84us                                        = 0x01,    // 84us    - Bus voltage conversion time
	Vbus_Conv_Time_150us                                       = 0x02,    // 150us   - Bus voltage conversion time
	Vbus_Conv_Time_280us                                       = 0x03,    // 280us   - Bus voltage conversion time
	Vbus_Conv_Time_540us                                       = 0x04,    // 540us   - Bus voltage conversion time
	Vbus_Conv_Time_1052us                                      = 0x05,    // 1052us  - Bus voltage conversion time
	Vbus_Conv_Time_2074us                                      = 0x06,    // 2074us  - Bus voltage conversion time
	Vbus_Conv_Time_4120us                                      = 0x07     // 4120us  - Bus voltage conversion time

} INA228_ADC_Vbus_Conv_Time_enum;                                         // INA228总线电压转换时间枚举

typedef enum
{
	Vshunt_Conv_Time_50us                                      = 0x00,    // 50us    - Shunt voltage conversion time
	Vshunt_Conv_Time_84us                                      = 0x01,    // 84us    - Shunt voltage conversion time
	Vshunt_Conv_Time_150us                                     = 0x02,    // 150us   - Shunt voltage conversion time
	Vshunt_Conv_Time_280us                                     = 0x03,    // 280us   - Shunt voltage conversion time
	Vshunt_Conv_Time_540us                                     = 0x04,    // 540us   - Shunt voltage conversion time
	Vshunt_Conv_Time_1052us                                    = 0x05,    // 1052us  - Shunt voltage conversion time
	Vshunt_Conv_Time_2074us                                    = 0x06,    // 2074us  - Shunt voltage conversion time
	Vshunt_Conv_Time_4120us                                    = 0x07     // 4120us  - Shunt voltage conversion time

} INA228_ADC_Vshunt_Conv_Time_enum;                                       // INA228分流电压转换时间枚举


typedef enum
{
	Temp_Conv_Time_50us                                        = 0x00,    // 50us    - Temperature conversion time
	Temp_Conv_Time_84us                                        = 0x01,    // 84us    - Temperature conversion time
	Temp_Conv_Time_150us                                       = 0x02,    // 150us   - Temperature conversion time
	Temp_Conv_Time_280us                                       = 0x03,    // 280us   - Temperature conversion time
	Temp_Conv_Time_540us                                       = 0x04,    // 540us   - Temperature conversion time
	Temp_Conv_Time_1052us                                      = 0x05,    // 1052us  - Temperature conversion time
	Temp_Conv_Time_2074us                                      = 0x06,    // 2074us  - Temperature conversion time
	Temp_Conv_Time_4120us                                      = 0x07     // 4120us  - Temperature conversion time
} INA228_ADC_Temp_Conv_Time_enum;                                         // INA228温度转换时间枚举

typedef enum
{
	Avg_Mode_1_Sample                                          = 0x00,    // 1     sample averaging mode     - 1次采样平均模式
	Avg_Mode_4_Samples                                         = 0x01,    // 4     sample averaging mode     - 4次采样平均模式
	Avg_Mode_16_Samples                                        = 0x02,    // 16    sample averaging mode     - 16次采样平均模式
	Avg_Mode_64_Samples                                        = 0x03,    // 64    sample averaging mode     - 64次采样平均模式
	Avg_Mode_128_Samples                                       = 0x04,    // 128   sample averaging mode     - 128次采样平均模式
	Avg_Mode_256_Samples                                       = 0x05,    // 256   sample averaging mode     - 256次采样平均模式
	Avg_Mode_512_Samples                                       = 0x06,    // 512   sample averaging mode     - 512次采样平均模式
	Avg_Mode_1024_Samples                                      = 0x07     // 1024  sample averaging mode     - 1024次采样平均模式
} INA228_ADC_Avg_Sample_enum;                                             // INA228采样平均模式枚举

//====================================================================================================================================



//Diagnostic Flags and Alert (DIAG_ALERT) Register=====================================================================================

// Config_Alert_Clock, bit 15
typedef enum
{
	Alert_Clock_Disable                                        = 0x00,    // Transparent - 透明模式
	Alert_Clock_Enable                                         = 0x01     // Latched - 锁存模式
} INA228_Alert_Clock_Config_enum;                                         // INA228报警时钟配置枚举

// Configures Conversion Ready Flag.bit14
typedef enum
{
	Alert_Ready_Flag_Disable                                   = 0x00,    // Disable conversion ready flag on ALERT pin - 禁用ALERT引脚转换就绪标志
	Alert_Ready_Flag_Enable                                    = 0x01     // Enables conversion ready flag on ALERT pin - 启用ALERT引脚转换就绪标志
} INA228_Alert_Ready_Flag_Config_enum;                                    // INA228报警就绪标志配置枚举

// When enabled, ALERT function is asserted on the completedaveraged value.bit13
typedef enum
{
	Alert_Asserted_Disable                                     = 0x00,    // ALERT comparison on non-averaged (ADC) value - 基于非平均值(ADC)的ALERT比较
	Alert_Asserted_Enable                                      = 0x01     // ALERT comparison on averaged value - 基于平均值的ALERT比较
} INA228_Alert_Asserted_Config_enum;                                      // INA228报警断言配置枚举

// Alert Polarity bit sets the Alert pin polarity.bit 12
typedef enum
{
	Alert_Active_Low                                           = 0x00,    // Normal (Active-low, open-drain) - 正常模式(低电平有效,开漏)
	Alert_Active_High                                          = 0x01     // Inverted (active-high, open-drain ) - 反相模式(高电平有效,开漏)
} INA228_Alert_Polarity_Config_enum;                                      // INA228报警极性配置枚举




// This bit indicates the health of the ENERGY register.
// If the 40 bit ENERGY register has overflowed this bit is set to 1.
// Clears when the ENERGY register is read. bit 11
typedef enum 
{
	Energy_Normal                                              = 0x00,    // Normal operation
	Energy_Overflowed                                          = 0x01     // Energy register overflowed
} INA228_Eenrgy_Status_enum;


// This bit indicates the health of the CHARGE register.
// If the 40 bit CHARGE register has overflowed this bit is set to 1
// Clears when the CHARGE register is read. bit 10
typedef enum 
{
	Charge_Normal                                              = 0x00,    // Normal operation
	Charge_Overflowed                                          = 0x01     // Charge register overflowed
} INA228_Charge_Status_enum;

// This bit is set to 1 if an arithmetic operation resulted in an overflow error.
// It indicates that current and power data may be invalid.
// Must be manually cleared by triggering another conversion or by
// clearing the accumulators with the RSTACC bit. bit 9
typedef enum
{
	Arithmetic_Normal                                          = 0x00,    // Normal operation
	Arithmetic_Overflowed                                      = 0x01     // Arithmetic overflow occurred
} INA228_Arithmetic_Overflow_enum;

// Reserved bit 8
typedef enum
{
	Reserved_Bit                                               = 0x00     // Reserved. Always reads 0
} INA228_Reserved_Bit_enum;

// This bit is set to 1 if the temperature measurement exceeds the
// threshold limit in the temperature over-limit register. bit 7
typedef enum
{
	Temp_Over_Normal                                           = 0x00,    // Normal operation
	Temp_Over_Limit                                            = 0x01     // Temperature over-limit occurred	
} INA228_Temperature_Over_Limit_enum;

// This bit is set to 1 if the shunt voltage measurement exceeds the
// threshold limit in the shunt over-limit register.
// When ALATCH = 1 this bit is cleared by reading this register. bit 6
typedef enum
{
	Shunt_Voltage_Over_Normal                                  = 0x00,    // Normal operation
	Shunt_Voltage_Over_Limit                                   = 0x01     // Shunt voltage over-limit occurred
} INA228_Shunt_Voltage_Over_Limit_enum;

// This bit is set to 1 if the shunt voltage measurement falls below the
// threshold limit in the shunt under-limit register.
// When ALATCH = 1 this bit is cleared by reading this register. bit 5
typedef enum
{
	Shunt_Voltage_Under_Normal                                 = 0x00,    // Normal operation
	Shunt_Voltage_Under_Limit                                  = 0x01     // Shunt voltage under-limit occurred
} INA228_Shunt_Voltage_Under_Limit_enum;

// This bit is set to 1 if the bus voltage measurement exceeds the
// threshold limit in the bus over-limit register.
// When ALATCH = 1 this bit is cleared by reading this register. bit 4
typedef enum
{
	Bus_Voltage_Over_Normal                                    = 0x00,    // Normal operation
	Bus_Voltage_Over_Limit                                     = 0x01     // Bus voltage over-limit occurred
} INA228_Bus_Voltage_Over_Limit_enum;

// This bit is set to 1 if the bus voltage measurement falls below the
// threshold limit in the bus under-limit register.
// When ALATCH = 1 this bit is cleared by reading this register. bit 3
typedef enum
{
	Bus_Voltage_Under_Normal                                   = 0x00,    // Normal operation
	Bus_Voltage_Under_Limit                                    = 0x01     // Bus voltage under-limit occurred
} INA228_Bus_Voltage_Under_Limit_enum;

// This bit is set to 1 if the power measurement exceeds the threshold
// limit in the power limit register.
// When ALATCH = 1 this bit is cleared by reading this register. bit 2
typedef enum
{
	Power_Normal                                               = 0x00,    // Normal operation
	Power_Over_Limit                                           = 0x01     // Power over-limit occurred
} INA228_Power_Over_Limit_enum;

// This bit is set to 1 if the conversion is completed.
// When ALATCH = 1 this bit is cleared by reading this register or
// starting a new triggered conversion. bit 1
typedef enum
{
	Conversion_Not_Ready                                       = 0x00,    // Conversion not complete
	Conversion_Ready                                           = 0x01     // Conversion complete
} INA228_Conversion_Ready_enum;

// This bit is set to 0 if a checksum error is detected in the device trim
// memory space. bit 0
typedef enum
{
	Checksum_Error                                             = 0x00,    // Memory Checksum Error
	Checksum_Normal                                            = 0x01     // Normal Operation
} INA228_Checksum_Status_enum;

//====================================================================================================================================
typedef struct
{
	INA228_Config_Reset_enum                   Config_Reset;                   // Reset bit
	INA228_Config_ResACC_enum                  Config_ResACC;                  // Reserved for ACC bit
	INA228_Config_Init_Delay_Conver_enum       Config_Init_Delay_Conver;       // Initial ADC conversion delay bits
	INA228_Config_Temperature_enum             Config_Temperature;             // Shunt Temperature Compensation bit
	INA228_Config_Range_enum                   Config_Range;                   // Shunt full scale range selection bits
	INA228_Config_Reserved_enum                Config_Reserved;                // Reserved bits
	uint16_t                                   Calibration_Value;			   // Calibration register
	uint16_t                                   Vbus_Alert_Limit;			   // Vbus Alert Limit register
	uint16_t                                   Vshunt_Alert_Limit;			   // Vshunt Alert Limit register
	uint16_t                                   Vtemp_Alert_Limit;			   // Vtemp Alert Limit register
	float                                      Current_LSB;					   // Alert Limit register

} INA228_Config_Info_t;         // INA228句柄结构体


typedef struct ina228
{
	INA228_ADC_Mode_Config_enum		           ADC_Mode_Config;                 // ADC operating mode bits
	INA228_ADC_Vbus_Conv_Time_enum	           ADC_Vbus_Conv_Time;              // Bus voltage conversion time bits
	INA228_ADC_Vshunt_Conv_Time_enum	       ADC_Vshunt_Conv_Time;            // Shunt voltage conversion time bits
	INA228_ADC_Temp_Conv_Time_enum	           ADC_Temp_Conv_Time;              // Temperature conversion time bits
	INA228_ADC_Avg_Sample_enum		           ADC_Avg_Sample;                  // Sample averaging mode bits

}INA228_ADC_Config_Info_t;     // INA228 ADC配置结构体

typedef struct
{
	INA228_Alert_Clock_Config_enum             Alert_Clock_Config;              // Alert Clock configuration bit
	INA228_Alert_Ready_Flag_Config_enum        Alert_Ready_Flag_Config;         // Conversion Ready Flag configuration bit
	INA228_Alert_Asserted_Config_enum          Alert_Asserted_Config;           // Alert Asserted configuration bit
	INA228_Alert_Polarity_Config_enum          Alert_Polarity_Config;           // Alert Polarity configuration bit
	INA228_Eenrgy_Status_enum                  Eenrgy_Status;                   // Energy register status bit
	INA228_Charge_Status_enum                  Charge_Status;                   // Charge register status bit
	INA228_Arithmetic_Overflow_enum            Arithmetic_Overflow;             // Arithmetic overflow status bit
	INA228_Reserved_Bit_enum                   Reserved_Bit;                    // Reserved bit
	INA228_Temperature_Over_Limit_enum         Temperature_Over_Limit;          // Temperature over-limit status bit
	INA228_Shunt_Voltage_Over_Limit_enum       Shunt_Voltage_Over_Limit;        // Shunt voltage over-limit status bit
	INA228_Shunt_Voltage_Under_Limit_enum      Shunt_Voltage_Under_Limit;       // Shunt voltage under-limit status bit
	INA228_Bus_Voltage_Over_Limit_enum         Bus_Voltage_Over_Limit;          // Bus voltage over-limit status bit
	INA228_Bus_Voltage_Under_Limit_enum        Bus_Voltage_Under_Limit;         // Bus voltage under-limit status bit
	INA228_Power_Over_Limit_enum               Power_Over_Limit;                // Power over-limit status bit
	INA228_Conversion_Ready_enum               Conversion_Ready;                // Conversion ready status bit
	INA228_Checksum_Status_enum                Checksum_Status;                 // Checksum status bit

} INA228_Diag_Alert_Info_t;    // INA228诊断报警结构体



typedef struct  
{
	void  (*CHG_INA228_Config)(uint32_t GPIOx, uint16_t SCL_Pin, uint16_t SDA_Pin, INA228_Addr_enum Addr);
	uint16_t (*CHG_INA228_Get_Manufacturer_ID)(void);
	uint16_t (*CHG_INA228_Get_Device_ID)(void);
	int32_t (*CHG_INA228_Get_Shunt_Voltage)(void);
    int32_t (*CHG_INA228_Get_Bus_Voltage)(void);
	float (*CHG_INA228_Get_Temperature)(void);
	void  (*CHG_INA228_Unlock_Alert)(void);
	int32_t (*CHG_INA228_Get_Current)(void);    
	float (*CHG_INA228_Get_Energy)(void);
	float (*CHG_INA228_Get_Charge)(void);
	float (*CHG_INA228_Get_Power)(void);
	void  (*CHG_INA228_Text_Online)(void);
}INA228_Device_Func_t;

extern const INA228_Device_Func_t INA228_Device_Func;