#include <stdio.h>
#include <string.h>
#include "pico/stdlib.h"
#include "command.h"
#include "led.h"
#include "log.h"
#include "device.h"
#include "memory.h"

#define LINE_SIZE 32
static char line[LINE_SIZE];
static uint line_length;
static bool line_overflow;
static void cmd_ping(void) { printf("pong\n"); }

#include "clock.h"
#include "profiling.h"
// Estimate: 175+110+190+110=585 cycles per term; 1e6*585/125e6 = 4.68 s.
static const uint CALC_PI_TERMS = 1000000;
static const uint64_t BLINK_HALF_PERIOD_US = 500000;
static volatile double pi_result;
static double calc_pi(uint terms)
{
    double sum = 0.0, sign = 1.0;
    for (uint k=0; k<terms; ++k) {
        sum += sign / (2.0 * k + 1.0);
        sign = -sign;
    }
    return 4.0 * sum;
}
static void cmd_calc_pi(void)
{
    uint64_t start = time_us_64();
    pi_result = calc_pi(CALC_PI_TERMS);
    uint64_t elapsed = time_us_64() - start;
    printf("pi: %.8f\ntime: %llu ms\n", pi_result, (unsigned long long)(elapsed/1000));
}
static void cmd_main_time_exec(void)
{
    printf("iteration avg %.2f us, max %u us\n", (double)profiling_avg_us(), (unsigned)profiling_max_us());
}
static void cmd_main_time_reset(void) { profiling_reset_max(); printf("max reset\n"); }
static void cmd_device_info(void) { device_info(); }
static void cmd_log_version(void) { log_version(); }
static void cmd_mem_info(void) { mem_info(); }
static void cmd_fw_info(void) { fw_info(); }
static void cmd_dev_info(void) { dev_info(); }
static void cmd_boot_info(void) { boot_info(); }
const struct command_t commands[] = {

    {"info", cmd_device_info}, {"version", cmd_log_version}, {"ping", cmd_ping},
    {"mem_info", cmd_mem_info}, {"fw_info", cmd_fw_info},
    {"dev_info", cmd_dev_info}, {"boot_info", cmd_boot_info},

    {"clk_info", clk_info}, {"uptime", uptime}, {"calc_pi", cmd_calc_pi},
    {"main_time_exec", cmd_main_time_exec}, {"main_time_reset", cmd_main_time_reset},
    {"clk_sys_low", clk_sys_low}, {"clk_sys_default", clk_sys_default},
};

const uint command_count = sizeof(commands) / sizeof(commands[0]);

static void handle_command(const char *command)
{
    for (uint i = 0; i < command_count; ++i) {
        if (strcmp(command, commands[i].name) == 0) {
            if (commands[i].handler != NULL) commands[i].handler();
            return;
        }
    }
    LOG_ERR("unknown command: %s\n", command);
}

static void read_line(void)
{
    int symbol = getchar_timeout_us(0);
    if (symbol == PICO_ERROR_TIMEOUT) return;
    if (symbol == '\r' || symbol == '\n') {
        if (line_length || line_overflow) {
            putchar('\n');
            line[line_length] = '\0';
            if (line_overflow) LOG_ERR("command too long\n");
            else handle_command(line);
        }
        line_length = 0;
        line_overflow = false;
    } else if (symbol >= 32 && symbol <= 126) {
        if (!line_overflow && line_length + 1 < LINE_SIZE) {
            line[line_length++] = (char)symbol;
            putchar(symbol);
        } else line_overflow = true;
    }
}

int main(void)
{
    stdio_init_all();
    led_init();
    uint64_t last_us = time_us_64();
    profiling_init();
    while (true) {
        profiling_iteration();
        uint64_t now_us = time_us_64();
        if (now_us - last_us >= BLINK_HALF_PERIOD_US) {
            last_us = now_us;
            led_toggle();
        }
        read_line();
    }
}
