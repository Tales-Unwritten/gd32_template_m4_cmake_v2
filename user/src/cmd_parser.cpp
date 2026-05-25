#include "cmd_parser.hpp"
#include "link.h"

#include <cstring>
#include <cstdio>
#include <cstdarg>

// ============================================================
//  行缓冲（双缓冲，无堆分配）
// ============================================================

static constexpr uint16_t LINE_BUF_SIZE = 256;
static constexpr uint8_t  LINE_POOL     = 2;     // 双缓冲：收一行、处理一行

static char     g_line_buf[LINE_POOL][LINE_BUF_SIZE];
static uint16_t g_line_len[LINE_POOL]  = {};       // 每行已接收长度
static uint8_t  g_line_rdy = 0;                    // 就绪行数
static uint8_t  g_line_wr  = 0;                    // 当前写入行
static uint8_t  g_line_rd  = 0;                    // 当前读取行

// ============================================================
//  输出回调
// ============================================================

static cmd_output_t g_output_fn = nullptr;

void cmd_set_output(cmd_output_t fn)
{
    g_output_fn = fn;
}

// ============================================================
//  数据喂入
// ============================================================

void cmd_feed(const uint8_t *data, uint16_t len)
{
    for (uint16_t i = 0; i < len; ++i)
    {
        char c = static_cast<char>(data[i]);

        if (c == '\r') continue;   // 忽略回车

        if (c == '\n')
        {
            // 行结束：标记就绪，切换到下一个写入缓冲
            if (g_line_len[g_line_wr] > 0)
            {
                g_line_buf[g_line_wr][g_line_len[g_line_wr]] = '\0';
                g_line_rdy++;
                g_line_wr = (g_line_wr + 1) % LINE_POOL;
                g_line_len[g_line_wr] = 0;
            }
        }
        else if (g_line_len[g_line_wr] < LINE_BUF_SIZE - 1)
        {
            g_line_buf[g_line_wr][g_line_len[g_line_wr]++] = c;
        }
        // 溢出：静默丢弃
    }
}

bool cmd_has_line()
{
    return g_line_rdy > 0;
}

const char *cmd_get_line()
{
    if (g_line_rdy == 0) return nullptr;

    const char *line = g_line_buf[g_line_rd];
    g_line_rd = (g_line_rd + 1) % LINE_POOL;
    g_line_rdy--;
    return line;
}

// ============================================================
//  命令查找
// ============================================================

const cmd_entry *cmd_find(const char *name)
{
    for (int i = 0; i < g_cmd_count; ++i)
    {
        if (std::strcmp(g_cmd_table[i].name, name) == 0)
            return &g_cmd_table[i];
    }
    return nullptr;
}

int cmd_count()
{
    return g_cmd_count;
}

// ============================================================
//  命令分发
// ============================================================

void cmd_dispatch(const char *line)
{
    if (!line || line[0] == '\0') return;

    // ── Tokenize：分割命令名和参数 ──
    char  buf[LINE_BUF_SIZE];
    char *argv[16];
    int   argc = 0;

    std::strncpy(buf, line, sizeof(buf) - 1);
    buf[sizeof(buf) - 1] = '\0';

    char *token = std::strtok(buf, " \t");
    while (token && argc < 16)
    {
        argv[argc++] = token;
        token = std::strtok(nullptr, " \t");
    }

    if (argc == 0) return;

    // ── 内置 help 命令 ──
    if (std::strcmp(argv[0], "help") == 0)
    {
        cmd_printf("\r\n=== Commands (%d) ===\r\n", g_cmd_count);
        for (int i = 0; i < g_cmd_count; ++i)
        {
            cmd_printf("  %-20s - %s\r\n",
                       g_cmd_table[i].name,
                       g_cmd_table[i].help ? g_cmd_table[i].help : "");
        }
        cmd_printf("  %-20s - %s\r\n", "help", "显示此帮助");
        return;
    }

    // ── 查命令表并执行 ──
    const cmd_entry *cmd = cmd_find(argv[0]);
    if (cmd && cmd->handler)
    {
        cmd->handler(argc, argv);
    }
    else
    {
        cmd_printf("unknown command: %s (type 'help' for list)\r\n", argv[0]);
    }
}

// ============================================================
//  格式化输出
// ============================================================

void cmd_printf(const char *fmt, ...)
{
    char buf[128];
    va_list args;
    va_start(args, fmt);
    int len = vsnprintf(buf, sizeof(buf), fmt, args);
    va_end(args);

    if (len <= 0) return;

    if (g_output_fn)
    {
        g_output_fn(reinterpret_cast<const uint8_t *>(buf),
                    static_cast<uint16_t>(len));
    }
    else
    {
        debug_uart.send_data(reinterpret_cast<const uint8_t *>(buf),
                             static_cast<uint16_t>(len));
    }
}
