#include "api/api-commands.h"
#include "api/api.h"
#include "platform/platform.h"
#include "firmware.h"
#include "device/device.h"
#include "systime/systime.h"
#include "profiling/profiling.h"
#include "led-task/led-task.h"
#include "button-task/button-task.h"
#include "pi-task/pi-task.h"
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <stdint.h>
static bool parse_u32(const char *text, uint32_t *value)
{
    if (!text || !*text) return false;
    for (const char *p=text; *p; ++p) if (*p<'0' || *p>'9') return false;
    errno=0;
    char *end;
    unsigned long result=strtoul(text,&end,10);
    if (errno==ERANGE || *end || result>UINT32_MAX) return false;
    *value=(uint32_t)result;
    return true;
}
static void cmd_info(const command_t *command)
{
    (void)command;
    device_info_t info;
    device_get_info(&info);
    printf("name: %s\nversion: %s\nproject: %s\nrepo: %s\n", FIRMWARE_NAME,FIRMWARE_VERSION,FIRMWARE_PROJECT,FIRMWARE_REPO);
    printf("board: %s\nserial: %s\n",info.board,info.serial);
    printf("chip: manufacturer 0x%03x, part 0x%04x, revision %u\n",(unsigned)info.manufacturer,(unsigned)info.part,(unsigned)info.revision);
    printf("pico-sdk: %s\n",info.sdk_version);
}
static void cmd_uptime(const command_t *command)
{
    (void)command;
    printf("uptime: %llu ms\n",(unsigned long long)systime_ms());
}
static void print_led(void)
{
    const char *states[]={"off","on","blink"};
    printf("led: %s, period %u ms\n",states[led_task_get_state()],(unsigned)led_task_get_period_ms());
}
static void cmd_led_enable(const command_t *command) { (void)command; led_task_set_state(LED_STATE_ON); print_led(); }
static void cmd_led_disable(const command_t *command) { (void)command; led_task_set_state(LED_STATE_OFF); print_led(); }
static void cmd_led_blink(const command_t *command) { (void)command; led_task_set_state(LED_STATE_BLINK); print_led(); }
static void cmd_led_period(const command_t *command)
{
    uint32_t value;
    if (command->argc!=1 || !parse_u32(command->argv[0],&value)) {
        printf("error: usage led_period <period_ms>\n"); return;
    }
    if (!value) { printf("error: period_ms must be greater than 0\n"); return; }
    led_task_set_period_ms(value); print_led();
}
static void cmd_button(const command_t *command)
{
    (void)command;
    printf("button: %s, presses %u\n",button_task_is_pressed()?"pressed":"released",(unsigned)button_task_get_press_count());
}
static void cmd_pi_start(const command_t *command)
{
    uint32_t terms=1000000;
    if (command->argc>1 || (command->argc==1 && !parse_u32(command->argv[0],&terms))) {
        printf("error: usage pi_start [<terms>]\n"); return;
    }
    if (!terms) { printf("error: terms must be greater than 0\n"); return; }
    if (!pi_task_start(terms)) { printf("error: pi is busy\n"); return; }
    printf("pi: started, %u terms\n",(unsigned)terms);
}
static void cmd_pi(const command_t *command)
{
    (void)command;
    pi_result_t result=pi_task_get_result();
    switch (result.state) {
    case PI_NOT_STARTED: printf("pi: not started\n"); break;
    case PI_RUNNING: printf("pi: running\n"); break;
    case PI_READY: printf("pi: %.9f in %llu ms\n",result.value,(unsigned long long)result.elapsed_ms); break;
    }
}
static void cmd_profiling(const command_t *command)
{
    (void)command;
#if PROFILING_ENABLED
    for (unsigned i=0; i<PROFILING_STOPWATCH_COUNT; ++i) {
        profiling_result_t result;
        if (profiling_get(i,&result))
            printf("%-10s count %llu mean %.2f us max %llu us\n",result.name,
                (unsigned long long)result.count,result.mean_us,(unsigned long long)result.max_us);
    }
#else
    printf("profiling is disabled in this build\n");
#endif
}
static void cmd_profiling_reset(const command_t *command)
{
    (void)command; profiling_reset(); printf("profiling reset\n");
}
#define TELEMETRY_DEFAULT_PERIOD_MS 100
static void cmd_get_voltage(const command_t *command)
{
    (void)command;
    printf("voltage: %.3f V\n", (double)platform_voltage_read());
}
static void cmd_get_temp(const command_t *command)
{
    (void)command;
    printf("temp: %.1f C\n", (double)platform_temp_read());
}
void api_print_telemetry(const telemetry_record_t *record)
{
    printf("tm: %llu %.3f %.1f\n", (unsigned long long)record->time_us,
           (double)record->voltage_v, (double)record->temp_c);
}
static void cmd_tm_start(const command_t *command)
{
    uint32_t value = TELEMETRY_DEFAULT_PERIOD_MS;
    if (command->argc > 1 || (command->argc == 1 && !parse_u32(command->argv[0], &value))) {
        printf("error: usage tm_start [<period_ms>]\n");
        return;
    }
    if (!telemetry_task_start(value)) {
        printf("error: period_ms must be at least 10\n");
        return;
    }
    printf("telemetry: started, period %u ms\n", (unsigned)value);
}
static void cmd_tm_stop(const command_t *command)
{
    (void)command;
    telemetry_task_stop();
    printf("telemetry: stopped\n");
}
static void cmd_charge_on(const command_t *command)
{
    (void)command;
    platform_charge_set(true);
    printf("charge: on\n");
}
static void cmd_charge_off(const command_t *command)
{
    (void)command;
    platform_charge_set(false);
    printf("charge: off\n");
}
const api_command_t api_commands[]={
    {"info","device passport",cmd_info}, {"uptime","time since reset",cmd_uptime},
    {"led_enable","turn LED on",cmd_led_enable}, {"led_disable","turn LED off",cmd_led_disable},
    {"led_blink","blink LED",cmd_led_blink}, {"led_period","set period in ms",cmd_led_period},
    {"button","button state and presses",cmd_button}, {"pi_start","start calculation [terms]",cmd_pi_start},
    {"pi","calculation result",cmd_pi}, {"profiling","stopwatch results",cmd_profiling},
    {"profiling_reset","reset stopwatches",cmd_profiling_reset},
    {"get_voltage", "voltage on GP26, ADC input 0", cmd_get_voltage},
    {"get_temp", "chip temperature, degrees Celsius", cmd_get_temp},
    {"tm_start", "start telemetry [period_ms], minimum 10 ms", cmd_tm_start},
    {"tm_stop", "stop telemetry", cmd_tm_stop},
    {"charge_on", "charge RC circuit through GP22", cmd_charge_on},
    {"charge_off", "discharge RC circuit through GP22", cmd_charge_off},
};
const size_t api_command_count=sizeof(api_commands)/sizeof(api_commands[0]);
