// Actual 311-adc C code with synthetic clocks, GPIO, ADC inputs and board identity.
#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include "adc_model.h"
#include "platform/platform.h"
#include "profiling/profiling.h"
#include "device/device.h"
#include "api/api.h"
#include "led-task/led-task.h"
#include "button-task/button-task.h"
#include "pi-task/pi-task.h"
#include "telemetry-task/telemetry-task.h"
#undef printf
#undef putchar
#undef vprintf

static const char *input;
static char output[2048];
static size_t output_length;
void device_get_info(device_info_t *info)
{
    *info = (device_info_t){.board="pico", .serial="0000000000000000",
        .manufacturer=0x927, .part=2, .revision=2, .sdk_version="2.3.0"};
}
int getchar_timeout_us(uint32_t timeout)
{
    (void)timeout;
    return input && *input ? (unsigned char)*input++ : -1;
}
int host_putchar(int ch)
{
    if (ch == '\n') {
        output[output_length] = '\0';
        if (output_length) fprintf(stdout, "%.6f\t%s\n", (double)adc_model_now / 1e6, output);
        output_length = 0;
    } else if (output_length + 1 < sizeof(output)) output[output_length++] = (char)ch;
    return ch;
}
int host_vprintf(const char *format, va_list args)
{
    char value[2048];
    int n = vsnprintf(value, sizeof(value), format, args);
    for (size_t i = 0; value[i]; ++i) host_putchar(value[i]);
    return n;
}
int host_printf(const char *format, ...)
{
    va_list args;
    va_start(args, format);
    int n = host_vprintf(format, args);
    va_end(args);
    return n;
}
static void tick(void)
{
    profiling_start(0);
    profiling_start(1);
    const command_t *command = stdio_text_protocol_handle();
    if (command) api_handle(command);
    profiling_stop(1);
    profiling_start(2); led_task_handle(); profiling_stop(2);
    profiling_start(3); button_task_handle(); profiling_stop(3);
    profiling_start(4); pi_task_handle(); profiling_stop(4);
    profiling_start(5); telemetry_task_handle(); profiling_stop(5);
    profiling_stop(0);
    adc_model_now += 10;
}
int main(int argc, char **argv)
{
    adc_model_rc = argc > 1 && strcmp(argv[1], "--rc") == 0;
    platform_init();
    stdio_text_protocol_init();
    led_task_init();
    button_task_init(led_task_next_state);
    telemetry_task_init(api_print_telemetry);
    const char *names[] = {"loop", "command", "led", "button", "pi", "telemetry"};
    for (unsigned i = 0; i < 6; ++i) profiling_stopwatch_init(i, names[i]);
    char record[2048], bytes[512];
    while (fgets(record, sizeof(record), stdin)) {
        char *hex;
        uint64_t target = strtoull(record, &hex, 10);
        while (adc_model_now < target) tick();
        size_t count = 0;
        while (*hex == ' ') ++hex;
        while (hex[0] && hex[0] != '\n' && hex[1] && count + 1 < sizeof(bytes)) {
            char pair[3] = {hex[0], hex[1], 0};
            bytes[count++] = (char)strtoul(pair, NULL, 16);
            hex += 2;
        }
        bytes[count] = 0;
        input = bytes;
        while (*input) tick();
    }
    return 0;
}
