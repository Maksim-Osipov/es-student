#include "api/api-commands.h"
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
    for (unsigned i=0; i<2; ++i)
        printf("button %u: %s, presses %u\n",i+1,button_task_is_pressed(i)?"pressed":"released",(unsigned)button_task_get_press_count(i));
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
const api_command_t api_commands[]={
    {"info","device passport",cmd_info}, {"uptime","time since reset",cmd_uptime},
    {"led_enable","turn LED on",cmd_led_enable}, {"led_disable","turn LED off",cmd_led_disable},
    {"led_blink","blink LED",cmd_led_blink}, {"led_period","set period in ms",cmd_led_period},
    {"button","button state and presses",cmd_button}, {"pi_start","start calculation [terms]",cmd_pi_start},
    {"pi","calculation result",cmd_pi}, {"profiling","stopwatch results",cmd_profiling},
    {"profiling_reset","reset stopwatches",cmd_profiling_reset},
};
const size_t api_command_count=sizeof(api_commands)/sizeof(api_commands[0]);
