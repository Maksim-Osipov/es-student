#pragma once
#include <stdbool.h>
#include <stdint.h>
typedef void (*button_task_callback_t)(void);
// With on_long_press, on_press is a short press reported after release.
// Without on_long_press, on_press is reported as soon as the press is debounced.
void button_task_init(unsigned number, button_task_callback_t on_press,
                      button_task_callback_t on_long_press);
void button_task_handle(unsigned number);
bool button_task_is_pressed(unsigned number);
uint32_t button_task_get_press_count(unsigned number);
