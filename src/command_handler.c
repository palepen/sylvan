#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <elf.h>


#include "sylvan/inferior.h"
#include "command_handler.h"
#include "handle_command.h"
#include "auxv.h"

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
    // supress warnings;
    (void)command;
    (void)inf;


    print_commands(SYLVAN_STANDARD_COMMAND);
    return 0;
}

/** @brief Handler for 'quit' command */
int handle_exit(char **command, struct sylvan_inferior *inf)
{
    // supress warnings;
    (void)command;
    (void)inf;

    printf("Exiting Debugger\n");
    return 1;
}

/** @brief Handler for 'continue' command */
int handle_continue(char **command, struct sylvan_inferior *inf)
{
    // supress warnings;
    (void)command;

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
    // supress warnings;
    (void)command;
    (void)inf;

    print_commands(SYLVAN_INFO_COMMAND);
    return 0;
}

/**
 * @brief Handler for info address command
 * @return 0 if success 1 for failure
 */
int handle_info_address(char **command, struct sylvan_inferior *inf)
{
    // supress warnings;
    (void)command;
    (void)inf;


    printf("Symbol Table Required\n");
    return 0;
}

/**
 * @brief Handler for info all-address command
 * @return 0 if success 1 for failure
 */
int handle_info_all_registers(char **command, struct sylvan_inferior *inf)
{
    // supress warnings;
    (void)command;
    (void)inf;


    printf("No Registers now\n");
    return 0;
}

int handle_info_args(char **command, struct sylvan_inferior *inf)
{
    (void)inf;
    (void)command;
    printf("Done by reading the registers\n");
    return 0;
}

int handle_info_auto_load(char **command, struct sylvan_inferior *inf)
{
    // supress warnings;
    (void)command;
    (void)inf;


    printf("no auto-load support\n");
    return 0;
}

int handle_info_auxv(char **command, struct sylvan_inferior *inf)
{
    // supress warnings;
    (void)command;

    if(!inf)
    {
        fprintf(stderr, "Null Inferior Pointer\n");
        return -1;
    }

    size_t len;
    unsigned char *raw_auxv = target_read_auxv(inf, &len);

    if(!raw_auxv)
    {
        return -1;
    }

    struct auxv_entry *entries = parse_auxv(raw_auxv, len, 1);
    free(raw_auxv);

    if(!entries)
    {
        fprintf(stderr, "Error: Failed to parse auxv");
        return -1;
    }
    
    printf("Auxiliary Vector for PID %d:\n", inf->pid);
    printf("Type  Value                 Name                Description\n");
    printf("----  --------------------  --------            -----------\n");
    for (size_t i = 0; entries[i].type != AT_NULL; i++) {
        print_auxv_entry(&entries[i]);
    }
    free(entries);
    return 0;
}