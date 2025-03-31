#include <stdio.h>  // For printf, fprintf
#include <stdlib.h> // For free
#include <unistd.h> // For close
#include <elf.h>    // For ELF types (Elf64_Ehdr, etc.)
#include <fcntl.h>  // For open
#include <errno.h>
#include <string.h>

#include "sylvan/inferior.h" // For sylvan_inferior, sylvan_run, etc.
#include "command_handler.h" // For sylvan_commands, sylvan_info_commands
#include "auxv.h"            // For auxv_entry, parse_auxv, etc.
#include "sylvan/error.h"    // For sylvan_code_t, sylvan_get_last_error
#include "handle_command.h"
#include "register.h"

/**
 * @brief Prints available commands or info subcommands
 * @param tp Type of commands to print (standard or info)
 * @param print_all 1 - prints all the programs
 */
static void print_commands(enum sylvan_command_type tp, int print_all)
{
    size_t i = 0;
    switch (tp)
    {
    case SYLVAN_STANDARD_COMMAND:
        printf("Commands:\n");
        while (sylvan_commands[i].name)
        {
            printf("    %s  - %s\n", sylvan_commands[i].name, sylvan_commands[i].desc);
            i++;
        }
        if (print_all == 0)
            break;

    case SYLVAN_INFO_COMMAND:
        printf("Info Commands:\n");
        while (sylvan_sub_commands[i].name)
        {
            if (sylvan_sub_commands[i].command_type == SYLVAN_INFO_COMMAND)
                printf("    %s  - %s\n", sylvan_sub_commands[i].name, sylvan_sub_commands[i].desc);
            i++;
        }
        if (print_all == 0)
            break;

    case SYLVAN_SET_COMMAND:
        printf("Set Commands:\n");
        while (sylvan_sub_commands[i].name)
        {
            if (sylvan_sub_commands[i].command_type == SYLVAN_SET_COMMAND)
                printf("    %s  - %s\n", sylvan_sub_commands[i].name, sylvan_sub_commands[i].desc);
            i++;
        }
        if (print_all == 0)
            break;
    default:
        break;
    }
}

/**
 * @brief Handler for 'help' command
 * @param command Array of command strings
 * @param inf Pointer to the current inferior structure
 */
int handle_help(char **command, struct sylvan_inferior **inf)
{
    if (inf && command)
    {
        (void)command;
        (void)inf;
    }

    print_commands(SYLVAN_STANDARD_COMMAND, 0);
    return 0;
}

/**
 * @brief Handler for 'quit' command
 * @param command Array of command strings
 * @param inf Pointer to the current inferior structure
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

    if (sylvan_continue(curr_inf))
    {
        fprintf(stderr, "%s\n", sylvan_get_last_error());
        return 0;
    }
    return 0;
}

/**
 * @brief Handler for 'info' command
 * @param command Array of command strings
 * @param inf Pointer to the current inferior structure
 * @return 0 to success and 2 to move to sub_command
 */
int handle_info(char **command, struct sylvan_inferior **inf)
{
    if (command[1])
    {
        return 2;
    }
    (void)inf;
    print_commands(SYLVAN_INFO_COMMAND, 0);
    return 0;
}

/**
 * @brief Handler for 'info address' command
 * @param command Array of command strings
 * @param inf Pointer to the current inferior structure
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
 */
int handle_info_all_registers(char **command, struct sylvan_inferior **inf)
{
    if (inf && command)
    {
        (void)command;
        (void)inf;
    }
    struct user_regs_struct *regs = (struct user_regs_struct*) malloc(sizeof(struct user_regs_struct));
    
    if (sylvan_get_regs(*inf, regs))
    {
        fprintf(stderr, "%s\n", sylvan_get_last_error());
        free(regs);
        return 0;
    }
    print_registers(regs);
    free(regs);
    return 0;
}

/**
 * @brief Handler for 'info args' command
 * @param command Array of command strings
 * @param inf Pointer to the current inferior structure
 */
int handle_info_args(char **command, struct sylvan_inferior **inf)
{
    if (!inf || !(*inf))
    {
        fprintf(stderr, "Null inferior\n");
    }
    (void)command;

    printf("Arguments: %s\n", (*inf)->args);
    return 0;
}

/**
 * @brief Handler for 'info auto-load' command
 * @param command Array of command strings
 * @param inf Pointer to the current inferior structure
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
    if (curr_inf->pid == 0)
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
    printf("\tIs Attached: %d\n", curr_inf->is_attached);

    return 0;
}

/**
 * @brief Handler for 'add-inferior' command creates a new inferior
 * @param command Array of command strings
 * @param inf Pointer to the current inferior structure
 */
int handle_add_inferior(char **command, struct sylvan_inferior **inf)
{

    if (*inf)
    {
        if (sylvan_inferior_destroy(*inf))
        {
            fprintf(stderr, "%s\n", sylvan_get_last_error());
            return 0;
        }
    }

    if (sylvan_inferior_create(inf))
    {
        fprintf(stderr, "%s\n", sylvan_get_last_error());
        return 0;
    }

    printf("Created New Inferior: \n\tID: %d\n", (*inf)->id);
    return 0;
}

/**
 * @brief runs a program which is given in args
 */
int handle_run(char **command, struct sylvan_inferior **inf)
{
    if (sylvan_run(*inf))
    {
        fprintf(stderr, "%s\n", sylvan_get_last_error());
    }
    (void)command;
    return 0;
}

int handle_step_inst(char **command, struct sylvan_inferior **inf)
{
    if (sylvan_stepinst(*inf))
    {
        fprintf(stderr, "%s\n", sylvan_get_last_error());
        return 0;
    }
    (void)command;
    printf("Single Instruction executed\n");
    return 0;
}

int handle_file(char **command, struct sylvan_inferior **inf)
{

    if (command[1] == NULL)
    {
        fprintf(stderr, "No filename provided\n");
        return 0;
    }
    const char *filepath = command[1];

    if (sylvan_set_filepath(*inf, filepath))
    {
        fprintf(stderr, "%s\n", sylvan_get_last_error());
        return 0;
    }

    printf("File %s assigned to inferior id: %d\n", filepath, (*inf)->id);
    return 0;
}

int handle_attach(char **command, struct sylvan_inferior **inf)
{
    
    if (command[1] == NULL || strcmp(command[1], "-p") != 0)
    {
        fprintf(stderr, "Invalid Arguments\n");
        printf("Usage:\n");
        printf("   attach -p <pid>\n");

        return 0;
    }
    if (command[2] == NULL)
    {
        fprintf(stderr, "No Pid\n");
        printf("Usage:\n");
        printf("    attach -p <pid>\n");
        return 0;
    }

    char *endptr;
    errno = 0;
    long pid = strtol(command[2], &endptr, 10);

    if (errno == ERANGE || pid <= 0 || *endptr != '\0')
    {
        fprintf(stderr, "Invalid PID: %s\n", command[2]);
        printf("Usage:\n    add-inferior <filepath>\n    add-inferior -p <pid>\n");
        return 0;
    }

    if (sylvan_attach(*inf, pid))
    {
        fprintf(stderr, "%s\n", sylvan_get_last_error());
        return 0;
    }
    printf("Process PID: %ld to inferior id: %d\n", pid, (*inf)->id);
    return 0;
}

/**
 * @brief Handler for 'set' command
 * @param command Array of command strings
 * @param inf Pointer to the current inferior structure
 * @return 0 to success and 2 to move to sub_command
 */
int handle_set(char **command, struct sylvan_inferior **inf)
{
    if (command[1])
    {
        return 2;
    }
    (void)inf;
    print_commands(SYLVAN_SET_COMMAND, 0);
    return 0;
}

int handle_set_args(char **command, struct sylvan_inferior **inf)
{

    size_t total_len = 0;
    int arg_count = 0;

    while (command[arg_count] != NULL)
    {
        total_len += strlen(command[arg_count]) + 1;
        arg_count++;
    }

    char *args = (char *)malloc(sizeof(char) * total_len);
    if (!args)
    {
        fprintf(stderr, "Memory allocation failed\n");
        return 0;
    }

    args[0] = '\0';

    for (int i = 0; i < arg_count; i++)
    {
        if (i > 0)
        {
            strcat(args, " ");
        }
        strcat(args, command[i]);
    }

    if (sylvan_set_args(*inf, args))
    {
        fprintf(stderr, "%s\n", sylvan_get_last_error());
        free(args);
        return 0;
    }
    printf("Added arguments to inferior: %d\n", (*inf)->id);

    free(args);
    return 0;
}
