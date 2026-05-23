#include "../App/inc/private.h"

const uint32_t BaudValue[] = {
    110,     // 0
    300,     // 1
    600,     // 2
    1200,    // 3
    2400,    // 4
    4800,    // 5
    9600,    // 6
    19200,   // 7
    38400,   // 8
    57600,   // 9
    115200,  // 10
    128000,  // 11
    230400,  // 12
    256000,  // 13
    460800,  // 14
    500000,  // 15
    512000,  // 16
    600000,  // 17
    750000,  // 18
    921600,  // 19
    1000000, // 20
    1500000, // 21
    2000000  // 22
};

const uint32_t BaudValue_Size = sizeof(BaudValue) / sizeof(BaudValue[0]);

// 写入
void Save_BaudValue(uint32_t baud, uint8_t interface)
{
    uint32_t EEPROM_ADDR_BAUD_VAL = 0; // 局部变量，避免静态变量的副作用
    switch (interface)
    {
    case 0:
        EEPROM_ADDR_BAUD_VAL = EEPROM_ADDR_BAUD_VAL0;
        break;
    case 1:
        EEPROM_ADDR_BAUD_VAL = EEPROM_ADDR_BAUD_VAL1;
        break;
    case 2:
        EEPROM_ADDR_BAUD_VAL = EEPROM_ADDR_BAUD_VAL2;
        break;
    default:
        // 如果 interface 无效，直接返回或处理错误
        return;
    }

    // 写入波特率值到 EEPROM
    uint8_t *p = (uint8_t *)&baud;
    for (int i = 0; i < 4; i++)
    {
        ee_Write_Byte(EEPROM_ADDR_BAUD_VAL + i, p[i]);
    }
}



// 读取 + 验证
uint32_t Load_BaudValue(uint8_t interface)
{
    uint32_t EEPROM_ADDR_BAUD_VAL = 0; // 局部变量，避免静态变量的副作用
    switch (interface)
    {
    case 0:
        EEPROM_ADDR_BAUD_VAL = EEPROM_ADDR_BAUD_VAL0;
        break;
    case 1:
        EEPROM_ADDR_BAUD_VAL = EEPROM_ADDR_BAUD_VAL1;
        break;
    case 2:
        EEPROM_ADDR_BAUD_VAL = EEPROM_ADDR_BAUD_VAL2;
        break;
    default:
        // 如果 interface 无效，返回默认值
        return 115200;
    }

    uint32_t baud = 0;
    uint8_t *p = (uint8_t *)&baud;
    for (int i = 0; i < 4; i++)
    {
        p[i] = ee_Read_Byte(EEPROM_ADDR_BAUD_VAL + i);
    }

    // 验证是否在合法范围内
    // 使用统一的全局数组 BaudValue 校验
    for (size_t i = 0; i < sizeof(BaudValue) / sizeof(BaudValue[0]); i++)
    {
        if (baud == BaudValue[i])
            return baud;
    }
    return 115200; // 非法数据返回默认
}


void config_baudrate(uint32_t baud, uint8_t interface)
{
    Save_BaudValue(baud,interface);
    // baud =Load_BaudValue(interface);
}

void interface_init(uint8_t interface)
{

    uint32_t baud = Load_BaudValue(interface);
    switch (interface)
    {
    case 0:
        debug_init_config(baud);
        break;
    case 1:
        Rs485_Init_config(baud);
        break;

    case 2:
        usartx_config(baud);
        break;
    default:
        break;
    }
}

