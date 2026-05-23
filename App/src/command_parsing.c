#include "../App/inc/command_parsing.h"

static void handle_channel_all_open(void)    // 打开所有通道
{

}
static void handle_channel_all_close(void)   // 关闭所有通道  
{

}
static void handle_channel_open(uint8_t ch)  // 打开指定通道
{

}
static void handle_channel_close(uint8_t ch) // 关闭指定通道
{

}

// /**
//  * @brief 解析并执行通道控制命令 (Channel01_open / Channel01_close / Channel_all_open 等)
//  * @param cmd 接收到的命令字符串
//  * @param response_buf 用于填充响应字符串的缓冲区
//  * @return true: 命令匹配并执行成功 / false: 不是通道命令或执行失败
//  */
// static bool parse_channel_command(const char *cmd, char *response_buf)
// {
//     int channel_id = 0;
//     char action[10] = {0};

//     // 1. 优先处理特殊命令：Channel_all_open / Channel_all_close
//     if (strncmp(cmd, "Channel_all_open\r\n", 18) == 0)
//     {
//         handle_channel_all_open(); 
//         snprintf(response_buf, 64, "Channel_is_all_open\r\n");
//         return true;
//     }
//     if (strncmp(cmd, "Channel_all_close\r\n", 19) == 0)
//     {
//         handle_channel_all_close();
//         snprintf(response_buf, 64, "Channel_is_all_close\r\n");
//         return true;
//     }

//     // 2. 解析标准格式：Channel%02d_open\r\n 或 Channel%02d_close\r\n
//     // sscanf 返回 2 表示成功解析出 通道号 + 动作
//     if (sscanf(cmd, "Channel%02d_%9s\r\n", &channel_id, action) == 2)
//     {
//         // 验证通道号范围
//         if (channel_id < 1 || channel_id > 16)
//         {
//             return false; // 通道号非法，交给后续逻辑处理
//         }

//         // 根据动作执行对应操作
//         if (strcmp(action, "open") == 0)
//         {
//             handle_channel_open(channel_id); // 你需要实现这个统一处理函数
//             snprintf(response_buf, 64, "Channel%02d_is_open\r\n", channel_id);
//             return true;
//         }
//         else if (strcmp(action, "close") == 0)
//         {
//             handle_channel_close(channel_id);
//             snprintf(response_buf, 64, "Channel%02d_is_close\r\n", channel_id);
//             return true;
//         }
//     }
//     // 3. 格式不匹配，说明不是通道命令
//     return false;
// }

// /**
//  * @brief 命令解析函数
//  * @param PC_Receive_Buffer 存放接收数据的结构体
//  * @param Usart_Send_Data 需要传入的发送函数指针
//  * @param buf 传入的发送函数指针的数据，
//  * @param count 传入的发送函数指针的数据的长度
//  * @return 
//  */
// void command_parsing(PC_Receive_Buffer_t *PC_Receive_Buffer, void (*Usart_Send_Data)(uint8_t *buf, uint8_t count))
// {
//     for (size_t i = 0; i < CHCHE_COUNT; i++)
//     {
//         // 检查缓冲区是否有有效数据
//         if (PC_Receive_Buffer[i].Buffer_Status != 1 || PC_Receive_Buffer[i].Buffer_Length <= 3)
//         {
//             continue;
//         }

//         bool command_found = false;
//         char response_buf[64] = {0}; // 统一响应缓冲区

//         // 🔹 优先级1: 尝试解析通道命令（动态解析，替代原 channel_cmd 查表）
//         if (parse_channel_command((char *)PC_Receive_Buffer[i].Buffer, response_buf))
//         {
//             Usart_Send_Data((uint8_t *)response_buf, strlen(response_buf));
//             command_found = true;
//         }

//         // 🔹 优先级2: 查固定命令表 (cmd_table)
//         if (!command_found)
//         {
//             for (size_t j = 0; j < sizeof(cmd_table) / sizeof(cmd_table[0]); j++)
//             {
//                 if (memcmp(PC_Receive_Buffer[i].Buffer, cmd_table[j].cmd, strlen(cmd_table[j].cmd)) == 0)
//                 {
//                     cmd_table[j].handler();
//                     Usart_Send_Data((uint8_t *)send_buffer, strlen(send_buffer));
//                     command_found = true;
//                     break;
//                 }
//             }
//         }

//         // 🔹 优先级3: 解析波特率命令 (baud_table，支持参数)
//         if (!command_found)
//         {
//             for (size_t j = 0; j < sizeof(baud_table) / sizeof(baud_table[0]); j++)
//             {
//                 size_t prefix_len = strlen(baud_table[j].cmd_prefix);
//                 if (strncmp((char *)PC_Receive_Buffer[i].Buffer, baud_table[j].cmd_prefix, prefix_len) == 0)
//                 {
//                     const char *p = (char *)PC_Receive_Buffer[i].Buffer + prefix_len;
//                     uint32_t baudrate = 0;

//                     while (*p == ' ')
//                         p++; // 跳过空格
//                     while (*p >= '0' && *p <= '9')
//                     {
//                         baudrate = baudrate * 10 + (*p - '0');
//                         p++;
//                     }

//                     if (baudrate > 0 && (*p == '\r' || *p == '\n'))
//                     {
//                         baud_table[j].handler(baudrate, baud_table[j].interface);
//                         Usart_Send_Data((uint8_t *)send_buffer, strlen(send_buffer));
//                         command_found = true;
//                         break;
//                     }
//                 }
//             }
//         }

//         // 未知命令处理（可选）
//         // if (!command_found) {
//         //     Usart_Send_Data((uint8_t *)"unknown_command\r\n", 18);
//         // }

//         // 清理缓冲区
//         memset(send_buffer, 0, sizeof(send_buffer)); // 清空发送缓冲区
//         memset(PC_Receive_Buffer[i].Buffer, 0, PC_Receive_Buffer[i].Buffer_Length);
//         PC_Receive_Buffer[i].Buffer_Status = 0;
//         PC_Receive_Buffer[i].Buffer_Length = 0;
//     }
// }


/**
 * @brief 解析并执行通道控制命令 
 * @param cmd 接收到的命令字符串
 * @param response_buf 用于填充响应字符串的缓冲区
 * @return true: 命令匹配并执行成功 / false: 不是通道命令或执行失败
 */
static bool parse_channel_command(const char *cmd, char *response_buf)
{
    // 1. 快速前缀拦截：如果连 "Channel" 都不是，直接返回
    if (strncmp(cmd, "Channel", 7) != 0)
    {
        return false;
    }

    const char *p = cmd + 7; // 指向 "Channel" 后面的字符

    // 2. 匹配 "_all_open" / "_all_close"
    if (strncmp(p, "_all_open\r\n", 11) == 0)
    {
        handle_channel_all_open(); 
        strcpy(response_buf, "Channel_is_all_open\r\n"); // 确定安全的长度直接用 strcpy 更快
        return true;
    }
    if (strncmp(p, "_all_close\r\n", 12) == 0)
    {
        handle_channel_all_close();
        strcpy(response_buf, "Channel_is_all_close\r\n");
        return true;
    }

    // 3. 匹配具体的通道号 (例如 "01_open")
    // 检查接下来的两个字符是否都是数字
    if (p[0] >= '0' && p[0] <= '9' && p[1] >= '0' && p[1] <= '9')
    {
        // ASCII转数字：比如 '0' 和 '1' -> 0 * 10 + 1 = 1
        int channel_id = (p[0] - '0') * 10 + (p[1] - '0');
        
        if (channel_id >= 1 && channel_id <= 16)
        {
            p += 2; // 指向通道号后面的下划线 "_open"
            
            if (strncmp(p, "_open", 5) == 0)
            {
                handle_channel_open(channel_id);
                snprintf(response_buf, 64, "Channel%02d_is_open\r\n", channel_id);
                return true;
            }
            else if (strncmp(p, "_close", 6) == 0)
            {
                handle_channel_close(channel_id);
                snprintf(response_buf, 64, "Channel%02d_is_close\r\n", channel_id);
                return true;
            }
        }
    }

    return false;
}

/**
 * @brief 命令解析函数
 */
void command_parsing(PC_Receive_Buffer_t *PC_Receive_Buffer, void (*Usart_Send_Data)(uint8_t *buf, uint8_t count))
{
    for (size_t i = 0; i < CHCHE_COUNT; i++)
    {
        // 检查缓冲区是否有有效数据
        if (PC_Receive_Buffer[i].Buffer_Status != 1 || PC_Receive_Buffer[i].Buffer_Length <= 3)
        {
            continue;
        }

        bool command_found = false;
        char response_buf[64] = {0};
        char *recv_cmd = (char *)PC_Receive_Buffer[i].Buffer;

        //  优先级1: 尝试解析通道命令
        if (parse_channel_command(recv_cmd, response_buf))
        {
            // 注意：你原始参数定义第二个是 count，所以这里用 strlen，强转匹配你的接口
            Usart_Send_Data((uint8_t *)response_buf, (uint8_t)strlen(response_buf));
            command_found = true;
        }

        // 🔹 优先级2: 查固定命令表 (cmd_table)
        if (!command_found)
        {
            for (size_t j = 0; j < sizeof(cmd_table) / sizeof(cmd_table[0]); j++)
            {
                // 优化：将 strlen 提取到变量中，或者信任编译器的优化
                if (memcmp(recv_cmd, cmd_table[j].cmd, strlen(cmd_table[j].cmd)) == 0)
                {
                    cmd_table[j].handler();
                    Usart_Send_Data((uint8_t *)send_buffer, (uint8_t)strlen(send_buffer));
                    command_found = true;
                    break;
                }
            }
        }

        // 🔹 优先级3: 解析波特率命令 (baud_table)
        if (!command_found)
        {
            for (size_t j = 0; j < sizeof(baud_table) / sizeof(baud_table[0]); j++)
            {
                size_t prefix_len = strlen(baud_table[j].cmd_prefix);
                if (strncmp(recv_cmd, baud_table[j].cmd_prefix, prefix_len) == 0)
                {
                    const char *p = recv_cmd + prefix_len;
                    uint32_t baudrate = 0;

                    while (*p == ' ') p++;
                    while (*p >= '0' && *p <= '9')
                    {
                        baudrate = baudrate * 10 + (*p - '0');
                        p++;
                    }

                    if (baudrate > 0 && (*p == '\r' || *p == '\n'))
                    {
                        baud_table[j].handler(baudrate, baud_table[j].interface);
                        Usart_Send_Data((uint8_t *)send_buffer, (uint8_t)strlen(send_buffer));
                        command_found = true;
                        break;
                    }
                }
            }
        }

        // 🔹 清理动作统一执行
        memset(send_buffer, 0, sizeof(send_buffer)); // 如果 send_buffer 是全局的，这里清理没问题
        memset(PC_Receive_Buffer[i].Buffer, 0, PC_Receive_Buffer[i].Buffer_Length);
        PC_Receive_Buffer[i].Buffer_Status = 0;
        PC_Receive_Buffer[i].Buffer_Length = 0;
    }
}