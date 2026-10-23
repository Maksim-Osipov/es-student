#pragma once
#include <stdbool.h>
#include <stdint.h>
typedef void (*button_task_callback_t)(void);
void button_task_init(unsigned number, button_task_callback_t callback);
void button_task_handle(unsigned number);
bool button_task_is_pressed(unsigned number);
uint32_t button_task_get_press_count(unsigned number);
