#include "button-task/button-task.h"
#include "platform/platform.h"
#include "systime/systime.h"
typedef enum { RELEASED, PRESS_FILTER, PRESSED, RELEASE_FILTER } button_state_t;
static const uint64_t DEBOUNCE_US=20000;
typedef struct {
    button_state_t state;
    uint64_t changed_us;
    uint32_t presses;
    button_task_callback_t on_press;
} button_t;
static button_t buttons[2];
void button_task_init(unsigned number, button_task_callback_t callback)
{
    if (number>=2) return;
    button_t *button=&buttons[number];
    button->state=RELEASED;
    button->presses=0;
    button->changed_us=systime_us();
    button->on_press=callback;
}
void button_task_handle(unsigned number)
{
    if (number>=2) return;
    button_t *button=&buttons[number];
    bool down=platform_button_read(number);
    uint64_t now=systime_us();
    switch (button->state) {
    case RELEASED:
        if (down) { button->state=PRESS_FILTER; button->changed_us=now; }
        break;
    case PRESS_FILTER:
        if (!down) button->state=RELEASED;
        else if (now-button->changed_us>=DEBOUNCE_US) {
            button->state=PRESSED;
            ++button->presses;
            if (button->on_press) button->on_press();
        }
        break;
    case PRESSED:
        if (!down) { button->state=RELEASE_FILTER; button->changed_us=now; }
        break;
    case RELEASE_FILTER:
        if (down) button->state=PRESSED;
        else if (now-button->changed_us>=DEBOUNCE_US) button->state=RELEASED;
        break;
    }
}
bool button_task_is_pressed(unsigned number) { if (number>=2) return false; const button_t *button=&buttons[number]; return button->state==PRESSED || button->state==RELEASE_FILTER; }
uint32_t button_task_get_press_count(unsigned number) { if (number>=2) return 0; const button_t *button=&buttons[number]; return button->presses; }
