#pragma once
#include <stdio.h>
#include <stdarg.h>
int host_printf(const char *format, ...);
int host_vprintf(const char *format, va_list args);
int host_putchar(int ch);
#define printf host_printf
#define vprintf host_vprintf
#define putchar host_putchar
