#pragma once
#include <stdint.h>
#include <stdbool.h>
typedef unsigned int uint;
#define GPIO_IN false
#define GPIO_OUT true
#define PICO_ERROR_TIMEOUT (-1)
int getchar_timeout_us(uint32_t timeout);
void stdio_init_all(void);
void gpio_init(uint pin);
void gpio_set_dir(uint pin, bool output);
void gpio_put(uint pin, bool on);
bool gpio_get(uint pin);
void gpio_pull_up(uint pin);
