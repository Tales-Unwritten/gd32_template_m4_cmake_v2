#include "../Hardware/inc/ina228.h"

I2C_Info INA228_i2C;

static void INA228_Init(uint32_t GPIOx, uint16_t SCL_Pin, uint16_t SDA_Pin, INA228_Addr_enum Addr)
{   
	Soft_I2C_Init(&INA228_i2C, GPIOx, SCL_Pin, SDA_Pin, Addr);                                                                                                                                             
}

// 寄存器长度表（按 INA228 手册）
static uint8_t INA228_RegLen(uint16_t reg)
{
    switch (reg)
    {
    case Cmd_Voltage_Shunt_Register:
    case Cmd_Bus_Voltage_Register1:
    case Cmd_Current_Register1:
    case Cmd_Power_Register1:
        return 3;   // 24-bit

    case Cmd_Energt_Register:
    case Cmde_Charge_Register:
        return 5;   // 40-bit

    default:
        return 2;   // default 16-bit registers
    }
}

static int32_t SignExtend20(uint32_t x)
{
    if (x & (1 << 19))            // 20-bit sign bit
        x |= 0xFFF00000;          // 扩展到32bit
    return (int32_t)x;
}

static int32_t SignExtend24(uint32_t x)
{
    if (x & (1 << 23))        // 24-bit sign bit
        x |= 0xFF000000;      // extend to negative
    return (int32_t)x;
}

static int64_t SignExtend40(uint64_t x)
{
    if (x & ((uint64_t)1 << 39))     // bit 39 = sign
        x |= 0xFFFFFF0000000000ULL;  // extend to 64-bit negative
    return (int64_t)x;
}


void Ina228_WriteRegister(I2C_Info *i2c, INA228_Addr_enum Device_Add, 
                          INA228_Cmd_enum RegAdd, uint64_t Data)
{
    uint8_t len = INA228_RegLen(RegAdd);

    Soft_I2C_Start(i2c);
    Soft_I2C_WriteByte(i2c, Device_Add << 1);    // Write mode
    Soft_I2C_ReceiveAck(i2c);

    Soft_I2C_WriteByte(i2c, RegAdd);             // Register address
    Soft_I2C_ReceiveAck(i2c);

    // 从高位字节开始写（和手册一致）
    for (int8_t i = len - 1; i >= 0; i--)
    {
        uint8_t byte_to_write = (Data >> (8 * i)) & 0xFF;

        Soft_I2C_WriteByte(i2c, byte_to_write);
        Soft_I2C_ReceiveAck(i2c);
    }

    Soft_I2C_Stop(i2c);
}


uint64_t Ina228_ReadRegister(I2C_Info *i2c, INA228_Addr_enum Device_Add, uint16_t RegAddress)
{
    uint64_t Data = 0;
    uint8_t len = INA228_RegLen(RegAddress);

    //----------------- 写寄存器地址 -----------------
    Soft_I2C_Start(i2c);
    Soft_I2C_WriteByte(i2c, Device_Add << 1);
    Soft_I2C_ReceiveAck(i2c);
    Soft_I2C_WriteByte(i2c, RegAddress);
    Soft_I2C_ReceiveAck(i2c);

    //----------------- 读取数据 -----------------
    Soft_I2C_Start(i2c);
    Soft_I2C_WriteByte(i2c, (Device_Add << 1) | 0x01);
    Soft_I2C_ReceiveAck(i2c);

    for (uint8_t i = 0; i < len; i++)
    {
        uint8_t rx = Soft_I2C_ReceiveByte(i2c);

        // shift and append
        Data = (Data << 8) | rx;

        // 最后 1 字节 NACK；其他 ACK
        // 向I2C总线写入应答信号，如果是最后一个字节则写入NACK(1)，否则写入ACK(0)
        Soft_I2C_WriteAck(i2c, (i == (len - 1)) ? 1 : 0);
    }

    Soft_I2C_Stop(i2c);

    return Data;
}


INA228_Config_Info_t INA228_Config_Info =
{
	.Config_Reset              = Reset_No_Reset,
	.Config_ResACC             = Normal_Operation,
	.Config_Init_Delay_Conver  = Init_Delay_Time_0s,
	.Config_Temperature        = Temper_Comperm_Enable,
	.Config_Range              = Range_40_96mV,
	.Config_Reserved           = Reserved_Config_Bit,
    .Calibration_Value         = 0x0FBB,          // Calibration register
    .Vbus_Alert_Limit          = 0x0EC0,          // Vbus Alert Limit register
    .Vshunt_Alert_Limit        = 0x1F95,          // Vshunt Alert Limit register
    .Vtemp_Alert_Limit         = 0x7FFF,          // Vtemp Alert Limit register
    .Current_LSB               = MAX_EXPECTED_CURRENT / (1 << 19),            // Current LSB value

};

INA228_ADC_Config_Info_t INA228_ADC_Config_Info =
{
	.ADC_Mode_Config           = Mode_Continuous_Shunt_Bus_Voltage,
	.ADC_Vbus_Conv_Time        = Vbus_Conv_Time_2074us,
	.ADC_Vshunt_Conv_Time      = Vshunt_Conv_Time_1052us,
	.ADC_Temp_Conv_Time        = Temp_Conv_Time_280us,
	.ADC_Avg_Sample            = Avg_Mode_256_Samples,	
};

INA228_Diag_Alert_Info_t INA228_Diag_Alert_Info=
{
	.Alert_Clock_Config        = Alert_Clock_Enable,
	.Alert_Ready_Flag_Config   = Alert_Ready_Flag_Disable,
	.Alert_Asserted_Config     = Alert_Asserted_Disable,
	.Alert_Polarity_Config     = Alert_Active_Low,
	.Eenrgy_Status             = Energy_Normal,
	.Charge_Status             = Charge_Normal,
	.Arithmetic_Overflow       = Arithmetic_Normal,
	.Reserved_Bit              = Reserved_Bit,
	.Temperature_Over_Limit    = Temp_Over_Normal,
	.Shunt_Voltage_Over_Limit  = Shunt_Voltage_Over_Limit,
	.Shunt_Voltage_Under_Limit = Shunt_Voltage_Under_Limit,
	.Bus_Voltage_Over_Limit    = Bus_Voltage_Over_Limit,
	.Bus_Voltage_Under_Limit   = Bus_Voltage_Under_Limit,
	.Power_Over_Limit          = Power_Normal,
	.Conversion_Ready          = Conversion_Not_Ready,
	.Checksum_Status           = Checksum_Normal,
};


static uint16_t INA228_BuildConfigRegister(INA228_Config_Info_t *Info)
{
	uint16_t Config_Register = 0;
	Config_Register |= (Info->Config_Reset << 15);               // Reset bit
	Config_Register |= (Info->Config_ResACC << 14);              // Reserved for ACC bit
	Config_Register |= (Info->Config_Init_Delay_Conver << 6);    // Initial ADC conversion delay bits
	Config_Register |= (Info->Config_Temperature << 5);          // Shunt Temperature Compensation bit
	Config_Register |= (Info->Config_Range << 4);                // Shunt full scale range selection bits
	Config_Register |= (Info->Config_Reserved << 0);             // Reserved bits
	return Config_Register;
}

static uint16_t INA228_BuildADCConfigRegister(INA228_ADC_Config_Info_t *Info)
{
	uint16_t ADC_Config_Register = 0;
	ADC_Config_Register |= (Info->ADC_Mode_Config << 12);        // ADC operating mode bits
	ADC_Config_Register |= (Info->ADC_Vbus_Conv_Time << 8);      // Bus voltage conversion time bits
	ADC_Config_Register |= (Info->ADC_Vshunt_Conv_Time << 4);    // Shunt voltage conversion time bits
	ADC_Config_Register |= (Info->ADC_Temp_Conv_Time << 2);      // Temperature conversion time bits
	ADC_Config_Register |= (Info->ADC_Avg_Sample << 0);          // Sample averaging mode bits
	return ADC_Config_Register;
};

static uint16_t INA228_BuildAlertRegister(INA228_Diag_Alert_Info_t *Info)
{
	uint16_t Alert_Register = 0;
	Alert_Register |= (Info->Alert_Clock_Config << 15);          // Alert Clock configuration bit
	Alert_Register |= (Info->Alert_Ready_Flag_Config << 14);     // Conversion Ready Flag configuration bit
	Alert_Register |= (Info->Alert_Asserted_Config << 13);       // Alert Asserted configuration bit
	Alert_Register |= (Info->Alert_Polarity_Config << 12);       // Alert Polarity configuration bit
	Alert_Register |= (Info->Eenrgy_Status << 11);               // Energy register status bit
	Alert_Register |= (Info->Charge_Status << 10);               // Charge register status bit
	Alert_Register |= (Info->Arithmetic_Overflow << 9);          // Arithmetic overflow status bit
	Alert_Register |= (Info->Reserved_Bit << 8);                 // Reserved bit
	Alert_Register |= (Info->Temperature_Over_Limit << 7);       // Temperature over-limit status bit
	Alert_Register |= (Info->Shunt_Voltage_Over_Limit << 6);     // Shunt voltage over-limit status bit
	Alert_Register |= (Info->Shunt_Voltage_Under_Limit << 5);    // Shunt voltage under-limit status bit
	Alert_Register |= (Info->Bus_Voltage_Over_Limit << 4);       // Bus voltage over-limit status bit
	Alert_Register |= (Info->Bus_Voltage_Under_Limit << 3);      // Bus voltage under-limit status bit
	Alert_Register |= (Info->Power_Over_Limit << 2);             // Power over-limit status bit
	Alert_Register |= (Info->Conversion_Ready << 1);             // Conversion ready status bit
	Alert_Register |= (Info->Checksum_Status << 0);              // Checksum status bit
	return Alert_Register;
};


static void INA228_BuildConfiguration(void)
{
    uint16_t Config_Reg = INA228_BuildConfigRegister(&INA228_Config_Info);
    Ina228_WriteRegister(&INA228_i2C, Ina228_7bit_address0, Cmd_Config_Register1, Config_Reg);

    uint16_t ADC_Config_Reg = INA228_BuildADCConfigRegister(&INA228_ADC_Config_Info);
    Ina228_WriteRegister(&INA228_i2C, Ina228_7bit_address0, Cmd_ADC_Config_Register, ADC_Config_Reg);

    uint16_t Alert_Reg = INA228_BuildAlertRegister(&INA228_Diag_Alert_Info);
    Ina228_WriteRegister(&INA228_i2C, Ina228_7bit_address0, Cmd_Diagnostic_Alert_Register, Alert_Reg);

    Ina228_WriteRegister(&INA228_i2C, Ina228_7bit_address0, Cmd_Shunt_Calibration_Register, INA228_Config_Info.Calibration_Value);

    Ina228_WriteRegister(&INA228_i2C, Ina228_7bit_address0, Cmd_Shunt_Overvoltage_Register, INA228_Config_Info.Vshunt_Alert_Limit);

    // Ina228_WriteRegister(&INA228_i2C, Ina228_7bit_address0, Cmd_Bus_Overvoltage_Register, INA228_Config_Info.Vbus_Alert_Limit);
}


static int32_t INA228_Get_Shunt_Voltage(void)
{
    uint32_t raw = Ina228_ReadRegister(&INA228_i2C,
                                       Ina228_7bit_address0,
                                       Cmd_Voltage_Shunt_Register);

    raw >>= 4;
    int64_t signedVal = SignExtend20(raw);
    // LSB = 312.5 nV = 0.0000003125 V
    signedVal=(signedVal * 3125)/1000;
    return signedVal; // 单位：nV
}

// static int32_t INA228_Get_Bus_Voltage(void)
// {
//     // uint32_t raw = Ina228_ReadRegister(
//     //     &INA228_i2C,
//     //     Ina228_7bit_address0,
//     //     Cmd_Bus_Voltage_Register1
//     // );

//     // // 移除低4位保留位
//     // raw >>= 4;

//     // // 因为手册写 Twos Complement → 必须符号扩展
//     // int32_t signedVal = SignExtend20(raw);

//     // // 1 bit = 195.3125 uV = 1953125 / 10 uV
//     // // 为了避免小数，放大 10 倍再除
//     // int64_t uV_x10 = (int64_t)signedVal * 1953125;
//     // return (int32_t)((uV_x10 + 5) / 10);

//     uint32_t raw = Ina228_ReadRegister(
//         &INA228_i2C,
//         Ina228_7bit_address0,
//         Cmd_Bus_Voltage_Register1);

    
//     raw >>= 4;// 去掉低 4 位保留位

//     // 20-bit Two's Complement 符号扩展
//     int32_t signedVal = SignExtend20(raw);

//     // 1 bit = 195.3125 uV = 1953125 / 10 uV
//     // 使用 int64 防止乘法溢出
//     int64_t uV_x10 = (int64_t)signedVal * 1953125;

//     // 转回 uV，带四舍五入
//     int32_t uV = (int32_t)((uV_x10 + 5) / 10);

//     return uV; // 单位：uV
// }

static int32_t INA228_Get_Bus_Voltage_uV(void)
{
    uint32_t raw = Ina228_ReadRegister(&INA228_i2C,
                                       Ina228_7bit_address0,
                                       Cmd_Bus_Voltage_Register1);

    raw >>= 4;           // 移除低4位保留位
    // 因为手册写 Twos Complement → 必须符号扩展
    int64_t signedVal = SignExtend20(raw);
    // printf("Raw Bus Voltage: 0x%X, Signed: %ld\n", raw, signedVal);

    // LSB = 195.3125 µV = 0.0001953125 V
    // return (float)signedVal * 0.0001953125f;
    signedVal= (signedVal * 1953125)/10000;
    return signedVal; // 单位：uV
}


static int32_t INA228_Get_Bus_Current(void)
{
    uint32_t raw = Ina228_ReadRegister(&INA228_i2C,
                                       Ina228_7bit_address0,
                                       Cmd_Current_Register1);

    uint32_t val20 = (raw >> 4) & 0xFFFFF;   // 提取20bit有效数据
    // uint32_t val20 = (raw >> 4);   // 提取20bit有效数据

    // 20-bit two's complement 符号扩展
    if (val20 & (1 << 19))
        val20 |= 0xFFF00000;
        
    int32_t signedVal = (int32_t)val20;

    float val = (float)signedVal * INA228_Config_Info.Current_LSB;

    return val;
}

//获取温度 7.8125m°C/LSB
static float INA228_Get_Temperature(void)
{
    float raw = 0.00f;
    int16_t temp_val;
    temp_val = Ina228_ReadRegister(&INA228_i2C, Ina228_7bit_address0, Cmd_Temperature_Register);
    // Conversion factor: 7.8125m°C/LSB
    raw = (float)temp_val * 0.0078125f;
    return raw;
}






//获取电能 电源 LSB x 16 = 976.5625µJ/LSB
static float INA228_Get_Energy(void)
{
    int32_t energy_val = Ina228_ReadRegister(&INA228_i2C, Ina228_7bit_address0, Cmd_Energt_Register);
    int32_t signedVal = SignExtend40(energy_val);
    float val = (float)signedVal * 0.0009765625f;
    return val;
}

// 获取电荷 电流 LSB = 19.073486µC/LSB
static float INA228_Get_Charge(void)
{
    int32_t charge_val = Ina228_ReadRegister(&INA228_i2C, Ina228_7bit_address0, Cmde_Charge_Register);
    int32_t signedVal = SignExtend40(charge_val);
    float val = (float)signedVal * 0.000019073486f;
    // float val = (float)signedVal * INA228_Config_Info.Current_LSB;
    return val;
}

static float INA228_Get_Power(void)
{
    float raw =0.00f;
    uint16_t power_val;
    power_val = Ina228_ReadRegister(&INA228_i2C, Ina228_7bit_address0, Cmd_Power_Register1);
    // Conversion factor: 61.035156µW/LSB
    raw = (float)power_val * 0.000061035156f;
    return raw;
}

static uint16_t INA228_Get_Manufacturer_ID(void)
{
    uint16_t MANUFACTURER_ID=Ina228_ReadRegister(&INA228_i2C,Ina228_7bit_address0,Cmd_Manufacturer_ID);
    return MANUFACTURER_ID;
}

static uint16_t INA228_Get_Die_ID(void)
{
    uint16_t Die_ID=Ina228_ReadRegister(&INA228_i2C,Ina228_7bit_address0,Cmd_Device_ID);
    return Die_ID;
}


void INA228_Unlock_Alert(void)
{
	uint16_t Mask_Enable_Register = 0;
	Mask_Enable_Register = Ina228_ReadRegister(&INA228_i2C, Ina228_7bit_address0, Cmd_Diagnostic_Alert_Register);
}



static void INA228_Text_Online(void)
{
    // 读取所有寄存器原始数据

    uint16_t raw_Config = Ina228_ReadRegister(&INA228_i2C, Ina228_7bit_address0, Cmd_Config_Register1);
    uint16_t raw_ADC_Config = Ina228_ReadRegister(&INA228_i2C, Ina228_7bit_address0, Cmd_ADC_Config_Register);
    uint16_t raw_Shunt_Calibration = Ina228_ReadRegister(&INA228_i2C, Ina228_7bit_address0, Cmd_Shunt_Calibration_Register);
    uint16_t raw_Diag_Alert = Ina228_ReadRegister(&INA228_i2C, Ina228_7bit_address0, Cmd_Diagnostic_Alert_Register);
    
    uint32_t raw_Shunt_Voltage = Ina228_ReadRegister(&INA228_i2C, Ina228_7bit_address0, Cmd_Voltage_Shunt_Register);
    uint32_t raw_Bus_Voltage = Ina228_ReadRegister(&INA228_i2C, Ina228_7bit_address0, Cmd_Bus_Voltage_Register1);    
    uint16_t raw_Temperature = Ina228_ReadRegister(&INA228_i2C, Ina228_7bit_address0, Cmd_Temperature_Register);
    uint32_t raw_Current = Ina228_ReadRegister(&INA228_i2C, Ina228_7bit_address0, Cmd_Current_Register1);
    uint16_t raw_Power = Ina228_ReadRegister(&INA228_i2C, Ina228_7bit_address0, Cmd_Power_Register1);
    uint64_t raw_Energy = Ina228_ReadRegister(&INA228_i2C, Ina228_7bit_address0, Cmd_Energt_Register);
    uint64_t raw_Charge = Ina228_ReadRegister(&INA228_i2C, Ina228_7bit_address0, Cmde_Charge_Register);

    // 延时2ms

    uint16_t Die_ID = INA228_Device_Func.CHG_INA228_Get_Device_ID();
    uint16_t MANUFACTURER_ID = INA228_Device_Func.CHG_INA228_Get_Manufacturer_ID();
    
    // 延时2ms
    
    float Shunt_Voltage = INA228_Device_Func.CHG_INA228_Get_Shunt_Voltage();
    
    float Bus_Voltage = INA228_Device_Func.CHG_INA228_Get_Bus_Voltage();
    
    float Bus_Current = INA228_Device_Func.CHG_INA228_Get_Current();
    
    float Temperature = INA228_Device_Func.CHG_INA228_Get_Temperature();
    
    float Power = INA228_Device_Func.CHG_INA228_Get_Power();
    
    float Energy = INA228_Device_Func.CHG_INA228_Get_Energy();
    
    float Charge = INA228_Device_Func.CHG_INA228_Get_Charge();
    // delay_ms(2);
    // 打印所有测试结果
    printf("========== INA228 Test Results ==========\r\n");
    printf("Bus_Voltage:     %.6f V\r\n", Bus_Voltage);
    printf("Bus_Current:     %.6f A\r\n", Bus_Current);
    printf("Shunt_Voltage:   %.6f mV\r\n", Shunt_Voltage * 1000);
    printf("Temperature:     %.2f C\r\n", Temperature);
    printf("Power:           %.6f W\r\n", Power);
    printf("Energy:          %.6f J\r\n", Energy);
    printf("Charge:          %.6f C\r\n", Charge);
    printf("MANUFACTURER_ID: 0x%04X\r\n", MANUFACTURER_ID);
    printf("Die_ID:          0x%04X\r\n", Die_ID);
    printf("=========================================\r\n\r\n");

    // 延时2ms
    // delay_ms(2);
    //打印原始寄存器数据
    printf("========== INA228 Register Test Results ==========\r\n");
    printf("Raw Config Register:          0x%04X\r\n", raw_Config);
    printf("Raw ADC Config Register:      0x%04X\r\n", raw_ADC_Config);
    printf("Raw Shunt Calibration Reg:    0x%04X\r\n", raw_Shunt_Calibration);
    printf("Raw Diag Alert Register:      0x%04X\r\n", raw_Diag_Alert);
    printf("---------------------------------------------------\r\n");

    printf("Raw Shunt Voltage Register:   0x%06X\r\n", raw_Shunt_Voltage);
    printf("Raw Bus Voltage Register:     0x%06X\r\n", raw_Bus_Voltage);
    printf("Raw Temperature Register:     0x%04X\r\n", raw_Temperature);
    printf("Raw Current Register:         0x%06X\r\n", raw_Current);
    printf("Raw Power Register:           0x%04X\r\n", raw_Power);
    printf("Raw Energy Register:          0x%010llX\r\n", raw_Energy);
    printf("Raw Charge Register:          0x%010llX\r\n", raw_Charge);
    printf("===================================================\r\n\r\n");

}

static void INA228_Config(uint32_t GPIOx, uint16_t SCL_Pin, uint16_t SDA_Pin, INA228_Addr_enum Addr)
{
    INA228_Init(GPIOx, SCL_Pin, SDA_Pin, Addr);
    INA228_BuildConfiguration();
}
const INA228_Device_Func_t INA228_Device_Func =
{
    .CHG_INA228_Config               = INA228_Config,
    .CHG_INA228_Get_Manufacturer_ID  = INA228_Get_Manufacturer_ID,
    .CHG_INA228_Get_Device_ID        = INA228_Get_Die_ID,
    .CHG_INA228_Get_Shunt_Voltage    = INA228_Get_Shunt_Voltage,
    .CHG_INA228_Get_Bus_Voltage      = INA228_Get_Bus_Voltage_uV,
    .CHG_INA228_Get_Temperature      = INA228_Get_Temperature,
    .CHG_INA228_Unlock_Alert         = INA228_Unlock_Alert,
    .CHG_INA228_Get_Current          = INA228_Get_Bus_Current,    
    .CHG_INA228_Get_Energy           = INA228_Get_Energy,
    .CHG_INA228_Get_Charge           = INA228_Get_Charge,
    .CHG_INA228_Get_Power            = INA228_Get_Power,
    .CHG_INA228_Text_Online          = INA228_Text_Online,
};
