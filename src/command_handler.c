#include <stdio.h>

#include "sylvan/inferior.h"
#include "command_handler.h"
#include "handle_command.h"

/**
 * @brief Prints available commands or info subcommands
 * @param tp Type of commands to print (standard or info)
 */
static void print_commands(enum sylvan_command_type tp)
{
    size_t i = 0;
    if (tp == SYLVAN_STANDARD_COMMAND)
    {
        printf("Commands:\n");
        while (sylvan_commands[i].name && sylvan_commands[i].desc)
        {
            printf("    %s  - %s\n", sylvan_commands[i].name, sylvan_commands[i].desc);
            i++;
        }
    }
    else if (tp == SYLVAN_INFO_COMMAND)
    {
        printf("Info Commands:\n");
        while (sylvan_info_commands[i].name && sylvan_info_commands[i].desc)
        {
            printf("    info %s  - %s\n", sylvan_info_commands[i].name, sylvan_info_commands[i].desc);
            i++;
        }
    }
}


/** @brief Handler for 'help' command */
int handle_help(char **command, struct sylvan_inferior *inf)
{
    print_commands(SYLVAN_STANDARD_COMMAND);
    return 0;
}

/** @brief Handler for 'quit' command */
int handle_exit(char **command, struct sylvan_inferior *inf)
{
    printf("Exiting Debugger\n");
    return 1;
}

/** @brief Handler for 'continue' command */
int handle_continue(char **command, struct sylvan_inferior *inf)
{
    if (inf->status != SYLVAN_INFSTATE_STOPPED)
    {
        printf("Process is not stopped\n");
        return 0;
    }
    if (sylvan_continue(inf) < 0)
    {
        fprintf(stderr, "Failed to continue process\n");
        return 1;
    }
    return 0;
}

/** @brief Handler for 'info' command */
int handle_info(char **command, struct sylvan_inferior *inf)
{
    if (command[1] == NULL)
    {
        print_commands(SYLVAN_INFO_COMMAND);
        return 0;
    }
    return handle_command(command, inf); // Re-run with full command for subcommand
}