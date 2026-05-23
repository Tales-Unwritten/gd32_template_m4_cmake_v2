#include "../Hardware/inc/can.h"

FlagStatus receive_flag = RESET;              // CAN接收标志位，用于指示是否有新的CAN消息接收
uint8_t transmit_number = 0x0;                // CAN发送计数器，用于跟踪发送的消息数量
can_trasnmit_message_struct transmit_message; // CAN发送消息结构体，用于存储待发送的CAN帧数据

/* enable can clock */
static void can_gpio_init(void)
{
    /* enable can clock */
    rcu_periph_clock_enable(RCU_GPIOB); // 使能GPIOB端口时钟

#ifdef DEV_CAN0_USED
    rcu_periph_clock_enable(RCU_CAN0);
    gpio_output_options_set(GPIOB, GPIO_OTYPE_PP, GPIO_OSPEED_50MHZ, GPIO_PIN_8 | GPIO_PIN_9);
    gpio_mode_set(GPIOB, GPIO_MODE_AF, GPIO_PUPD_NONE, GPIO_PIN_8 | GPIO_PIN_9);
    gpio_af_set(GPIOB, GPIO_AF_9, GPIO_PIN_8 | GPIO_PIN_9);
#else
    rcu_periph_clock_enable(RCU_CAN1);
    gpio_output_options_set(GPIOB, GPIO_OTYPE_PP, GPIO_OSPEED_50MHZ, GPIO_PIN_12 | GPIO_PIN_13);
    gpio_mode_set(GPIOB, GPIO_MODE_AF, GPIO_PUPD_NONE, GPIO_PIN_12 | GPIO_PIN_13);
    gpio_af_set(GPIOB, GPIO_AF_9, GPIO_PIN_12 | GPIO_PIN_13);
#endif
}

static void can_networking_init(void)
{
    can_parameter_struct can_parameter;
    can_filter_parameter_struct can_filter;

    can_struct_para_init(CAN_INIT_STRUCT, &can_parameter);
    can_struct_para_init(CAN_FILTER_STRUCT, &can_filter);

    can_deinit(CANX);

    // CAN参数配置
    can_parameter.time_triggered = DISABLE;
    can_parameter.auto_bus_off_recovery = ENABLE;
    can_parameter.auto_wake_up = DISABLE;
    can_parameter.auto_retrans = ENABLE;
    can_parameter.rec_fifo_overwrite = DISABLE;
    can_parameter.trans_fifo_order = DISABLE;
    can_parameter.working_mode = CAN_NORMAL_MODE;
    can_parameter.resync_jump_width = CAN_BT_SJW_1TQ;  // 同步跳跃宽度1TQ
    can_parameter.time_segment_1 = CAN_BT_BS1_7TQ;     // 时间段1为7TQ
    can_parameter.time_segment_2 = CAN_BT_BS2_2TQ;     // 时间段2为2TQ
    
    // 1Mbps波特率配置 (240MHz系统时钟)
    can_parameter.prescaler = 5;  // 预分频器24
    
    /*
    // 500Kbps波特率配置 (如果需要)
    can_parameter.prescaler = 48;  // 预分频器48
    */
    

    // 过滤器配置
#ifdef DEV_CAN0_USED
    can_filter.filter_number = 0;
#else
    can_filter.filter_number = 15;
#endif
    
    can_filter.filter_mode = CAN_FILTERMODE_MASK;
    can_filter.filter_bits = CAN_FILTERBITS_32BIT;
    can_filter.filter_list_high = 0x0000;
    can_filter.filter_list_low = 0x0000;
    can_filter.filter_mask_high = 0x0000;
    can_filter.filter_mask_low = 0x0000;
    can_filter.filter_fifo_number = CAN_FIFO1;
    can_filter.filter_enable = ENABLE;
    can_filter_init(&can_filter);

    can_init(CANX, &can_parameter);

    // 检查初始化结果
    if (can_flag_get(CANX, CAN_FLAG_IWS) != SET) {
        printf("CAN initialization may have failed!\n");
    } else {
        printf("CAN controller initialized successfully\n");
    }
}

static void nvic_config(void)
{
#ifdef DEV_CAN0_USED
    /* configure CAN0 NVIC */
    nvic_irq_enable(CAN0_RX1_IRQn, 0, 0); // 启用CAN0接收FIFO1中断，抢占优先级0，子优先级0
#else
    /* configure CAN1 NVIC */
    nvic_irq_enable(CAN1_RX1_IRQn, 0, 0); // 启用CAN1接收FIFO1中断，抢占优先级0，子优先级0
#endif
}

/**
 * @brief CAN发送数据函数
 *
 * 该函数用于发送CAN数据帧，检查数据长度是否符合要求，并填充发送消息结构体。
 *
 * @param data 指向要发送的数据缓冲区的指针
 * @param len 数据长度，最大为8字节
 */
// void can_transmit_data(uint8_t *data, uint8_t len)
// {
//     if (data == NULL || len > 8)
//     {
//         return;
//     }

//     // 建议设置一个有效的ID
//     transmit_message.tx_sfid = 0x123; // 标准帧ID
//     transmit_message.tx_efid = 0x00000000;
//     transmit_message.tx_ff = CAN_FF_STANDARD;
//     transmit_message.tx_ft = CAN_FT_DATA;
//     transmit_message.tx_dlen = len;

//     for (uint8_t i = 0; i < len; i++)
//     {
//         transmit_message.tx_data[i] = data[i];
//     }

//     // // 发送消息
//     uint8_t mailbox = can_message_transmit(CANX, &transmit_message);

//     // 可选：检查发送状态
    
//     uint32_t timeout = 0xFFFF;
//     while((can_transmit_states(CANX, mailbox) != CAN_TRANSMIT_OK) && (timeout != 0)) {
//         timeout--;
//     }
//     if(timeout == 0) {
//         printf( "CAN transmit timeout!\n");
//     }
    
// }

void can_transmit_data(uint8_t *data, uint8_t len)
{
    if (data == NULL || len > 8) {
        printf("Invalid data or length\n");
        return;
    }

    // 检查CAN控制器状态
    if (can_flag_get(CANX, CAN_FLAG_BOERR) == SET) {
        printf("CAN Bus Off Error!\n");
        can_flag_clear(CANX, CAN_FLAG_BOERR);  // 尝试清除
        return;
    }
    
    if (can_flag_get(CANX, CAN_FLAG_PERR) == SET) {
        printf("CAN Passive Error!\n");
        return;
    }

    // 检查邮箱状态
    printf("Before transmit - Mailbox states:\n");
    printf("Mailbox 0: %d\n", can_transmit_states(CANX, 0));
    printf("Mailbox 1: %d\n", can_transmit_states(CANX, 1));
    printf("Mailbox 2: %d\n", can_transmit_states(CANX, 2));

    // 填充发送消息
    transmit_message.tx_sfid = 0x123;
    transmit_message.tx_efid = 0x00000000;
    transmit_message.tx_ff = CAN_FF_STANDARD;
    transmit_message.tx_ft = CAN_FT_DATA;
    transmit_message.tx_dlen = len;

    for (uint8_t i = 0; i < len; i++) {
        transmit_message.tx_data[i] = data[i];
    }

    // 尝试发送
    uint8_t mailbox = can_message_transmit(CANX, &transmit_message);
    
    printf("can_message_transmit returned mailbox: %d\n", mailbox);
    
    if (mailbox == CAN_NOMAILBOX) {
        printf("No mailbox available!\n");
        
        // 检查CAN控制器是否使能
        if (can_flag_get(CANX, CAN_FLAG_IWS) != SET) {
            printf("CAN controller not initialized properly!\n");
        }
        
        // 强制检查邮箱状态
        FlagStatus tx_empty0 = can_flag_get(CANX, CAN_FLAG_TME0);
        FlagStatus tx_empty1 = can_flag_get(CANX, CAN_FLAG_TME1);
        FlagStatus tx_empty2 = can_flag_get(CANX, CAN_FLAG_TME2);
        
        printf("TX Mailbox Empty Flags - 0:%d, 1:%d, 2:%d\r\n", 
               tx_empty0, tx_empty1, tx_empty2);
        
        return;
    }

    printf("Using mailbox %d for transmission\r\n", mailbox);
    
    // 等待发送完成
    uint32_t timeout = 0xFFFF;
    can_transmit_state_enum state;
    
    do {
        state = can_transmit_states(CANX, mailbox);
        timeout--;
    } while ((state == CAN_TRANSMIT_PENDING) && (timeout != 0));
    
    if (state == CAN_TRANSMIT_OK) {
        printf("CAN transmit success!\n");
    } else if (state == CAN_TRANSMIT_FAILED) {
        printf("CAN transmit failed! State: %d\n", state);
    } else {
        printf("CAN transmit timeout! Final state: %d\n", state);
    }
}

/**
 * @brief 通用CAN接收FIFO1中断处理函数
 *
 * 该函数用于处理CAN接收FIFO1非空中断，设置接收标志位，并可扩展为错误处理等功能。
 * 增加了基本的安全性和健壮性检查。
 */
static void CANx_RX_IRQHandler(void)
{
    // 检查接收FIFO1非空中断标志
    if (can_interrupt_flag_get(CANX, CAN_INT_FLAG_RFL1) == SET) {
        // 检查FIFO溢出错误
        if (can_interrupt_flag_get(CANX, CAN_INT_FLAG_RFO1) == SET) {
            can_interrupt_flag_clear(CANX, CAN_INT_FLAG_RFO1);
            printf("CAN FIFO1 overflow!\r\n");  // 可选：添加错误提示
        }

        // 检查错误警告标志
        if (can_flag_get(CANX, CAN_FLAG_WERR) == SET) {
            printf("CAN warning error!\r\n");   // 可选：添加错误提示
        }

        can_interrupt_flag_clear(CANX, CAN_INT_FLAG_RFL1);
        receive_flag = SET;
    }
    
    // 可选：添加其他中断标志检查
    if (can_interrupt_flag_get(CANX, CAN_INT_FLAG_BOERR) == SET) {
        can_interrupt_flag_clear(CANX, CAN_INT_FLAG_BOERR);
        printf("CAN bus off!\r\n");
    }
}


/**
 * @brief CAN配置函数
 *
 * 该函数用于初始化CAN模块，包括GPIO引脚配置、CAN网络配置和中断向量配置。
 */
// 在can_config函数中添加：
void can_config(void)
{
    can_gpio_init();
    can_networking_init();
    nvic_config();
    can_interrupt_enable(CANX, CAN_INT_RFNE1);
}


#ifdef DEV_CAN0_USED
void CAN0_RX1_IRQHandler(void)  // 正确的函数名
{
    CANx_RX_IRQHandler();
}
#else
void CAN1_RX1_IRQHandler(void)  // 正确的函数名
{
    CANx_RX_IRQHandler();
}
#endif

void can_transmit_text(void)
{
    // 发送测试数据
    uint8_t test_data[8] = { 0x11, 0x90, 0x77, 0x67, 0x23, 0x70, 0x55, 0x99 };
    can_transmit_data(test_data, sizeof(test_data));
}