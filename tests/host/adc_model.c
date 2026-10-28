// Educational hardware model. This file never runs on the Pico.
#include "adc_model.h"
#include "pico/stdlib.h"
#include "hardware/adc.h"
#include <math.h>

uint64_t adc_model_now;
bool adc_model_rc, adc_model_fixed;
uint16_t adc_model_voltage_raw, adc_model_temp_raw = 876;
unsigned adc_model_channel, adc_model_pin;
bool adc_model_enabled, adc_model_temp_enabled;
bool adc_model_outputs[30], adc_model_pins[30];
static double rc_voltage;
static uint64_t rc_updated;

uint64_t time_us_64(void) { return adc_model_now; }
void stdio_init_all(void) {}
void gpio_init(uint pin) { adc_model_pins[pin] = false; }
void gpio_set_dir(uint pin, bool output) { adc_model_outputs[pin] = output; }
bool gpio_get(uint pin) { return adc_model_pins[pin]; }
void gpio_pull_up(uint pin) { adc_model_pins[pin] = true; }

static double capacitor_voltage(void)
{
    // 10 kohm * 100 uF = 1 s, ideal RC with 12-bit ADC quantization.
    double target = adc_model_pins[22] ? 3.3 : 0.0;
    return target + (rc_voltage - target) * exp(-(double)(adc_model_now - rc_updated) / 1e6);
}
void gpio_put(uint pin, bool on)
{
    if (pin == 22) {
        rc_voltage = capacitor_voltage();
        rc_updated = adc_model_now;
    }
    adc_model_pins[pin] = on;
}
void adc_init(void) { adc_model_enabled = true; }
void adc_gpio_init(unsigned pin) { adc_model_pin = pin; }
void adc_set_temp_sensor_enabled(bool on) { adc_model_temp_enabled = on; }
void adc_select_input(unsigned channel) { adc_model_channel = channel; }
uint16_t adc_read(void)
{
    if (adc_model_channel == 4)
        return adc_model_fixed ? adc_model_temp_raw : (uint16_t)(875 + (adc_model_now / 100000) % 2);
    if (adc_model_fixed) return adc_model_voltage_raw;
    double v = adc_model_rc ? capacitor_voltage() : 0.6 + 0.02 * sin((double)adc_model_now / 1e6);
    long raw = lround(v * 4096 / 3.3);
    return (uint16_t)(raw < 0 ? 0 : raw > 4095 ? 4095 : raw);
}
