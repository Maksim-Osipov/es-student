#include "telemetry-task/telemetry-task.h"
#include "platform/platform.h"
#include "systime/systime.h"

static telemetry_state_t state;
static telemetry_task_callback_t callback;
static uint32_t period_ms;
static uint64_t last_us;
static bool first_record;

void telemetry_task_init(telemetry_task_callback_t on_record)
{
    callback = on_record;
    state = TELEMETRY_STATE_IDLE;
    period_ms = 100;
    first_record = false;
}

bool telemetry_task_start(uint32_t value)
{
    if (value < 10) return false;
    period_ms = value;
    last_us = systime_us();
    first_record = true;
    state = TELEMETRY_STATE_RUN;
    return true;
}

void telemetry_task_stop(void) { state = TELEMETRY_STATE_IDLE; }

void telemetry_task_handle(void)
{
    switch (state) {
    case TELEMETRY_STATE_RUN:
        if (first_record || systime_period_elapsed(&last_us, (uint64_t)period_ms * 1000)) {
            first_record = false;
            telemetry_record_t record;
            record.time_us = systime_us();
            record.voltage_v = platform_voltage_read();
            record.temp_c = platform_temp_read();
            if (callback) callback(&record);
        }
        break;
    case TELEMETRY_STATE_IDLE:
        break;
    }
}

telemetry_state_t telemetry_task_get_state(void) { return state; }
uint32_t telemetry_task_get_period_ms(void) { return period_ms; }
