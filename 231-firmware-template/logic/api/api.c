#include "api/api.h"
#include "api/api-commands.h"
#include <stdio.h>
#include <string.h>
void api_handle(const command_t *command)
{
    if (!command) return;
    if (command->truncated) {
        printf("error: command longer than 63 characters or 4 arguments\n");
        return;
    }
    if (strcmp(command->name,"help")==0) {
        printf("%-16s %s\n", "help", "list of commands");
        for (size_t i=0; i<api_command_count; ++i)
            printf("%-16s %s\n", api_commands[i].name, api_commands[i].help);
        return;
    }
    for (size_t i=0; i<api_command_count; ++i) {
        if (strcmp(command->name,api_commands[i].name)==0) {
            api_commands[i].callback(command);
            return;
        }
    }
    printf("error: unknown command '%s', try help\n", command->name);
}
