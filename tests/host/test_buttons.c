#include <assert.h>
#include <stdint.h>
#include <stddef.h>
#include <stdio.h>
#include "platform/platform.h"
#include "button-task/button-task.h"
#include "led-task/led-task.h"
static uint64_t now;
static bool down[2];
static unsigned short_calls, long_calls, second_calls;
uint64_t time_us_64(void) { return now; }
void platform_init(void) {}
void platform_led_set(bool on) { (void)on; }
bool platform_button_read(unsigned number) { return number<2 && down[number]; }
static void poll(void) { button_task_handle(0); button_task_handle(1); }
static void advance(uint64_t us) { now+=us; poll(); }
static void short_press(void) { ++short_calls; led_task_next_state(); }
static void long_press(void) { ++long_calls; led_task_blink(); }
static void second_press(void) { ++second_calls; led_task_next_period(); }
static void reset(void)
{
    now=0;
    down[0]=false;
    down[1]=false;
    short_calls=long_calls=second_calls=0;
    led_task_init();
    button_task_init(0, short_press, long_press);
    button_task_init(1, second_press, NULL);
}
static void test_short_press_and_bounce(void)
{
    reset();
    // A contact pulse shorter than the debounce window is not a press.
    down[0]=true; poll(); advance(19999);
    down[0]=false; poll(); advance(20000);
    assert(button_task_get_press_count(0)==0 && short_calls==0 && long_calls==0);

    down[0]=true; poll(); advance(20000);
    assert(button_task_is_pressed(0) && button_task_get_press_count(0)==1);
    assert(short_calls==0 && led_task_get_state()==LED_STATE_BLINK);
    advance(50000);
    down[0]=false; poll(); advance(10000);
    down[0]=true; poll(); // Release bounce does not complete a short press.
    assert(button_task_is_pressed(0) && short_calls==0);
    down[0]=false; poll(); advance(19999);
    assert(short_calls==0 && button_task_is_pressed(0));
    advance(1);
    assert(short_calls==1 && long_calls==0 && !button_task_is_pressed(0));
    assert(led_task_get_state()==LED_STATE_OFF && button_task_get_press_count(0)==1);
    advance(2000000);
    assert(short_calls==1 && long_calls==0);
}
static void test_long_press_and_independence(void)
{
    reset();
    led_task_set_state(LED_STATE_OFF);
    down[0]=true; poll(); now+=20000; poll();
    uint64_t held_since=now;
    assert(short_calls==0 && long_calls==0 && led_task_get_state()==LED_STATE_OFF);
    // The second button works immediately while the first one is held.
    for (unsigned i=0; i<3; ++i) {
        down[1]=true; poll(); advance(20000);
        assert(led_task_get_period_ms()==(i==0 ? 250u : i==1 ? 500u : 1000u));
        down[1]=false; poll(); advance(20000);
    }
    assert(second_calls==3 && short_calls==0 && long_calls==0);
    now=held_since+999999; poll();
    assert(long_calls==0 && led_task_get_state()==LED_STATE_OFF);
    advance(1);
    assert(long_calls==1 && short_calls==0 && led_task_get_state()==LED_STATE_BLINK);
    advance(3000000);
    assert(long_calls==1 && short_calls==0);
    // A release bounce must not report a second long press or a short press.
    down[0]=false; poll(); advance(10000);
    down[0]=true; poll(); advance(20000);
    assert(long_calls==1 && short_calls==0);
    down[0]=false; poll(); advance(20000);
    assert(long_calls==1 && short_calls==0 && !button_task_is_pressed(0));
    assert(button_task_get_press_count(0)==1 && !button_task_is_pressed(0));
}
static void test_release_before_long_threshold(void)
{
    reset();
    down[0]=true; poll(); advance(20000); advance(999999);
    down[0]=false; poll(); advance(20000);
    // Release filtering ends after the threshold, but the hold ended before it.
    assert(short_calls==1 && long_calls==0 && !button_task_is_pressed(0));
}
static void test_second_button_hold_and_rearm(void)
{
    reset();
    down[1]=true; poll(); advance(20000);
    assert(second_calls==1 && led_task_get_period_ms()==250);
    advance(2000000);
    assert(second_calls==1);
    // A complete short gesture still works while button 2 remains held.
    down[0]=true; poll(); advance(20000);
    down[0]=false; poll(); advance(20000);
    assert(short_calls==1 && long_calls==0 && second_calls==1);
    down[0]=true; poll(); advance(20000); advance(1000000);
    assert(short_calls==1 && long_calls==1 && led_task_get_state()==LED_STATE_BLINK);
    down[0]=false; poll(); advance(20000);
    down[0]=true; poll(); advance(20000);
    down[0]=false; poll(); advance(20000);
    assert(short_calls==2 && long_calls==1 && led_task_get_state()==LED_STATE_OFF);
    down[1]=false; poll(); advance(20000);
    assert(second_calls==1);
    assert(button_task_get_press_count(0)==3 && button_task_get_press_count(1)==1);
}
static void test_clock_and_optional_callbacks(void)
{
    reset();
    now=UINT64_C(5000000000);
    button_task_init(0, NULL, long_press);
    down[0]=true; poll(); advance(20000); advance(999999);
    assert(long_calls==0);
    advance(1);
    assert(long_calls==1);
    down[0]=false; poll(); advance(20000);
    button_task_init(0, NULL, NULL);
    down[0]=true; poll(); advance(20000); advance(2000000);
    down[0]=false; poll(); advance(20000);
    assert(!button_task_is_pressed(0) && button_task_get_press_count(0)==1);
    button_task_init(2, short_press, long_press);
    button_task_handle(2);
    assert(!button_task_is_pressed(2) && button_task_get_press_count(2)==0);
}
int main(void)
{
    test_short_press_and_bounce();
    test_long_press_and_independence();
    test_release_before_long_threshold();
    test_second_button_hold_and_rearm();
    test_clock_and_optional_callbacks();
    puts("PASS: short/long gestures, debounce, one-shot callbacks, independent buttons and 64-bit time");
    return 0;
}
