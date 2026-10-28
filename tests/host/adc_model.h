#pragma once
#include <stdint.h>
#include <stdbool.h>
extern uint64_t adc_model_now;
extern bool adc_model_rc;
extern bool adc_model_fixed;
extern uint16_t adc_model_voltage_raw, adc_model_temp_raw;
extern unsigned adc_model_channel, adc_model_pin;
extern bool adc_model_enabled, adc_model_temp_enabled;
extern bool adc_model_outputs[30], adc_model_pins[30];
