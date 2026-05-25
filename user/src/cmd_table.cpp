#include "cmd_parser.hpp"

// ============================================================
//  示例命令（复制此模板，替换为实际命令）
// ============================================================

static void cmd_get_voltage(int argc, char *argv[])
{
    (void)argc; (void)argv;
    cmd_printf("get_voltage: TODO\r\n");
}

static void cmd_get_current(int argc, char *argv[])
{
    (void)argc; (void)argv;
    cmd_printf("get_current: TODO\r\n");
}

// ============================================================
//  命令表（添加新命令只改这里）
// ============================================================

CMD_TABLE_BEGIN
    CMD("get_voltage", cmd_get_voltage, "读取总线电压 mV")
    CMD("get_current", cmd_get_current, "读取电流 mA")
CMD_TABLE_END
