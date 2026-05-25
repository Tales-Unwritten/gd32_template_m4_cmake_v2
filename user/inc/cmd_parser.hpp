#pragma once

#ifdef __cplusplus

#include <cstdint>

/**
 * @brief  串口命令解析框架（命令表驱动）
 *
 * 设计原则：
 *  - 添加新命令只需在两个位置改动：① 写 handler 函数 ② 在命令表加一行
 *  - 框架代码（cmd_dispatch / cmd_find / tokenize）永远不变
 *  - 零堆分配，全部栈上操作
 *  - 内置 help 命令自动生成命令列表
 *
 * 使用方式：
 * @code
 *   // ── 1. 定义 handler ──
 *   static void cmd_get_voltage(int argc, char *argv[]) {
 *       cmd_printf("voltage=%ld mV\n", (long)ina.getBusVoltage_mV());
 *   }
 *
 *   // ── 2. 注册到命令表（唯一需要改的地方） ──
 *   CMD_TABLE_BEGIN
 *       CMD("get_voltage", cmd_get_voltage, "读取总线电压 mV")
 *       CMD("get_current", cmd_get_current, "读取电流 mA")
 *   CMD_TABLE_END
 *
 *   // ── 3. 在 UART 接收回调中调用 ──
 *   void on_uart_rx(const uint8_t *data, uint16_t len, void *ctx) {
 *       cmd_feed(data, len);           // 喂数据
 *       while (cmd_has_line()) {
 *           cmd_dispatch(cmd_get_line());  // 解析并执行
 *       }
 *   }
 * @endcode
 */

// ============================================================
//  类型定义
// ============================================================

/** @brief 命令处理函数签名：argc=参数个数, argv=参数列表 */
typedef void (*cmd_handler_t)(int argc, char *argv[]);

/** @brief 命令表条目 */
struct cmd_entry
{
    const char    *name;        // 命令名（不含空格）
    cmd_handler_t  handler;     // 处理函数
    const char    *help;        // 帮助文本（用于 help 命令）
};

// ============================================================
//  命令表注册宏
// ============================================================

/**
 * 命令表是全局 const 数组，添加命令只需在 BEGIN/END 之间加 CMD() 行。
 * 放在某个 .cpp 文件中（建议统一放在 cmd_table.cpp）。
 *
 * @code
 *   CMD_TABLE_BEGIN
 *       CMD("get_voltage", cmd_get_voltage, "读取总线电压")
 *       CMD("get_current", cmd_get_current, "读取电流")
 *   CMD_TABLE_END
 * @endcode
 */
#define CMD_TABLE_BEGIN \
    inline const cmd_entry g_cmd_table[] = {

#define CMD(name, func, desc) \
    { name, func, desc },

#define CMD_TABLE_END \
    }; \
    inline const int g_cmd_count = sizeof(g_cmd_table) / sizeof(g_cmd_table[0]);

// ============================================================
//  框架 API
// ============================================================

/**
 * @brief 喂入接收到的数据（内部维护行缓冲，攒到换行后触发解析）
 * @param data  接收到的字节
 * @param len   字节数
 */
void cmd_feed(const uint8_t *data, uint16_t len);

/**
 * @brief  是否有完整行待解析
 * @return true=有完整行
 */
bool cmd_has_line();

/**
 * @brief  取出当前行（调用后内部移动到下一行）
 * @return 指向内部行缓冲的指针（以 \0 结尾）
 */
const char *cmd_get_line();

/**
 * @brief  解析并执行一行命令
 * @param  line  以 \0 结尾的命令字符串
 *
 * 流程：分割命令名 → 查命令表 → 调用 handler
 * 如果命令名是 "help"，自动打印所有已注册命令。
 */
void cmd_dispatch(const char *line);

/**
 * @brief  在命令表中查找命令
 * @param  name  命令名
 * @return 找到返回条目指针，未找到返回 nullptr
 */
const cmd_entry *cmd_find(const char *name);

/**
 * @brief  获取命令表条目数
 */
int cmd_count();

/**
 * @brief  格式化输出（handler 中用 cmd_printf 代替 printf）
 *
 * 内部调用项目的串口输出函数。需要根据实际串口实现调整。
 */
#ifdef __cplusplus
void cmd_printf(const char *fmt, ...) __attribute__((format(printf, 1, 2)));
#else
void cmd_printf(const char *fmt, ...);
#endif

/**
 * @brief  设置输出回调（用于对接不同串口）
 *
 * 默认使用 debug_uart.send_data()。如果要用其他串口，
 * 在 init 阶段调用此函数替换。
 */
typedef void (*cmd_output_t)(const uint8_t *data, uint16_t len);
void cmd_set_output(cmd_output_t fn);

// ============================================================
//  外部声明（由用户定义）
// ============================================================

/** @brief 命令表（由 CMD_TABLE_BEGIN/END 宏生成） */
extern const cmd_entry g_cmd_table[];
extern const int       g_cmd_count;

#endif /* __cplusplus */
