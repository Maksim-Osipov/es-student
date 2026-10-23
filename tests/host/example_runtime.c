// Educational model: production command/task code; synthetic GPIO, clock and ID.
#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include "platform/platform.h"
#include "systime/systime.h"
#include "profiling/profiling.h"
#include "device/device.h"
#include "api/api.h"
#include "led-task/led-task.h"
#include "button-task/button-task.h"
#include "pi-task/pi-task.h"
#undef printf
#undef putchar
#undef vprintf
static uint64_t now;
static const char *input;
static char output[2048];
static size_t output_length;
uint64_t time_us_64(void) { return now+5000000; }
void platform_init(void) {}
void platform_led_set(bool on) { (void)on; }
bool platform_button_read(void) { return false; }
void device_get_info(device_info_t *info)
{
    *info=(device_info_t){.board="pico",.serial="0000000000000000",.manufacturer=0x927,.part=2,.revision=2,.sdk_version="2.3.0"};
}
int getchar_timeout_us(uint32_t timeout) { (void)timeout; return input && *input ? (unsigned char)*input++ : -1; }
int host_putchar(int ch)
{
    if (ch=='\n') {
        output[output_length]='\0';
        if (output_length) fprintf(stdout,"%.6f\t%s\n",(double)now/1000000,output);
        output_length=0;
    } else if (output_length+1<sizeof(output)) output[output_length++]=(char)ch;
    return ch;
}
int host_vprintf(const char *fmt, va_list args)
{
    char text[2048];
    int n=vsnprintf(text,sizeof(text),fmt,args);
    for (size_t i=0; text[i]; ++i) host_putchar(text[i]);
    return n;
}
int host_printf(const char *fmt, ...)
{
    va_list args; va_start(args,fmt); int n=host_vprintf(fmt,args); va_end(args); return n;
}
static void tick(void)
{
    profiling_start(0);
    profiling_start(1);
    const command_t *command=stdio_text_protocol_handle();
    if (command) api_handle(command);
    now+=2; profiling_stop(1);
    profiling_start(2); led_task_handle(); now+=1; profiling_stop(2);
    profiling_start(3); button_task_handle(); now+=1; profiling_stop(3);
    profiling_start(4);
    if (pi_task_get_result().state==PI_RUNNING) now+=300;
    pi_task_handle(); now+=1; profiling_stop(4);
    profiling_stop(0);
}
int main(void)
{
    const char *names[]={"loop","command","led","button","pi"};
    for (unsigned i=0;i<5;++i) profiling_stopwatch_init(i,names[i]);
    stdio_text_protocol_init(); led_task_init(); button_task_init(led_task_next_state);
    char record[2048], bytes[512];
    while (fgets(record,sizeof(record),stdin)) {
        char *hex;
        uint64_t target=strtoull(record,&hex,10);
        while (now<target) tick();
        size_t count=0;
        while (*hex==' ') ++hex;
        while (hex[0] && hex[0]!='\n' && hex[1] && count+1<sizeof(bytes)) {
            char pair[3]={hex[0],hex[1],0};
            bytes[count++]=(char)strtoul(pair,NULL,16); hex+=2;
        }
        bytes[count]=0; input=bytes;
        while (*input) tick();
    }
    return 0;
}
