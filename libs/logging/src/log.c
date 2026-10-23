#include "logging/log.h"
#include <stdio.h>
#include <stdarg.h>
void log_write(const char *level, const char *function, unsigned line, const char *format, ...)
{
    printf("%s %s:%u ", level, function, line);
    va_list args;
    va_start(args, format);
    vprintf(format, args);
    va_end(args);
    putchar('\n');
}
