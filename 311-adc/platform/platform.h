#pragma once
#include <stdbool.h>
void platform_init(void);
void platform_led_set(bool on);
bool platform_button_read(void);
// ADC input GP26 in volts; the internal sensor in degrees Celsius.
float platform_voltage_read(void);
float platform_temp_read(void);
// Drive GP22 through the resistor to charge/discharge the RC circuit.
void platform_charge_set(bool on);
