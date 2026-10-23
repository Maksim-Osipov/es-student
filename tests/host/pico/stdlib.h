#pragma once
#include <stdint.h>
#define PICO_ERROR_TIMEOUT (-1)
int getchar_timeout_us(uint32_t timeout);
