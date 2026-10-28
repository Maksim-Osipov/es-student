#pragma once
#include <stdbool.h>
#include <stdint.h>
void adc_init(void);
void adc_gpio_init(unsigned pin);
void adc_set_temp_sensor_enabled(bool enabled);
void adc_select_input(unsigned channel);
uint16_t adc_read(void);
