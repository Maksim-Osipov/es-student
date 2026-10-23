#include "clock.h"
#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/clocks.h"
#include "hardware/regs/clocks.h"
#include "log.h"
static const uint32_t CLK_SYS_LOW_KHZ = 62500;
static void clock_row(const char *name, clock_handle_t clock, uint source)
{
    printf("%-10s %8u %8u kHz\n", name, (unsigned)(clock_get_hz(clock)/1000), (unsigned)frequency_count_khz(source));
}
void clk_info(void)
{
    printf("clock      configured measured\n");
    clock_row("clk_ref", clk_ref, CLOCKS_FC0_SRC_VALUE_CLK_REF);
    clock_row("clk_sys", clk_sys, CLOCKS_FC0_SRC_VALUE_CLK_SYS);
    clock_row("clk_peri", clk_peri, CLOCKS_FC0_SRC_VALUE_CLK_PERI);
    clock_row("clk_usb", clk_usb, CLOCKS_FC0_SRC_VALUE_CLK_USB);
    clock_row("clk_adc", clk_adc, CLOCKS_FC0_SRC_VALUE_CLK_ADC);
    printf("rosc %u\n", (unsigned)frequency_count_khz(CLOCKS_FC0_SRC_VALUE_ROSC_CLKSRC));
}
void uptime(void) { printf("uptime: %llu ms\n", (unsigned long long)(time_us_64()/1000)); }
// set_sys_clock_khz calls set_sys_clock_pll: by default
// PICO_CLOCK_ADJUST_PERI_CLOCK_WITH_SYS_CLOCK=0 selects the 48 MHz USB PLL
// for clk_peri, even when clk_sys is restored to SYS_CLK_KHZ.
static void clk_sys_set(uint32_t khz)
{
    if (set_sys_clock_khz(khz, false)) LOG_INF("clk_sys %u kHz\n", (unsigned)khz);
    else LOG_ERR("clk_sys %u kHz is not set\n", (unsigned)khz);
}
void clk_sys_low(void) { clk_sys_set(CLK_SYS_LOW_KHZ); }
void clk_sys_default(void) { clk_sys_set(SYS_CLK_KHZ); }
