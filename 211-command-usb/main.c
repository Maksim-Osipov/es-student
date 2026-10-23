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

#include "hardware/gpio.h"
static const uint BUTTON_PIN = 15;
static void cmd_enable(void) { led_set(true); LOG_INF("led on\n"); }
static void cmd_disable(void) { led_set(false); LOG_INF("led off\n"); }
static void cmd_device_info(void) { device_info(); }
static void cmd_log_version(void) { log_version(); }
static void cmd_mem_info(void) { mem_info(); }
static void cmd_fw_info(void) { fw_info(); }
static void cmd_dev_info(void) { dev_info(); }
static void cmd_boot_info(void) { boot_info(); }
const struct command_t commands[] = {
    {"enable", cmd_enable}, {"disable", cmd_disable},

    {"info", cmd_device_info}, {"version", cmd_log_version}, {"ping", cmd_ping},
    {"mem_info", cmd_mem_info}, {"fw_info", cmd_fw_info},
    {"dev_info", cmd_dev_info}, {"boot_info", cmd_boot_info},
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
    gpio_init(BUTTON_PIN);
    gpio_set_dir(BUTTON_PIN, GPIO_IN);
    gpio_pull_up(BUTTON_PIN);
    bool stable = gpio_get(BUTTON_PIN), candidate = stable;
    uint64_t changed = time_us_64();
    while (true) {
        bool raw = gpio_get(BUTTON_PIN);
        uint64_t now = time_us_64();
        if (raw != candidate) { candidate = raw; changed = now; }
        if (candidate != stable && now - changed >= 20000) {
            stable = candidate;
            if (!stable) {
                led_toggle();
                LOG_INF("led %s\n", led_is_on() ? "on" : "off");
            }
        }
        read_line();
    }
}
