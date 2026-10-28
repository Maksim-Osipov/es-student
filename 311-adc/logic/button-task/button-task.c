#include "button-task/button-task.h"
#include "platform/platform.h"
#include "systime/systime.h"
typedef enum { RELEASED, PRESS_FILTER, PRESSED, RELEASE_FILTER } button_state_t;
static const uint64_t DEBOUNCE_US=20000;
static button_state_t state;
static uint64_t changed_us;
static uint32_t presses;
static button_task_callback_t on_press;
void button_task_init(button_task_callback_t callback)
{
    state=RELEASED;
    presses=0;
    changed_us=systime_us();
    on_press=callback;
}
void button_task_handle(void)
{
    bool down=platform_button_read();
    uint64_t now=systime_us();
    switch (state) {
    case RELEASED:
        if (down) { state=PRESS_FILTER; changed_us=now; }
        break;
    case PRESS_FILTER:
        if (!down) state=RELEASED;
        else if (now-changed_us>=DEBOUNCE_US) {
            state=PRESSED;
            ++presses;
            if (on_press) on_press();
        }
        break;
    case PRESSED:
        if (!down) { state=RELEASE_FILTER; changed_us=now; }
        break;
    case RELEASE_FILTER:
        if (down) state=PRESSED;
        else if (now-changed_us>=DEBOUNCE_US) state=RELEASED;
        break;
    }
}
bool button_task_is_pressed(void) { return state==PRESSED || state==RELEASE_FILTER; }
uint32_t button_task_get_press_count(void) { return presses; }
