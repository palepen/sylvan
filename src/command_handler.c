#include <stdio.h>      // For printf, fprintf
#include <stdlib.h>     // For free
#include <unistd.h>     // For close
#include <elf.h>        // For ELF types (Elf64_Ehdr, etc.)
#include <fcntl.h>      // For open

#include "sylvan/inferior.h"    // For sylvan_inferior, sylvan_run, etc.
#include "command_handler.h"    // For sylvan_commands, sylvan_info_commands
#include "auxv.h"               // For auxv_entry, parse_auxv, etc.
#include "sylvan/error.h"       // For sylvan_code_t, sylvan_get_last_error
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

/**
 * @brief Handler for 'help' command
 * @param command Array of command strings
 * @param inf Pointer to the current inferior structure
 * @return 0 on success
 */
int handle_help(char **command, struct sylvan_inferior **inf)
{
    if (inf && command)
    {
        (void)command;
        (void)inf;
    }

    print_commands(SYLVAN_STANDARD_COMMAND);
    return 0;
}

/**
 * @brief Handler for 'quit' command
 * @param command Array of command strings
 * @param inf Pointer to the current inferior structure
 * @return 1 to indicate exit
 */
int handle_exit(char **command, struct sylvan_inferior **inf)
{
    if (inf && command)
    {
        (void)command;
        (void)inf;
    }

    printf("Exiting Debugger\n");
    return 1;
}

/**
 * @brief Handler for 'continue' command
 * @param command Array of command strings
 * @param inf Pointer to the current inferior structure
 * @return 0 on success, 1 on failure
 */
int handle_continue(char **command, struct sylvan_inferior **inf)
{
    (void)command;
    if (!inf)
    {
        fprintf(stderr, "Null Inferior Pointer\n");
        return 0;
    }

    struct sylvan_inferior *curr_inf = *inf;
    if (!curr_inf)
    {
        fprintf(stderr, "Null Inferior Pointer\n");
        return 0;
    }
    if (curr_inf->status != SYLVAN_INFSTATE_STOPPED)
    {
        printf("Process is not stopped\n");
        return 0;
    }
    if (sylvan_continue(curr_inf) < 0)
    {
        fprintf(stderr, "Failed to continue process\n");
        return 1;
    }
    return 0;
}

/**
 * @brief Handler for 'info' command
 * @param command Array of command strings
 * @param inf Pointer to the current inferior structure
 * @return 0 on success
 */
int handle_info(char **command, struct sylvan_inferior **inf)
{
    if (inf && command)
    {
        (void)command;
        (void)inf;
    }
    print_commands(SYLVAN_INFO_COMMAND);
    return 0;
}

/**
 * @brief Handler for 'info address' command
 * @param command Array of command strings
 * @param inf Pointer to the current inferior structure
 * @return 0 on success, 1 on failure
 */
int handle_info_address(char **command, struct sylvan_inferior **inf)
{
    if (inf && command)
    {
        (void)command;
        (void)inf;
    }

    printf("Symbol Table Required\n");
    return 0;
}

/**
 * @brief Handler for 'info all-registers' command
 * @param command Array of command strings
 * @param inf Pointer to the current inferior structure
 * @return 0 on success, 1 on failure
 */
int handle_info_all_registers(char **command, struct sylvan_inferior **inf)
{
    if (inf && command)
    {
        (void)command;
        (void)inf;
    }

    printf("No Registers now\n");
    return 0;
}

/**
 * @brief Handler for 'info args' command
 * @param command Array of command strings
 * @param inf Pointer to the current inferior structure
 * @return 0 on success
 */
int handle_info_args(char **command, struct sylvan_inferior **inf)
{
    if (inf && command)
    {
        (void)command;
        (void)inf;
    }
    printf("Done by reading the registers\n");
    return 0;
}

/**
 * @brief Handler for 'info auto-load' command
 * @param command Array of command strings
 * @param inf Pointer to the current inferior structure
 * @return 0 on success
 */
int handle_info_auto_load(char **command, struct sylvan_inferior **inf)
{
    if (inf && command)
    {
        (void)command;
        (void)inf;
    }
    printf("no auto-load support\n");
    return 0;
}

/**
 * @brief Handler for 'info auxv' command
 * @param command Array of command strings
 * @param inf Pointer to the current inferior structure
 * @return 0 on success, -1 on failure
 */
int handle_info_auxv(char **command, struct sylvan_inferior **inf)
{
    (void)command;
    
    if (!inf || !(*inf))
    {
        fprintf(stderr, "Null Inferior Pointer\n");
        return 0;
    }

    struct sylvan_inferior *curr_inf = *inf;
    if(curr_inf->pid == 0)
    {
        printf("Invalid PID\n");
        return 0;
    }

    size_t len;
    unsigned char *raw_auxv = target_read_auxv(curr_inf, &len);

    if (!raw_auxv)
    {
        return -1;
    }

    struct auxv_entry *entries = parse_auxv(raw_auxv, len, 1);
    free(raw_auxv);

    if (!entries)
    {
        fprintf(stderr, "Error: Failed to parse auxv\n");
        return -1;
    }

    printf("Auxiliary Vector for PID %d:\n", curr_inf->pid);
    printf("Type  Value                 Name                Description\n");
    printf("----  --------------------  --------            -----------\n");
    for (size_t i = 0; entries[i].type != AT_NULL; i++)
    {
        print_auxv_entry(&entries[i]);
    }
    free(entries);
    return 0;
}

/**
 * @brief Handler for 'info bookmark' command
 * @param command Array of command strings
 * @param inf Pointer to the current inferior structure
 * @return 0 on success, -1 on failure
 */
int handle_info_bookmark(char **command, struct sylvan_inferior **inf)
{
    if (!inf)
    {
        fprintf(stderr, "Error: Null Inferior Pointer\n");
        return -1;
    }

    if (inf && command)
    {
        (void)command;
        (void)inf;
    }

    printf("Not Implemented\n");
    return 0;
}

/**
 * @brief Handler for 'info breakpoints' command
 * @param command Array of command strings
 * @param inf Pointer to the current inferior structure
 * @return 0 on success
 */
int handle_info_breakpoints(char **command, struct sylvan_inferior **inf)
{
    if (inf && command)
    {
        (void)command;
        (void)inf;
    }

    printf("Num     Type            Address             Status\n");
    printf("----    --------        ----------------    ------------\n");
    return 0;
}

/**
 * @brief Handler for 'info copying' command
 * @param command Array of command strings
 * @param inf Pointer to the current inferior structure
 * @return 0 on success
 */
int handle_info_copying(char **command, struct sylvan_inferior **inf)
{
    (void)command;
    (void)inf;

    printf("Sylvan Copying Conditions:\n");
    printf("This is a placeholder for Sylvan's redistribution terms.\n");
    return 0;
}

/**
 * @brief Handler for 'info inferiors' command
 * @param command Array of command strings
 * @param inf Pointer to the current inferior structure
 * @return 0 on success
 */
int handle_info_inferiors(char **command, struct sylvan_inferior **inf)
{
    struct sylvan_inferior *curr_inf = *inf;
    (void)command;
    if (!inf || !curr_inf)
    {
        return 0;
    }

    printf("Id: %d\n", curr_inf->id);
    printf("\tPID: %d\n", curr_inf->pid);
    printf("\tPath: %s\n", curr_inf->realpath);

    return 0;
}

/**
 * @brief Handler for 'add-inferior' command
 * @param command Array of command strings
 * @param inf Pointer to the current inferior structure
 * @return 0 on success, 1 on failure
 */
int handle_add_inferior(char **command, struct sylvan_inferior **inf)
{
    if (!command[1])
    {
        printf("No file name provided\n");
        return 0;
    }
    
    if (*inf)
    {
        sylvan_inferior_destroy(*inf);  
    }
    sylvan_code_t ret = sylvan_inferior_create(inf);
    if (ret != SYLVANE_OK || !inf)
    {
        fprintf(stderr, sylvan_get_last_error());
        return 1;
    }
    
    const char *filepath = command[1];
    if (filepath)
    {
        ret = sylvan_set_filepath(*inf, filepath);
        if (ret != SYLVANE_OK)
        {
            fprintf(stderr, sylvan_get_last_error());
            sylvan_inferior_destroy(*inf);
            return 1;
        }
    }
    sylvan_run(*inf);
    struct sylvan_inferior *curr_inf = *inf;
    printf("Added inferior %d%s%s\n", curr_inf->pid, filepath ? " with executable " : "", filepath ? filepath : "");

    return 0;
}