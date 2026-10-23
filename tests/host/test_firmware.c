// Native tests: real firmware logic, deterministic clock and GPIO substitutes.
#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <string.h>
#include "platform/platform.h"
#include "systime/systime.h"
#include "profiling/profiling.h"
#include "stdio-text-protocol/stdio-text-protocol.h"
#include "led-task/led-task.h"
#include "button-task/button-task.h"
#include "pi-task/pi-task.h"
static uint64_t now;
static bool pin, button;
static unsigned callbacks;
static const char *input;
uint64_t time_us_64(void) { return now; }
void platform_init(void) {}
void platform_led_set(bool on) { pin=on; }
bool platform_button_read(void) { return button; }
int getchar_timeout_us(uint32_t timeout) { (void)timeout; return input && *input ? (unsigned char)*input++ : -1; }
static void pressed(void) { ++callbacks; }
static const command_t *frame(const char *text)
{
    input=text;
    const command_t *result=NULL;
    while (*input) { const command_t *next=stdio_text_protocol_handle(); if (next) result=next; }
    return result;
}
int main(void)
{
    uint64_t last=0;
    now=999; assert(!systime_period_elapsed(&last,1000));
    now=1001; assert(systime_period_elapsed(&last,1000) && last==1000);
    now=2000; assert(systime_period_elapsed(&last,1000) && last==2000);
    now=5001; assert(systime_period_elapsed(&last,1000) && last==5001);
    assert(!systime_period_elapsed(&last,0));
    now=0; led_task_init(); assert(pin && led_task_get_state()==LED_STATE_BLINK);
    now=500000; led_task_handle(); assert(!pin);
    led_task_set_state(LED_STATE_OFF); led_task_set_period_ms(UINT32_MAX);
    assert(led_task_get_period_ms()==UINT32_MAX && led_task_get_state()==LED_STATE_OFF);
    led_task_next_state(); assert(pin && led_task_get_state()==LED_STATE_ON);
    led_task_set_period_ms(1); led_task_set_state(LED_STATE_BLINK);
    now+=499; led_task_handle(); assert(pin);
    ++now; led_task_handle(); assert(!pin);
    button_task_init(pressed); button=true; button_task_handle();
    now+=10000; button=false; button_task_handle();
    now+=1000; button=true; button_task_handle();
    now+=19999; button_task_handle(); assert(callbacks==0);
    ++now; button_task_handle(); assert(callbacks==1 && button_task_is_pressed());
    now+=1000000; button_task_handle(); assert(callbacks==1);
    button=false; button_task_handle(); now+=19999; button_task_handle(); assert(button_task_is_pressed());
    ++now; button_task_handle(); assert(!button_task_is_pressed());
    button=true; button_task_handle(); now+=20000; button_task_handle(); assert(callbacks==2);
    assert(button_task_get_press_count()==2);
    assert(pi_task_get_result().state==PI_NOT_STARTED);
    assert(!pi_task_start(0)); assert(pi_task_start(1000000)); assert(!pi_task_start(1));
    unsigned calls=0;
    while (pi_task_get_result().state==PI_RUNNING) { now+=300; pi_task_handle(); ++calls; }
    pi_result_t pi=pi_task_get_result();
    assert(calls==15625 && fabs(pi.value-3.1415916535897743)<1e-12);
    assert(pi.elapsed_ms==4687);
    assert(pi_task_start(1)); pi_task_handle(); assert(pi_task_get_result().value==4.0);
    assert(pi_task_start(UINT32_MAX)); pi_task_handle(); assert(pi_task_get_result().state==PI_RUNNING);
    stdio_text_protocol_init();
    assert(!frame("   \r\n"));
    const command_t *cmd=frame("  led_period  500  \r");
    assert(cmd && !cmd->truncated && cmd->argc==1 && !strcmp(cmd->name,"led_period") && !strcmp(cmd->argv[0],"500"));
    cmd=frame("upx\177time\n"); assert(cmd && !strcmp(cmd->name,"uptime"));
    cmd=frame("led_period 1 2 3 4 5\n"); assert(cmd && cmd->truncated);
    cmd=frame("xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx\n");
    assert(cmd && cmd->truncated);
    assert(!frame("p")); cmd=frame("i\n"); assert(cmd && !strcmp(cmd->name,"pi"));
    cmd=frame("xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx\b\n");
    assert(cmd && !cmd->truncated && strlen(cmd->name)==62);
    cmd=frame("help\r\n"); assert(cmd && !strcmp(cmd->name,"help"));
    profiling_stopwatch_init(0,"test");
    profiling_start(0); now+=10; profiling_stop(0);
    profiling_start(0); now+=30; profiling_stop(0);
    profiling_result_t stats;
    assert(profiling_get(0,&stats) && stats.count==2 && stats.mean_us==20 && stats.max_us==30);
    profiling_start(0); profiling_reset(); now+=100; profiling_stop(0);
    assert(profiling_get(0,&stats) && stats.count==0 && stats.max_us==0);
    assert(!profiling_get(100,&stats));
    puts("\nPASS: timing, LED, debounce, pi, framing and profiling");
    return 0;
}
