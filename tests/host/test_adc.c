#include <assert.h>
#include <math.h>
#include <stdio.h>
#include "adc_model.h"
#include "platform/platform.h"
#include "telemetry-task/telemetry-task.h"

static unsigned records;
static telemetry_record_t last;
static void capture(const telemetry_record_t *record) { ++records; last = *record; }
static void at(uint64_t now) { adc_model_now = now; telemetry_task_handle(); }

int main(void)
{
    adc_model_fixed = true;
    platform_init();
    assert(adc_model_enabled && adc_model_temp_enabled && adc_model_pin == 26);
    assert(adc_model_outputs[22] && !adc_model_pins[22]);
    adc_model_voltage_raw = 0;
    assert(platform_voltage_read() == 0.0f && adc_model_channel == 0);
    adc_model_voltage_raw = 4095;
    assert(fabsf(platform_voltage_read() - 3.2991943f) < 0.00001f);
    adc_model_temp_raw = 876;
    assert(fabsf(platform_temp_read() - 27.1385f) < 0.01f && adc_model_channel == 4);
    adc_model_voltage_raw = 2048;
    assert(fabsf(platform_voltage_read() - 1.65f) < 0.00001f && adc_model_channel == 0);
    platform_charge_set(true);
    assert(adc_model_pins[22]);
    platform_charge_set(false);
    assert(!adc_model_pins[22]);

    telemetry_task_init(capture);
    at(0);
    assert(records == 0 && telemetry_task_get_state() == TELEMETRY_STATE_IDLE);
    assert(!telemetry_task_start(9));
    assert(telemetry_task_start(50));
    at(0); // First record must also work before one period of uptime.
    assert(records == 1 && last.time_us == 0);
    assert(fabsf(last.voltage_v - 1.65f) < 0.00001f && fabsf(last.temp_c - 27.1385f) < 0.01f);
    at(49999); assert(records == 1);
    at(50000); assert(records == 2 && last.time_us == 50000);
    at(100007); assert(records == 3);
    at(149999); assert(records == 3);
    at(150000); assert(records == 4); // A late call did not shift the grid.
    assert(!telemetry_task_start(0));
    assert(telemetry_task_get_period_ms() == 50 && telemetry_task_get_state() == TELEMETRY_STATE_RUN);
    at(200000); assert(records == 5);
    telemetry_task_stop();
    at(1000000); assert(records == 5);
    assert(telemetry_task_start(100));
    at(1000000); assert(records == 6);
    at(1100000); assert(records == 7);
    assert(telemetry_task_start(UINT32_MAX));
    at(1100000); assert(records == 8);
    at(1100000 + (uint64_t)UINT32_MAX * 1000 - 1); assert(records == 8);
    at(1100000 + (uint64_t)UINT32_MAX * 1000); assert(records == 9);
    telemetry_task_init(NULL);
    assert(telemetry_task_start(10));
    at(adc_model_now); // Optional callback must be safe.
    puts("PASS: ADC initialization, scaling, channel switching, RC GPIO, telemetry timing and state");
    return 0;
}
