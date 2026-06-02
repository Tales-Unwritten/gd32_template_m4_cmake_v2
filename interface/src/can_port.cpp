// ============================================================
// @platform GD32F4xx（当前平台）
//   移植到新 MCU 时，本文件需要完整重写，但公共接口签名不变。
//   需要替换的映射：
//     - 外设基址:   CAN0/CAN1 → 目标平台外设基址
//     - 时钟使能:   rcu_periph_clock_enable(RCU_CANx)
//     - 波特率:    can_frequency_set → 目标平台 CAN 时序 API
//     - 帧结构:    can_trasnmit_message_struct → 目标平台 CAN 帧
//     - GPIO 复用:  AF9 → 目标平台 CAN AF 编号
// ============================================================

#include "can_port.hpp"

#include "gd32f4xx.h"
#include "gd32f4xx_rcu.h"
#include "gd32f4xx_can.h"

// ============================================================
//  构造 / 析构
// ============================================================

can_port::can_port(const CanPortConfig &cfg)
    : _cfg(cfg)
    , _tx(cfg.tx_port, cfg.tx_pin)
    , _rx(cfg.rx_port, cfg.rx_pin)
    , _initialized(false)
    , _periph(0)
{
}

can_port::~can_port()
{
    deinit();
}

// ============================================================
//  时钟使能
// ============================================================

void can_port::_enable_clock()
{
    // [PORT] GD32 时钟映射
    if (_cfg.periph == can_id::can0) {
        rcu_periph_clock_enable(RCU_CAN0);
    } else {
        rcu_periph_clock_enable(RCU_CAN1);
    }
}

// ============================================================
//  CAN 外设时钟频率
// ============================================================

uint32_t can_port::_can_clock() const
{
    // [PORT] GD32: CAN 在 APB1 上
    // APB1 时钟由系统时钟分频得出，GD32F450 通常为 PCLK1 = 60MHz
    // 简化：假设 APB1 = SystemCoreClock / 4
    return rcu_clock_freq_get(CK_APB1);
}

// ============================================================
//  init
// ============================================================

void can_port::init()
{
    if (_initialized) return;

    // 外设基址
    if (_cfg.periph == can_id::can0) {
        _periph = CAN0;
    } else {
        _periph = CAN1;
    }

    _enable_clock();

    // GPIO AF
    _tx.init(GPIO_MODE_AF, GPIO_PUPD_PULLUP, GPIO_OTYPE_PP, GPIO_OSPEED_50MHZ);
    _tx.set_af(_cfg.af);
    _rx.init(GPIO_MODE_AF, GPIO_PUPD_PULLUP, GPIO_OTYPE_PP, GPIO_OSPEED_50MHZ);
    _rx.set_af(_cfg.af);

    // CAN 外设初始化
    can_parameter_struct can_cfg;
    can_struct_para_init(CAN_INIT_STRUCT, &can_cfg);
    can_cfg.working_mode       = _cfg.mode;
    can_cfg.resync_jump_width  = CAN_BT_SJW_1TQ;
    can_cfg.time_segment_1     = CAN_BT_BS1_8TQ;
    can_cfg.time_segment_2     = CAN_BT_BS2_7TQ;
    can_cfg.time_triggered     = DISABLE;
    can_cfg.auto_bus_off_recovery = ENABLE;
    can_cfg.auto_wake_up       = DISABLE;
    can_cfg.auto_retrans        = DISABLE;
    can_cfg.rec_fifo_overwrite = DISABLE;
    can_cfg.trans_fifo_order   = DISABLE;
    // 波特率 = CAN_CLK / prescaler / (1+TS1+TS2)
    // 例如 60MHz / 6 / (1+11+8) = 500kbps
    can_cfg.prescaler          = (_can_clock() + _cfg.baudrate * (1+8+7) / 2)
                                 / (_cfg.baudrate * (1+8+7));

    // CAN1 必须分配过滤器 bank（必须在 can_init 之前）
    if (_cfg.periph == can_id::can1) {
        can1_filter_start_bank(_cfg.filter_bank);
    }

    can_init(_periph, &can_cfg);

    // 配置过滤器：接收所有帧 → FIFO0
    can_filter_parameter_struct filter_cfg;
    can_struct_para_init(CAN_FILTER_STRUCT, &filter_cfg);
    filter_cfg.filter_number      = _cfg.filter_bank;
    filter_cfg.filter_mode        = CAN_FILTERMODE_MASK;
    filter_cfg.filter_bits        = CAN_FILTERBITS_32BIT;
    filter_cfg.filter_list_high   = 0x0000;
    filter_cfg.filter_list_low    = 0x0000;
    filter_cfg.filter_mask_high   = 0x0000;
    filter_cfg.filter_mask_low    = 0x0000;
    filter_cfg.filter_fifo_number = CAN_FIFO0;
    filter_cfg.filter_enable      = ENABLE;
    can_filter_init(&filter_cfg);

    _initialized = true;
}

// ============================================================
//  deinit
// ============================================================

void can_port::deinit()
{
    if (!_initialized) return;

    can_deinit(_periph);

    _tx.deinit();
    _rx.deinit();

    _periph = 0;
    _initialized = false;
}

// ============================================================
//  发送
// ============================================================

bool can_port::send(uint32_t id, bool is_ext, const uint8_t *data, uint8_t len)
{
    if (!_initialized || !data || len > 8) return false;

    can_transmit_message_struct tx_msg;
    can_struct_para_init(CAN_TX_MESSAGE_STRUCT, &tx_msg);

    if (is_ext) {
        tx_msg.tx_efid = id;
        tx_msg.tx_ff   = CAN_FF_EXTENDED;
    } else {
        tx_msg.tx_sfid = id;
        tx_msg.tx_ff   = CAN_FF_STANDARD;
    }
    tx_msg.tx_ft = CAN_FT_DATA;
    tx_msg.tx_dlen = len;
    for (uint8_t i = 0; i < len && i < 8; i++) {
        tx_msg.tx_data[i] = data[i];
    }

    // 等待空闲邮箱（超时保护）
    uint32_t timeout = 0xFFFF;
    while (can_message_transmit(_periph, &tx_msg) == CAN_TRANSMIT_NOMAILBOX) {
        if (--timeout == 0) return false;
    }
    // 等待发送完成（MTF0/1/2）
    timeout = 0xFFFF;
    while (!(CAN_TSTAT(_periph) & (CAN_TSTAT_MTF0 | CAN_TSTAT_MTF1 | CAN_TSTAT_MTF2))) {
        if (--timeout == 0) return false;
    }
    return true;
}

// ============================================================
//  接收
// ============================================================

bool can_port::rx_ready() const
{
    if (!_initialized) return false;
    return (can_receive_message_length_get(_periph, CAN_FIFO0) > 0);
}

bool can_port::recv(uint32_t &id, bool &is_ext, uint8_t *data, uint8_t &len)
{
    if (!_initialized || !data) return false;

    if (can_receive_message_length_get(_periph, CAN_FIFO0) == 0) {
        return false;
    }

    can_receive_message_struct rx_msg;
    can_struct_para_init(CAN_RX_MESSAGE_STRUCT, &rx_msg);
    can_message_receive(_periph, CAN_FIFO0, &rx_msg);

    is_ext = (rx_msg.rx_ff == CAN_FF_EXTENDED);
    id     = is_ext ? rx_msg.rx_efid : rx_msg.rx_sfid;
    len    = rx_msg.rx_dlen;
    for (uint8_t i = 0; i < len && i < 8; i++) {
        data[i] = rx_msg.rx_data[i];
    }

    return true;
}
