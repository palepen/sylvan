#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <elf.h>
#include <fcntl.h>
#include <errno.h>
#include <string.h>

#include "sylvan/inferior.h"
#include "command_handler.h"
#include "command_registry.h"
#include "auxiliary_vectors.h"
#include "sylvan/error.h"
#include "register.h"
#include "ui_utils.h"

/**
 * @brief Prints available commands or info subcommands
 * @param tp Type of commands to print (standard, info, or set)
 * @param print_all 1 - prints all the programs, 0 - prints only the specified type
 */
static void print_commands(enum sylvan_command_type tp, int print_all)
{
    struct sylvan_command_data *cmd, *tmp;

    int print_standard = (tp == SYLVAN_STANDARD_COMMAND || print_all);
    int print_info = (tp == SYLVAN_INFO_COMMAND || print_all);
    int print_set = (tp == SYLVAN_SET_COMMAND || print_all);

    if (print_standard)
    {
        printf("Commands:\n");
        HASH_ITER(hh, command_hash, cmd, tmp)
        {
            if (cmd->type == SYLVAN_STANDARD_COMMAND)
            {
                printf("    %s  - %s\n", cmd->name, cmd->description);
            }
        }
    }

    if (print_info)
    {
        printf("Info Commands:\n");
        HASH_ITER(hh, command_hash, cmd, tmp)
        {
            if (cmd->type == SYLVAN_INFO_COMMAND)
            {
                printf("    %s  - %s\n", cmd->name, cmd->description);
            }
        }
    }

    if (print_set)
    {
        printf("Set Commands:\n");
        HASH_ITER(hh, command_hash, cmd, tmp)
        {
            if (cmd->type == SYLVAN_SET_COMMAND)
            {
                printf("    %s  - %s\n", cmd->name, cmd->description);
            }
        }
    }
}

/**
 * @brief Handler for 'help' command
 * @param command Array of command strings
 * @param inf Pointer to the current inferior structure
 */
int handle_help(char **command, struct sylvan_inferior **inf)
{
    if (inf)
    {
        (void)inf;
    }

    if (command[1] && (strcmp(command[1], "-a") == 0 || strcmp(command[1], "--all") == 0))
        print_commands(SYLVAN_STANDARD_COMMAND, 1);
    else
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
 */
int handle_info(char **command, struct sylvan_inferior **inf)
{
    if (command[1])
    {
        fprintf(stderr, "Invalid Arguments\n");
        return 0;
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
int handle_info_registers(char **command, struct sylvan_inferior **inf)
{
    if (command[1])
    {
        fprintf(stderr, "Invalid Arguments\n");
        return 0;
    }

    struct user_regs_struct *regs = (struct user_regs_struct *)malloc(sizeof(struct user_regs_struct));

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
    if (command[1])
    {
        fprintf(stderr, "Invalid Arguments\n");
        return 0;
    }
    if (!inf || !(*inf))
    {
        fprintf(stderr, "Null inferior\n");
    }

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
    if (command[1])
    {
        fprintf(stderr, "Invalid Arguments\n");
        return 0;
    }

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
    if (command[1])
    {
        fprintf(stderr, "Invalid Arguments\n");
        return 0;
    }

    if (!inf)
    {
        fprintf(stderr, "Null Inferior Pointer\n");
    }
    struct sylvan_inferior *curr_inf = *inf;

    struct table_col cols[] = {
        {"NUM", 5, TABLE_COL_INT},
        {"TYPE", 12, TABLE_COL_STR},
        {"ADDRESS", 18, TABLE_COL_HEX_LONG},
        {"STATUS", 10, TABLE_COL_INT}};

    int col_count = 4;

    struct table_row *rows = NULL, *current = NULL;
    for (int i = 0; i < curr_inf->breakpoint_count; i++)
    {
        struct table_row *new_row = malloc(sizeof(struct table_row));
        void *row_data = malloc(sizeof(int) + sizeof(char *) + sizeof(unsigned long) + sizeof(int));
        int *int_data = (int *)row_data;
        int_data[0] = i;
        *(const char **)(row_data + sizeof(int)) = "software";
        *(unsigned long *)(row_data + sizeof(int) + sizeof(char *)) = curr_inf->breakpoints[i].addr;
        int_data[3] = curr_inf->breakpoints[i].is_enabled_phy;
        new_row->data = row_data;
        new_row->next = NULL;

        if (!rows)
            rows = new_row;
        else
            current->next = new_row;
        current = new_row;
    }

    print_table("BREAKPOINTS", cols, col_count, rows, curr_inf->breakpoint_count);

    current = rows;
    while (current)
    {
        struct table_row *next = current->next;
        free((void *)current->data);
        free(current);
        current = next;
    }
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
    if (command[1])
    {
        fprintf(stderr, "Invalid Arguments\n");
        return 0;
    }

    struct sylvan_inferior *curr_inf = *inf;
    if (!inf || !curr_inf)
    {
        return 0;
    }

    printf("Id: %d\n", curr_inf->id);
    printf("\tPID: %d\n", curr_inf->pid);
    printf("\tPath: %s\n", curr_inf->realpath);
    printf("\tIs Attached: %d\n", curr_inf->is_attached);
    printf("\tNum Breakpoints: %d\n", curr_inf->breakpoint_count);
    return 0;
}

/**
 * @brief Handler for 'add-inferior' command creates a new inferior
 * @param command Array of command strings
 * @param inf Pointer to the current inferior structure
 */
int handle_add_inferior(char **command, struct sylvan_inferior **inf)
{
    if (command[1])
    {
        fprintf(stderr, "Invalid Arguments\n");
        return 0;
    }

    if (inf)
    {
        fprintf(stderr, "Null Inferior Pointer\n");
        return 0;
    }

    if (sylvan_inferior_destroy(*inf))
    {
        fprintf(stderr, "%s\n", sylvan_get_last_error());
        return 0;
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
    if (command[1])
    {
        fprintf(stderr, "Invalid Arguments\n");
        return 0;
    }

    if (!inf)
    {
        fprintf(stderr, "Null Inferior Pointer\n");
        return 0;
    }

    if (sylvan_run(*inf))
    {
        fprintf(stderr, "%s\n", sylvan_get_last_error());
    }

    return 0;
}

/**
 * @brief step instruction command
 */
int handle_step_inst(char **command, struct sylvan_inferior **inf)
{
    if (command[1])
    {
        fprintf(stderr, "Invalid Arguments\n");
        return 0;
    }

    if (inf)
    {
        fprintf(stderr, "Null Inferior Pointer\n");
        return 0;
    }
    if (sylvan_stepinst(*inf))
    {
        fprintf(stderr, "%s\n", sylvan_get_last_error());
        return 0;
    }

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

    if (command[2])
    {
        fprintf(stderr, "Invalid Arguments\n");
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

/**
 * @brief attach handler
 */
int handle_attach(char **command, struct sylvan_inferior **inf)
{

    if (command[1] == NULL)
    {
        fprintf(stderr, "Invalid Arguments\n");
        printf("Usage:\n");
        printf("   attach <pid>\n");

        return 0;
    }

    if (command[2])
    {
        fprintf(stderr, "Invalid Arguments\n");
        return 0;
    }

    char *endptr;
    errno = 0;
    long pid = strtol(command[1], &endptr, 10);

    if (errno == ERANGE || pid <= 0 || *endptr != '\0')
    {
        fprintf(stderr, "Invalid PID: %s\n", command[1]);
        printf("Usage:\n    attach <pid>\n");
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
        fprintf(stderr, "Invalid Arguments\n");
        return 0;
    }

    (void)inf;
    print_commands(SYLVAN_SET_COMMAND, 0);
    return 0;
}

/**
 * @brief sets arguments for the program
 */
int handle_set_args(char **command, struct sylvan_inferior **inf)
{

    if (inf)
    {
        fprintf(stderr, "Null Inferior Pointer\n");
        return 0;
    }

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

/**
 * @brief handler to set register values
 */
int handle_set_reg(char **command, struct sylvan_inferior **inf)
{
    if (inf)
    {
        fprintf(stderr, "Null Inferior Pointer\n");
        return 0;
    }

    if (command[2] == NULL || command[3] == NULL)
    {
        fprintf(stderr, "No name or value given\n");
        printf("Usage:\n    set reg <register name> <value>\n");
        return 0;
    }

    if (command[4])
    {
        fprintf(stderr, "Invalid Arguments\n");
        return 0;
    }

    int idx = find_register_by_name(command[2]);
    if (idx == -1)
    {
        fprintf(stderr, "register not found\n");
        return 0;
    }
    char *endptr;
    errno = 0;
    long val = strtol(command[3], &endptr, 10);

    if (errno == ERANGE || val <= 0 || *endptr != '\0')
    {
        fprintf(stderr, "Invalid value: %s\n", command[3]);
        printf("Usage:\n    set reg <value>\n");
        return 0;
    }

    struct user_regs_struct *regs = (struct user_regs_struct *)malloc(sizeof(struct user_regs_struct));

    if (sylvan_get_regs(*inf, regs))
    {
        fprintf(stderr, "%s\n", sylvan_get_last_error());
        free(regs);
        return 0;
    }

    memcpy(((uint8_t *)regs + sylvan_registers_info[idx].offset), &val, sizeof(uint64_t));

    if (sylvan_set_regs(*inf, regs))
    {
        fprintf(stderr, "%s\n", sylvan_get_last_error());
        free(regs);
        return 0;
    }

    printf("Set %s to %ld\n", sylvan_registers_info[idx].name, val);
    free(regs);
    return 0;
}

/**
 * @brief sets a breakpoint
 */
int handle_breakpoint_set(char **command, struct sylvan_inferior **inf)
{
    if (inf)
    {
        fprintf(stderr, "Null Inferior Pointer\n");
        return 0;
    }
    if (command[1] == NULL)
    {
        fprintf(stderr, "address missing\n");
        printf("Usage:\n    breakpoint <address>\n");

        return 0;
    }

    if (command[2])
    {
        fprintf(stderr, "Invalid Arguments\n");
        return 0;
    }

    char *endptr;
    errno = 0;
    uintptr_t addr = strtol(&command[1][2], &endptr, 16);

    if (errno == ERANGE || addr <= 0 || *endptr != '\0')
    {
        addr = 0;
        return 0;
    }

    if (sylvan_breakpoint_set(*inf, addr))
    {
        fprintf(stderr, "%s\n", sylvan_get_last_error());
        return 0;
    }

    printf("breakpoint set at: %ld\n", addr);

    return 0;
}

/**
 * @brief enables the breakpoint if isDisable != 0 or disables it
 */
static int toggle_breakpoint(char **command, struct sylvan_inferior *inf, int isDisable)
{
    if (command[1] == NULL)
    {
        fprintf(stderr, "Invalid arguments\n");
        printf("Usage:\n\tdisable <id>\n\tdisable -a <address>\n");
        return 0;
    }

    char *endptr;
    errno = 0;
    if (strcmp(command[1], "-a") != 0)
    {
        int id = strtol(command[1], &endptr, 10);

        if (errno == ERANGE || id <= 0 || *endptr != '\0')
        {
            id = 0;
            fprintf(stderr, "Invalid id\n");
            return 0;
        }

        if (id > inf->breakpoint_count)
        {
            fprintf(stderr, "Invalid id use: info breakpoint\n");
            return 0;
        }

        if (isDisable)
        {
            if (sylvan_breakpoint_disable(inf, inf->breakpoints[id].addr))
            {
                fprintf(stderr, "%s\n", sylvan_get_last_error());
                return 0;
            }
        }
        else
        {
            if (sylvan_breakpoint_enable(inf, inf->breakpoints[id].addr))
            {
                fprintf(stderr, "%s\n", sylvan_get_last_error());
                return 0;
            }
        }
        printf("breakpoint at addr 0x%lx  %s\n", inf->breakpoints[id].addr, (isDisable ? "disabled" : "enabled"));
        return 0;
    }

    if (command[2] == NULL)
    {
        fprintf(stderr, "Invalid arguments\n");
        printf("Usage:\n\tdisable <id>\n\tdisable -a <address>\n");
        return 0;
    }

    uintptr_t addr = strtol(&command[2][2], &endptr, 16);

    if (errno == ERANGE || addr <= 0 || *endptr != '\0')
    {
        addr = 0;
        fprintf(stderr, "Invalid address\n");
        return 0;
    }

    if (isDisable)
    {
        if (sylvan_breakpoint_disable(inf, addr))
        {
            fprintf(stderr, "%s\n", sylvan_get_last_error());
            return 0;
        }
    }
    else
    {
        if (sylvan_breakpoint_enable(inf, addr))
        {
            fprintf(stderr, "%s\n", sylvan_get_last_error());
            return 0;
        }
    }
    printf("breakpoint at addr 0x%lx  %s\n", addr, (isDisable ? "disabled" : "enabled"));

    return 0;
}

/**
 * @brief disables the breakpoint
 */
int handle_disable_breakpoint(char **command, struct sylvan_inferior **inf)
{
    if (inf)
    {
        fprintf(stderr, "Null Inferior Pointer\n");
        return 0;
    }

    return toggle_breakpoint(command, *inf, 1);
}

/**
 * @brief enables the breakpoint
 */
int handle_enable_breakpoint(char **command, struct sylvan_inferior **inf)
{
    if (inf)
    {
        fprintf(stderr, "Null Inferior Pointer\n");
        return 0;
    }

    return toggle_breakpoint(command, *inf, 0);
}

/**
 * @brief Deletes a breakpoint
 */
int handle_delete_breakpoint(char **command, struct sylvan_inferior **inf)
{
    if (inf)
    {
        fprintf(stderr, "Null Inferior Pointer\n");
        return 0;
    }

    if (command[1] == NULL)
    {
        fprintf(stderr, "Invalid arguments\n");
        printf("Usage:\n\tdisable <id>\n\tdisable -a <address>\n");
        return 0;
    }

    char *endptr;
    errno = 0;
    if (strcmp(command[1], "-a") != 0)
    {
        int id = strtol(command[1], &endptr, 10);

        if (errno == ERANGE || id <= 0 || *endptr != '\0')
        {
            id = 0;
            fprintf(stderr, "Invalid id\n");
            return 0;
        }

        if (id > (*inf)->breakpoint_count)
        {
            fprintf(stderr, "Invalid id use: (*info) breakpoint\n");
            return 0;
        }

        if (sylvan_breakpoint_unset((*inf), (*inf)->breakpoints[id].addr))
        {
            fprintf(stderr, "%s\n", sylvan_get_last_error());
            return 0;
        }

        else
        {
            if (sylvan_breakpoint_enable((*inf), (*inf)->breakpoints[id].addr))
            {
                fprintf(stderr, "%s\n", sylvan_get_last_error());
                return 0;
            }
        }

        printf("breakpoint at addr 0x%lx  is deleted\n", (*inf)->breakpoints[id].addr);
        return 0;
    }

    if (command[2] == NULL)
    {
        fprintf(stderr, "Invalid arguments\n");
        printf("Usage:\n\tdisable <id>\n\tdisable -a <address>\n");
        return 0;
    }

    uintptr_t addr = strtol(&command[2][2], &endptr, 16);

    if (errno == ERANGE || addr <= 0 || *endptr != '\0')
    {
        addr = 0;
        fprintf(stderr, "Invalid address\n");
        return 0;
    }

    if (sylvan_breakpoint_unset((*inf), addr))
    {
        fprintf(stderr, "%s\n", sylvan_get_last_error());
        return 0;
    }

    printf("breakpoint at addr 0x%lx  is deleted\n", addr);
    return 0;
}

/**
 * @brief Handles the 'set alias' command to create a new alias for an existing command
 * @param[in] command The command array (command[1] is the original command, command[2] is the alias name)
 * @param[in] inf The sylvan_inferior being debugged (unused in this function)
 * @return 1 on success, 0 on failure
 */
int handle_set_alias(char **command, struct sylvan_inferior **inf)
{
    (void)inf;
    if (command[1] == NULL || command[2] == NULL)
    {
        fprintf(stderr, "Invalid arguments: Usage: set alias <command> <alias>\n");
        return 1;
    }

    int alias_id = HASH_COUNT(alias_table) + 1;

    if (insert_alias(command[2], command[3], alias_id, 'u'))
    {
        fprintf(stderr, "Failed to add alias\n");
        return 1;
    }

    printf("Alias added successfully: '%s' -> '%s'\n", command[2], command[3]);
    return 0;
}

int handle_info_alias(char **command, struct sylvan_inferior **inf)
{
    (void)inf;
    if (command[1])
    {
        fprintf(stderr, "Invalid arguments\n");
        return 0;
    }
    struct sylvan_command_alias *cmd_alias, *tmp;

    printf("Command Name         Alias   \n");
    printf("-------------------- -------\n");

    HASH_ITER(hh, alias_table, cmd_alias, tmp)
    {
        printf("%-20s %-8s\n", cmd_alias->name, cmd_alias->org_cmd);
    }
    return 0;
}

static void print_memory_table(uint64_t *data, int num_rows, uintptr_t addr)
{
    struct table_col cols[] = {
        {"Address", 18, TABLE_COL_HEX_LONG},
        {"Value", 40, TABLE_COL_STR}};

    struct table_row *rows = NULL, *current = NULL;
    for (int i = 0; i < num_rows; i++)
    {

        char *value_str = malloc(24); // 8 * 2 + 7 spaces + 1 null
        uint8_t bytes[8];
        for (int j = 0; j < 8; j++)
        {
            bytes[j] = (data[i] >> (j * 8)) & 0xFF;
        }
        snprintf(value_str, 24,
                 "%02x %02x %02x %02x %02x %02x %02x %02x",
                 bytes[7], bytes[6], bytes[5], bytes[4],
                 bytes[3], bytes[2], bytes[1], bytes[0]);

        struct table_row *new_row = malloc(sizeof(struct table_row));
        void *row_data = malloc(sizeof(uintptr_t) + sizeof(char *));
        *(uintptr_t *)row_data = addr + (i * 8);
        *(char **)(row_data + sizeof(uintptr_t)) = value_str;
        new_row->data = row_data;
        new_row->next = NULL;

        if (!rows)
        {
            rows = new_row;
        }
        else
        {
            current->next = new_row;
        }
        current = new_row;
    }

    print_table("", cols, 2, rows, num_rows);

    current = rows;
    while (current)
    {
        struct table_row *next = current->next;
        free((char *)(*(char **)((char *)current->data + sizeof(uintptr_t)))); // Free value_str
        free((void *)current->data);
        free(current);
        current = next;
    }
}

int handle_read_memory(char **command, struct sylvan_inferior **inf)
{


    int p_table = 0;

    char *endptr;
    errno = 0;
    int num_rows = 1;

    if (command[1] && (strncmp(command[1], "-t", 2) == 0))
    {
        p_table = 1;
    }

    char *addr_str = command[1], *row_str = command[2];

    if (p_table)
    {
        addr_str = command[2];
        row_str = command[3];
    }

    if(!addr_str)
    {
        fprintf(stderr, "Invalid: No address provided\n");
        return 0;
    }

    if (row_str)
    {
        num_rows = strtol(row_str, &endptr, 10);
        if (errno == ERANGE || num_rows <= 0 || *endptr != '\0')
        {
            fprintf(stderr, "Invalid lines number\n");
            return 0;
        }
        if (num_rows > 16)
        {
            printf("Limiting the lines to 16\n");
            num_rows = 16;
        }
    }

    uintptr_t addr = strtol(&addr_str[2], &endptr, 16);

    if (errno == ERANGE || addr <= 0 || *endptr != '\0')
    {
        fprintf(stderr, "%sInvalid address: %s%s\n", RED, addr_str, RESET);
        return 0;
    }

    uint64_t *data = malloc(num_rows * sizeof(uint64_t));
    for (int i = 0; i < num_rows; i++)
    {
        uintptr_t current_addr = addr + (i * 8);
        if (sylvan_get_memory(*inf, current_addr, &data[i]))
        {
            fprintf(stderr, "%sError at 0x%016lx: %s%s\n", RED, current_addr, sylvan_get_last_error(), RESET);
            free(data);
            return 0;
        }

        if (!p_table)
        {
            printf("0x%016lx:     0x%016lx\n", current_addr, data[i]);
        }
    }

    if (p_table)
    {
        print_memory_table(data, num_rows, addr);
    }

    free(data);

    return 0;
}

int handle_write_memory(char **command, struct sylvan_inferior **inf)
{
    if (!command || !inf)
    {
        fprintf(stderr, "Invalid arguments\n");
        return 0;
    }

    if (!command[1])
    {
        fprintf(stderr, "Enter Valid Address\n");
        printf("Usage:\n\tmemory_write <addr(in hex eg: 0xabcd)> <bytes> ...\n");
        printf("\t<value>: hex (e.g., 0x12, ff), decimal (e.g., 255), or string (e.g., \"hello\")\n");
        return 0;
    }

    if (!command[2])
    {
        fprintf(stderr, "%sNo values provided%s\n", RED, RESET);
        printf("Usage:\n\tmemory_write <addr> <value> ...\n");
        printf("\t<value>: hex (e.g., 0x12, ff), decimal (e.g., 255), or string (e.g., \"hello\")\n");
        return 0;
    }
    char *endptr;
    errno = 0;
    uintptr_t addr = strtol(&command[1][2], &endptr, 16);
    if (errno == ERANGE || addr <= 0 || *endptr != '\0')
    {
        fprintf(stderr, "%sInvalid address: %s%s\n", RED, command[2], RESET);
        return 0;
    }

    uint8_t bytes[256];
    size_t size = 0;
    for (int i = 2; command[i] != NULL && size < 256; i++)
    {
        const char *arg = command[i];
        
        if (arg[0] == '"' && arg[strlen(arg) - 1] == '"') {
            size_t len = strlen(arg) - 2; // Exclude quotes
            if (size + len > 256) {
                fprintf(stderr, "%sToo many bytes (max 256)%s\n", RED, RESET);
                return 0;
            }
            for (size_t j = 0; j < len; j++) {
                bytes[size++] = (uint8_t)arg[j + 1];
            }
            continue;
        }

        errno = 0;
        int base = (strncmp(arg, "0x", 2) == 0) ? 16 : 10;
        unsigned long value = strtoul(arg, &endptr, base);
        
        if (errno == ERANGE || value > 0xFF || *endptr != '\0') {
            fprintf(stderr, "%sInvalid value: %s%s\n", RED, arg, RESET);
            return 0;
        }

        bytes[size++] = (uint8_t)value;
    }

    if (sylvan_set_memory(*inf, addr, bytes, size))
    {
        fprintf(stderr, "%s\n", sylvan_get_last_error());
    }

    return 0;
}