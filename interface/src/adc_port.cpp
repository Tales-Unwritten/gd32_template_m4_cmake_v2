// ============================================================
// @platform GD32F4xx（当前平台）
// ============================================================

#include "adc_port.hpp"

#include <new>
#include "gd32f4xx.h"
#include "gd32f4xx_rcu.h"
#include "gd32f4xx_adc.h"
#include "gd32f4xx_gpio.h"

adc_port::adc_port(const AdcPortConfig &cfg)
    : _cfg(cfg), _ch_pins{},
      _initialized(false), _periph(0)
{
    for (uint8_t i = 0; i < 8; i++) {
        if (i < cfg.channel_count && cfg.channels[i].port != 0) {
            _ch_pins[i] = new (_ch_storage[i].data)
                io_ctrl(cfg.channels[i].port, cfg.channels[i].pin);
        }
    }
}

adc_port::~adc_port() { deinit(); }

void adc_port::_enable_clock()
{
    switch (_cfg.periph) {
        case adc_id::adc0: rcu_periph_clock_enable(RCU_ADC0); break;
        case adc_id::adc1: rcu_periph_clock_enable(RCU_ADC1); break;
        case adc_id::adc2: rcu_periph_clock_enable(RCU_ADC2); break;
    }
}

void adc_port::init()
{
    if (_initialized) return;

    switch (_cfg.periph) {
        case adc_id::adc0: _periph = ADC0; break;
        case adc_id::adc1: _periph = ADC1; break;
        case adc_id::adc2: _periph = ADC2; break;
    }

    _enable_clock();

    // GPIO: 模拟模式
    for (uint8_t i = 0; i < _cfg.channel_count && i < 8; i++) {
        if (_ch_pins[i]) {
            _ch_pins[i]->init(GPIO_MODE_ANALOG, GPIO_PUPD_NONE);
        }
    }

    // ADC 独立模式
    adc_sync_mode_config(ADC_SYNC_MODE_INDEPENDENT);
    adc_special_function_config(_periph, ADC_SCAN_MODE, DISABLE);
    adc_special_function_config(_periph, ADC_CONTINUOUS_MODE, DISABLE);
    adc_data_alignment_config(_periph, _cfg.data_alignment);
    adc_resolution_config(_periph, _cfg.resolution);
    adc_external_trigger_config(_periph, ADC_ROUTINE_CHANNEL,
                                EXTERNAL_TRIGGER_DISABLE);

    adc_enable(_periph);
    _initialized = true;
}

void adc_port::deinit()
{
    if (!_initialized) return;
    adc_disable(_periph);

    for (uint8_t i = 0; i < _cfg.channel_count && i < 8; i++) {
        if (_ch_pins[i]) {
            _ch_pins[i]->deinit();
            _ch_pins[i]->~io_ctrl();
            _ch_pins[i] = nullptr;
        }
    }
    _periph = 0;
    _initialized = false;
}

void adc_port::calibrate()
{
    if (!_initialized) return;
    adc_calibration_enable(_periph);
}

void adc_port::_config_channel(uint8_t index)
{
    if (index >= _cfg.channel_count) return;

    adc_routine_channel_config(_periph, 0,
                               _cfg.channels[index].channel,
                               _cfg.channels[index].sample_time);
    adc_channel_length_config(_periph, ADC_ROUTINE_CHANNEL, 1);
}

uint16_t adc_port::_do_convert()
{
    adc_software_trigger_enable(_periph, ADC_ROUTINE_CHANNEL);

    uint32_t timeout = 0xFFFF;
    while (!adc_flag_get(_periph, ADC_FLAG_EOC)) {
        if (--timeout == 0) return 0;
    }
    adc_flag_clear(_periph, ADC_FLAG_EOC);

    return (uint16_t)adc_routine_data_read(_periph);
}

uint16_t adc_port::read_raw(uint8_t ch_index)
{
    if (!_initialized || ch_index >= _cfg.channel_count) return 0;
    _config_channel(ch_index);
    return _do_convert();
}

uint32_t adc_port::read_mv(uint8_t ch_index)
{
    uint16_t raw = read_raw(ch_index);
    uint32_t max_raw;
    switch (_cfg.resolution) {
        case ADC_RESOLUTION_12B: max_raw = 4095; break;
        case ADC_RESOLUTION_10B: max_raw = 1023; break;
        case ADC_RESOLUTION_8B:  max_raw = 255;  break;
        default:                 max_raw = 63;   break;
    }
    return (uint32_t)raw * _cfg.vref_mv / max_raw;
}

void adc_port::scan_raw(uint16_t *results)
{
    if (!_initialized || !results) return;
    for (uint8_t i = 0; i < _cfg.channel_count; i++) {
        results[i] = read_raw(i);
    }
}

void adc_port::scan_mv(uint32_t *results)
{
    if (!_initialized || !results) return;
    for (uint8_t i = 0; i < _cfg.channel_count; i++) {
        results[i] = read_mv(i);
    }
}
