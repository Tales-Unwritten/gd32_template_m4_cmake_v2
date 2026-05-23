#include "modbus_map.hpp"

/** 设备信息区： */
uint16_t DeviceBlock[10] =
{
        1,
        100,
};
/** 控制参数 */
uint16_t ControlBlock[50];

/** 实时数据 */
uint16_t StatusBlock[50];

/** 区块表 */
typedef struct
{
    uint16_t            start_addr;
    uint16_t            length;
    modbus_block_type_t type;
    uint16_t            *data;

} modbus_block_t;

modbus_block_t ModbusBlockTable[] =
    {

        {0x0000, 10, MODBUS_BLOCK_HOLDING, DeviceBlock},

        {0x000A, 50, MODBUS_BLOCK_HOLDING, ControlBlock},

        {0x0040, 50, MODBUS_BLOCK_INPUT, StatusBlock},

};

/** 区块数量 */
uint16_t BlockCount = sizeof(ModbusBlockTable) / sizeof(modbus_block_t);

uint16_t Modbus_ReadRegister(uint16_t addr,modbus_block_type_t type)
{

    for (uint16_t i = 0; i < BlockCount; i++)
    {

        modbus_block_t *block = &ModbusBlockTable[i];

        if (block->type != type)
        {
            continue;
        }
        if (addr >= block->start_addr && addr < block->start_addr + block->length)
        {
            uint16_t index = addr - block->start_addr;
            return block->data[index];
        }
    }
    return 0;
}


void Modbus_WriteRegister(uint16_t addr, uint16_t value)
{
    for (uint16_t i = 0; i < BlockCount; i++)
    {
        modbus_block_t *block = &ModbusBlockTable[i];
        if (block->type != MODBUS_BLOCK_HOLDING)
        {
            continue;
        }
        if (addr >= block->start_addr && addr < block->start_addr + block->length)
        {
            uint16_t index = addr - block->start_addr;
            block->data[index] = value;
            return;
        }
    }
}
