#include <assert.h>
#include <stdint.h>
#include "platform/platform.h"
#include "button-task/button-task.h"
#include "led-task/led-task.h"
static uint64_t now;
static bool down[2];
uint64_t time_us_64(void) { return now; }
void platform_init(void) {}
void platform_led_set(bool on) { (void)on; }
bool platform_button_read(unsigned number) { return number<2 && down[number]; }
static void poll(void) { button_task_handle(0); button_task_handle(1); }
int main(void)
{
    led_task_init();
    button_task_init(0, led_task_next_state);
    button_task_init(1, led_task_next_period);
    down[0]=true; poll(); now+=20000; poll();
    assert(led_task_get_state()==LED_STATE_OFF);
    for (unsigned i=0; i<3; ++i) {
        down[1]=true; poll(); now+=20000; poll();
        assert(led_task_get_period_ms()==(i==0 ? 250u : i==1 ? 500u : 1000u));
        down[1]=false; poll(); now+=20000; poll();
    }
    assert(button_task_get_press_count(0)==1 && button_task_is_pressed(0));
    assert(button_task_get_press_count(1)==3 && !button_task_is_pressed(1));
    assert(!button_task_is_pressed(2) && button_task_get_press_count(2)==0);
    return 0;
}
