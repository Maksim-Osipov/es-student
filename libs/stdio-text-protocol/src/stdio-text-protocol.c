#include "stdio-text-protocol/stdio-text-protocol.h"
#include <stdio.h>
#include <string.h>
#include "pico/stdlib.h"
static char line[64];
static size_t length;
static bool overflow;
static command_t command;
void stdio_text_protocol_init(void) { length=0; overflow=false; }
const command_t *stdio_text_protocol_handle(void)
{
    int ch=getchar_timeout_us(0);
    if (ch==PICO_ERROR_TIMEOUT) return NULL;
    if (ch=='\r' || ch=='\n') {
        if (!length) return NULL;
        putchar('\n');
        line[length]='\0';
        command=(command_t){.truncated=overflow};
        length=0;
        overflow=false;
        char *word=strtok(line, " ");
        if (!word && !command.truncated) return NULL;
        command.name=word ? word : "";
        while ((word=strtok(NULL, " ")) != NULL) {
            if (command.argc==4) { command.truncated=true; break; }
            command.argv[command.argc++]=word;
        }
        return &command;
    }
    if (ch==0x08 || ch==0x7f) {
        if (length) { --length; overflow=false; printf("\b \b"); }
    } else if (ch>=0x20 && ch<=0x7e) {
        if (length<sizeof(line)-1) { line[length++]=(char)ch; putchar(ch); }
        else overflow=true;
    }
    return NULL;
}
