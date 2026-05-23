#pragma ocne
#include <stdint.h>
#include <string.h>
#include <stdio.h>
#include <stdbool.h>
#include "../Hardware/inc/debug_uart.h"
#include "../Hardware/inc/rs485.h"
#include "../Hardware/inc/ina226.h"
#include "../Hardware/inc/ina228.h"
#include "../Hardware/inc/usart_text.h"
#include "../App/inc/private.h"
static char send_buffer[128]; // Define send_buffer with sufficient size


// 内部校验逻辑
static bool is_valid_baudrate(uint32_t baudrate)
 {
    for (uint32_t i = 0; i < BaudValue_Size; i++) {
        if (baudrate == BaudValue[i]) return true;
    }
    return false;
}



// 通用设置函数（通过interface参数区分接口）
static void BaudrateSet(uint32_t baudrate, uint8_t interface)
{
    if (!is_valid_baudrate(baudrate))
    {
        snprintf(send_buffer, sizeof(send_buffer), "Error: Invalid baudrate %u\r\n", baudrate);
        return;
    }

    uint32_t BaudValue = 0;
    config_baudrate(baudrate, 0);
    BaudValue = Load_BaudValue(0);
    // printf("BaudValue:%d\r\n",BaudValue);
    // printf("baudrate:%d\r\n",baudrate);

    switch (interface)
    {
    case 0: // Debug interface
        debug_init_config(BaudValue);
        snprintf(send_buffer, sizeof(send_buffer), "Debug baudrate set to %u\r\n", BaudValue);
        break;
    case 1: // RS485 interface
        Rs485_Init_config(BaudValue);
        // snprintf(send_buffer, sizeof(send_buffer), "RS485 baudrate set to %u\r\n", BaudValue);
        break;
    case 2: // USART interface
        usartx_config(BaudValue);
        // snprintf(send_buffer, sizeof(send_buffer), "USART baudrate set to %u\r\n", BaudValue);
        break;
    default:
        snprintf(send_buffer, sizeof(send_buffer), "Error: Unknown interface %u\r\n", interface);
        break;
    }
}




// // 波特率有效性检查（集中管理）
// static bool is_valid_baudrate(uint32_t baudrate)
// {
//     static const uint32_t valid_baudrates[] = {
//         9600, 14400, 19200, 38400, 56000, 57600, 115200, 128000, 230400, 256000,
//         460800, 500000, 512000, 600000, 750000, 912600, 1000000, 1500000, 2000000};

//     for (uint32_t i = 0; i < sizeof(valid_baudrates) / sizeof(valid_baudrates[0]); i++)
//     {
//         if (baudrate == valid_baudrates[i])
//             return true;
//     }
//     return false;
// }






// static void BaudrateSet(uint32_t baudrate, uint8_t interface) {
//     if (!is_valid_baudrate(baudrate)) {
//         snprintf(send_buffer, sizeof(send_buffer), "Error: %u is not a standard baudrate\r\n", baudrate);
//         return;
//     }

//     // 1. 准备应答（此时还在旧波特率下）
//     snprintf(send_buffer, sizeof(send_buffer), "Baudrate changing to %u... Done\r\n", baudrate);
    
//     // 2. 写入 EEPROM 记录
//     Save_BaudValue(baudrate, interface);

//     // 3. 标记需要切换硬件（在 command_parsing 循环中处理）
//     // 此处仅做标记，具体的硬件操作放在发送完消息之后
// }




// 命令处理函数类型
typedef void (*cmd_handler_t)(void);

// 命令处理函数
static void handle_bus_voltage(void)
{
    float val = INA226_Device_Info.CHG_Ina226_Get_Bus_Voltage();
    sprintf((char *)send_buffer, "Vbus_Val=%0.6f\r\n", val);
}

static void handle_current(void)
{
    float val = INA226_Device_Info.CHG_Ina226_Get_Current();
    sprintf((char *)send_buffer, "Curr_Val=%0.6f\r\n", val);
}

static void handle_shunt_voltage(void)
{
    float val = INA226_Device_Info.CHG_Ina226_Get_Shunt_Voltage();
    sprintf((char *)send_buffer, "shunt_voltage=%0.4f\r\n", val);
}

static void handle_power(void)
{
    uint16_t val = INA226_Device_Info.CHG_Ina226_Get_Power();
    sprintf((char *)send_buffer, "Power_Value=0x%04X\r\n", val);
}

static void handle_unlock_alert(void)
{
    // uint16_t val = INA226_Device_Info.CHG_Ina226_Get_Die_ID();
    // sprintf((char *)send_buffer, "Alert is Ulock=0x%04X\r\n", val);
    INA226_Device_Info.CHG_Ina226_Get_Die_ID();
    sprintf((char *)send_buffer, "Alert is Ulock\r\n");
}

static void handle_manufacturer_id(void)
{
    uint16_t val = INA226_Device_Info.CHG_Ina226_Get_Manufacturer_ID();
    sprintf((char *)send_buffer, "Manufacturer_ID=0x%04X\r\n", val);
}

static void handle_die_id(void)
{
    uint16_t val = INA226_Device_Info.CHG_Ina226_Get_Die_ID();
    sprintf((char *)send_buffer, "Die_ID=0x%04X\r\n", val);
}


//INA228指令

static void handle_ina228_get_bus_voltage(void)
{
    int32_t val = INA228_Device_Func.CHG_INA228_Get_Bus_Voltage();
    // printf("Raw Bus Voltage: %d (20-bit signed)\r\n", val);
    sprintf((char *)send_buffer, "INA228_Vbus_Val=%04duV\r\n", val); // 转换为 mV，保留小数部分
}

static void handle_ina228_get_bus_current(void)
{
    int32_t val=INA228_Device_Func.CHG_INA228_Get_Current();
    sprintf((char *)send_buffer, "INA228_Curr_Val=%04dmA\r\n", val/1000);
}

static void handle_ina228_get_shunt_voltage(void)
{
    int32_t val=INA228_Device_Func.CHG_INA228_Get_Shunt_Voltage();
    sprintf((char *)send_buffer, "INA228_shunt_voltage=%06d\r\n", val);
}

static void handle_ina228_get_power(void)
{
    float val=INA228_Device_Func.CHG_INA228_Get_Power();
    sprintf((char *)send_buffer, "INA228_Power_Value=%0.6f\r\n", val);
}

static void handle_ina228_unlock_alert(void)
{
    INA228_Device_Func.CHG_INA228_Unlock_Alert();
    sprintf((char *)send_buffer, "INA228_Alert is Ulock\r\n");
}

static void handle_ina228_get_manufacturer_id(void)
{
    uint16_t val = INA228_Device_Func.CHG_INA228_Get_Manufacturer_ID();
    sprintf((char *)send_buffer, "INA228_Manufacturer_ID=0x%04X\r\n", val);
}

static void handle_ina228_get_device_id(void)
{
    uint16_t val = INA228_Device_Func.CHG_INA228_Get_Device_ID();
    sprintf((char *)send_buffer, "INA228_Device_ID=0x%04X\r\n", val);
}

static void handle_ina228_get_temperature(void)
{
    float val=INA228_Device_Func.CHG_INA228_Get_Temperature();
    sprintf((char *)send_buffer, "INA228_Temperature=%0.4f\r\n", val);
}

static void handle_ina228_text_online(void)
{
    INA228_Device_Func.CHG_INA228_Text_Online();
}

static void handle_ina228_get_energy(void)
{
    float val=INA228_Device_Func.CHG_INA228_Get_Energy();
    sprintf((char *)send_buffer, "INA228_Energy=%0.6f\r\n", val);
}

static void handle_ina228_get_charge(void)
{
    float val=INA228_Device_Func.CHG_INA228_Get_Charge();
    sprintf((char *)send_buffer, "INA228_Charge=%0.6f\r\n", val);
}


// 修改结构体定义：第三个字段改为接口标识（0=Debug, 1=RS485）
typedef struct
{
    const char *cmd_prefix;                                // 命令前缀（不含参数）
    void (*handler)(uint32_t baudrate, uint8_t interface); // 处理函数
    uint8_t interface;                                     // 接口类型标识
} Baud_struct;

// 仅需2个表项
static const Baud_struct baud_table[] = {
    {"Set DebugBaud ", BaudrateSet, 0}, // Debug接口
    {"Set Rs485Baud ", BaudrateSet, 1}, // Rs485接口
    {"Set UartBaud ",  BaudrateSet, 2}, // Usart接口
};

// 命令表
typedef struct
{
    const char *cmd;
    cmd_handler_t handler;
} cmd_struct;

static const cmd_struct cmd_table[] = 
{
    {"get_bus_voltage\r\n", handle_bus_voltage},
    {"get_current\r\n", handle_current},
    {"get_shunt_voltage\r\n", handle_shunt_voltage},
    {"get_power\r\n", handle_power},
    {"ulock_alert\r\n", handle_unlock_alert},
    {"get_manufacturer_id\r\n", handle_manufacturer_id},
    {"get_die_id\r\n", handle_die_id},
    {"ina228_get_bus_voltage\r\n", handle_ina228_get_bus_voltage},
    {"ina228_get_bus_current\r\n", handle_ina228_get_bus_current},
    {"ina228_get_shunt_voltage\r\n", handle_ina228_get_shunt_voltage},
    {"ina228_get_power\r\n", handle_ina228_get_power},
    {"ina228_ulock_alert\r\n", handle_ina228_unlock_alert},
    {"ina228_get_manufacturer_id\r\n", handle_ina228_get_manufacturer_id},
    {"ina228_get_device_id\r\n", handle_ina228_get_device_id},
    {"ina228_get_temperature\r\n", handle_ina228_get_temperature},
    {"ina228_text_online\r\n", handle_ina228_text_online},
    {"ina228_get_energy\r\n", handle_ina228_get_energy},
    {"ina228_get_charge\r\n", handle_ina228_get_charge}
};

typedef struct { 
    const char *recv_cmd;  // receive command
    const char *resp_cmd;  // response command
} ChannelCmdDef;


// bool is_valid_baudrate(uint32_t baudrate);
// void BaudrateSet(uint32_t baudrate, uint8_t interface);
void command_parsing(PC_Receive_Buffer_t *PC_Receive_Buffer, void (*Usart_Send_Data)(uint8_t *buf, uint8_t count));
