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
#include "disassemble.h"

/**
 * @brief Prints available commands or info subcommands with detailed usage
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
        printf("%sCommands:%s\n", CYAN, RESET);
        HASH_ITER(hh, command_hash, cmd, tmp)
        {
            if (cmd->type == SYLVAN_STANDARD_COMMAND)
            {
                printf("  %s%s%s\n", GREEN, cmd->name, RESET);
                printf("    %sDescription:%s %s%s%s\n", BLUE, RESET, YELLOW, cmd->description, RESET);
                printf("    %sUsage:%s %s\n", BLUE, RESET, cmd->usage);
                printf("\n");
            }
        }
    }

    if (print_info)
    {
        printf("%sInfo Commands:%s\n", CYAN, RESET);
        HASH_ITER(hh, command_hash, cmd, tmp)
        {
            if (cmd->type == SYLVAN_INFO_COMMAND)
            {
                printf("  %s%s%s\n", GREEN, cmd->name, RESET);
                printf("    %sDescription:%s %s%s%s\n", BLUE, RESET, YELLOW, cmd->description, RESET);
                printf("    %sUsage:%s %s\n", BLUE, RESET, cmd->usage);
                printf("\n");
            }
        }
    }

    if (print_set)
    {
        printf("%sSet Commands:%s\n", CYAN, RESET);
        HASH_ITER(hh, command_hash, cmd, tmp)
        {
            if (cmd->type == SYLVAN_SET_COMMAND)
            {
                printf("  %s%s%s\n", GREEN, cmd->name, RESET);
                printf("    %sDescription:%s %s%s%s\n", BLUE, RESET, YELLOW, cmd->description, RESET);
                printf("    %sUsage:%s %s\n", BLUE, RESET, cmd->usage);
                printf("\n");
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
    if (!inf)
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

    printf("%sExiting Debugger%s\n", MAGENTA, RESET);
    return 1;
}

/**
 * @brief Handler for 'continue' command
 * @param command Array of command strings
 * @param inf Pointer to the current inferior structures
 */
int handle_continue(char **command, struct sylvan_inferior **inf)
{
    (void)command;
    struct sylvan_inferior *curr_inf = *inf;

    if (sylvan_continue(curr_inf))
    {
        sylvan_print_error(sylvan_get_last_error());
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
        sylvan_print_error("Invalid Arguments");
    else
        print_commands(SYLVAN_INFO_COMMAND, 0);

    (void)inf;
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
        sylvan_print_error("Invalid Arguments");
        return 0;
    }

    struct user_regs_struct *regs = (struct user_regs_struct *)malloc(sizeof(struct user_regs_struct));

    if (sylvan_get_regs(*inf, regs))
    {

        sylvan_print_error(sylvan_get_last_error());
        free(regs);
        return 0;
    }
    print_registers(regs); // Assuming this function handles its own formatting
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
        sylvan_print_error("Invalid Arguments");
        return 0;
    }

    if (!inf || !(*inf))
    {
        sylvan_print_error("Null Inferior Pointer");
        return 0;
    }

    printf("Arguments: %s%s%s\n", GREEN, (*inf)->args, RESET);
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
        sylvan_print_error("Invalid Arguments");
        return 0;
    }

    if (!inf || !(*inf))
    {
        sylvan_print_error("Null Inferior Pointer");
        return 0;
    }

    struct sylvan_inferior *curr_inf = *inf;
    if (curr_inf->pid == 0)
    {
        sylvan_print_error("Invalid PID");
        return 0;
    }

    size_t len;
    unsigned char *raw_auxv = target_read_auxv(curr_inf, &len);

    if (!raw_auxv)
    {
        return 0;
    }

    struct auxv_entry *entries = parse_auxv(raw_auxv, len, 1);
    free(raw_auxv);

    if (!entries)
    {
        sylvan_print_error("Failed to parse auxv");
        return 0;
    }

    printf("%sAuxiliary Vector for PID %d:%s\n", CYAN, curr_inf->pid, RESET);
    printf("%sType  Value                 Name                Description%s\n", BLUE, RESET);
    printf("%s----  --------------------  --------            -----------%s\n", BLUE, RESET);
    for (size_t i = 0; entries[i].type != AT_NULL; i++)
    {
        print_auxv_entry(&entries[i]);
    }
    free(entries);
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
        sylvan_print_error("Invalid Arguments");
        return 0;
    }

    struct sylvan_inferior *curr_inf = *inf;
    if (!curr_inf)
    {
        sylvan_print_error("Null inferior pointer");
        return 0;
    }

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
        void *row_data = malloc(sizeof(int) + sizeof(char *) + sizeof(uint64_t) + sizeof(int));
        *(int *)(row_data) = i;
        *(const char **)(row_data + sizeof(int)) = "software";
        *(uint64_t *)(row_data + sizeof(int) + sizeof(char *)) = curr_inf->breakpoints[i].addr;
        *(int *)(row_data + sizeof(int) + sizeof(char *) + sizeof(uint64_t)) = curr_inf->breakpoints[i].is_enabled_log;
        new_row->data = row_data;
        new_row->next = NULL;
        
        printf("%ld\n", *(unsigned long *)(row_data + sizeof(int)));
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
 * @brief Handler for 'info inferiors' command
 * @param command Array of command strings
 * @param inf Pointer to the current inferior structure
 */
int handle_info_inferiors(char **command, struct sylvan_inferior **inf)
{
    if (command[1])
    {
        sylvan_print_error("Invalid Arguments");
        return 0;
    }

    struct sylvan_inferior *curr_inf = *inf;
    if (!inf || !curr_inf)
    {
        return 0;
    }

    printf("%sId:%s %d\n", BLUE, RESET, curr_inf->id);
    printf("\t%sPID:%s %d\n", BLUE, RESET, curr_inf->pid);
    printf("\t%sPath:%s %s\n", BLUE, RESET, curr_inf->realpath);
    printf("\t%sIs Attached:%s %d\n", BLUE, RESET, curr_inf->is_attached);
    printf("\t%sNum Breakpoints:%s %d\n", BLUE, RESET, curr_inf->breakpoint_count);
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
        sylvan_print_error("Invalid Arguments");
        return 0;
    }

    if (!inf)
    {
        sylvan_print_error("Null Inferior Pointer");
        return 0;
    }

    if (sylvan_inferior_destroy(*inf))
    {
        sylvan_print_error(sylvan_get_last_error());
        return 0;
    }

    if (sylvan_inferior_create(inf))
    {
        sylvan_print_error(sylvan_get_last_error());
        return 0;
    }

    sylvan_print_ok("Created New Inferior: ID: %d", (*inf)->id);
    return 0;
}

/**
 * @brief runs a program which is given in args
 */
int handle_run(char **command, struct sylvan_inferior **inf)
{
    if (command[1])
    {
        sylvan_print_error("Invalid Arguments");
        return 0;
    }

    if (!inf)
    {
        sylvan_print_error("Null Inferior Pointer");
        return 0;
    }

    if (sylvan_run(*inf))
    {
        sylvan_print_error(sylvan_get_last_error());
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
        sylvan_print_error("Invalid Arguments");
        return 0;
    }

    if (!inf)
    {
        sylvan_print_error("Null Inferior Pointer");
        return 0;
    }
    if (sylvan_stepinst(*inf))
    {
        sylvan_print_error(sylvan_get_last_error());
        return 0;
    }

    sylvan_print_ok("Single Instruction executed");
    return 0;
}

int handle_file(char **command, struct sylvan_inferior **inf)
{
    if (command[1] == NULL)
    {
        sylvan_print_error("No filename provided");
        return 0;
    }

    if (command[2])
    {
        sylvan_print_error("Invalid Arguments");
        return 0;
    }

    if((*inf)->pid != 0)
    {
        if(sylvan_inferior_destroy(*inf))
        {
            sylvan_print_error(sylvan_get_last_error());
            return 0;
        }

        if(sylvan_inferior_create(inf))
        {
            sylvan_print_error(sylvan_get_last_error());
            return 0;
        }
    }

    const char *filepath = command[1];

    if (sylvan_set_filepath(*inf, filepath))
    {
        sylvan_print_error(sylvan_get_last_error());
        return 0;
    }

    sylvan_print_ok("File %s assigned to inferior id: %d", filepath, (*inf)->id);
    return 0;
}

/**
 * @brief attach handler
 */
int handle_attach(char **command, struct sylvan_inferior **inf)
{
    if (command[1] == NULL)
    {
        sylvan_print_error("Invalid Arguments");
        sylvan_print_instruction("\tattach <pid>");
        return 0;
    }

    if (command[2])
    {
        sylvan_print_error("Invalid Arguments");
        return 0;
    }

    char *endptr;
    errno = 0;
    long pid = strtol(command[1], &endptr, 10);

    if (errno == ERANGE || pid <= 0 || *endptr != '\0')
    {
        sylvan_print_error("Invalid PID: %s", command[1]);
        sylvan_print_instruction("\tattach <pid>");
        return 0;
    }


    if((*inf)->pid != 0)
    {
        if(sylvan_inferior_destroy(*inf))
        {
            sylvan_print_error(sylvan_get_last_error());
            return 0;
        }

        if(sylvan_inferior_create(inf))
        {
            sylvan_print_error(sylvan_get_last_error());
            return 0;
        }
    }

    if (sylvan_attach(*inf, pid))
    {
        sylvan_print_error(sylvan_get_last_error());
        return 0;
    }
    sylvan_print_ok("Process PID: %ld to inferior id: %d", pid, (*inf)->id);
    return 0;
}

/**
 * @brief Handler for 'set' command
 * @param command Array of command strings
 * @param inf Pointer to the current inferior structure
 */
int handle_set(char **command, struct sylvan_inferior **inf)
{
    if (command[1])
    {
        sylvan_print_error("Invalid Arguments");
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
    if (!inf)
    {
        sylvan_print_error("Null Inferior Pointer");
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
        sylvan_print_error("Memory allocation failed");
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
        sylvan_print_error(sylvan_get_last_error());
        free(args);
        return 0;
    }
    sylvan_print_ok("Added arguments to inferior: %d\n", (*inf)->id);

    free(args);
    return 0;
}

/**
 * @brief handler to set register values
 */
int handle_set_reg(char **command, struct sylvan_inferior **inf)
{
    if (!inf)
    {
        sylvan_print_error("Null Inferior Pointer");
        return 0;
    }

    if (command[2] == NULL || command[3] == NULL)
    {
        sylvan_print_error("No name or value given");
        sylvan_print_instruction("\tset reg <register name> <value>");
        return 0;
    }

    if (command[4])
    {
        sylvan_print_error("Invalid Arguments");
        return 0;
    }

    int idx = find_register_by_name(command[2]);
    if (idx == -1)
    {
        sylvan_print_error("register not found");
        return 0;
    }
    char *endptr;
    errno = 0;
    long val = strtol(command[3], &endptr, 10);

    if (errno == ERANGE || val <= 0 || *endptr != '\0')
    {
        sylvan_print_error("Invalid value: %s", command[3]);
        sylvan_print_instruction("\tset reg <value>");
        return 0;
    }

    struct user_regs_struct *regs = (struct user_regs_struct *)malloc(sizeof(struct user_regs_struct));

    if (sylvan_get_regs(*inf, regs))
    {
        sylvan_print_error(sylvan_get_last_error());
        free(regs);
        return 0;
    }

    memcpy(((uint8_t *)regs + sylvan_registers_info[idx].offset), &val, sizeof(uint64_t));

    if (sylvan_set_regs(*inf, regs))
    {
        sylvan_print_error(sylvan_get_last_error());
        free(regs);
        return 0;
    }

    sylvan_print_ok("%s to %ld\n", sylvan_registers_info[idx].name, val);
    free(regs);
    return 0;
}

/**
 * @brief sets a breakpoint
 */
int handle_breakpoint_set(char **command, struct sylvan_inferior **inf)
{
    if (!inf)
    {
        sylvan_print_error("Null Inferior Pointer");
        return 0;
    }
    if (command[1] == NULL)
    {
        sylvan_print_error("address missing");
        sylvan_print_instruction("\tbreakpoint <address>");
        return 0;
    }

    if (command[2])
    {
        sylvan_print_error("Invalid Arguments");
        return 0;
    }

    char *endptr;
    errno = 0;
    uintptr_t addr = strtol(&command[1][2], &endptr, 16);
    if (errno == ERANGE || *endptr != '\0')
    {
        addr = 0;
    }

    if (command[1][0] != '0')
    {
        size_t func_sz = 0;
        if (get_function_bounds((*inf)->realpath, command[1], &addr, &func_sz))
        {
            return 0;
        }
    }

    if ((addr != 0 ) && sylvan_breakpoint_set(*inf, addr))
    {
        sylvan_print_error(sylvan_get_last_error());
        return 0;
    }

    sylvan_print_ok("breakpoint set at: 0x%016lx", addr);
    return 0;
}

/**
 * @brief enables the breakpoint if isDisable != 0 or disables it
 */
static int toggle_breakpoint(char **command, struct sylvan_inferior *inf, int isDisable)
{
    if (command[1] == NULL)
    {
        sylvan_print_error("Invalid Arguments");
        sylvan_print_instruction("\tdisable <id>\n\tdisable -a <address>");
        return 0;
    }

    char *endptr;
    errno = 0;
    if (strcmp(command[1], "-a") != 0)
    {
        int id = strtol(command[1], &endptr, 10);

        if (errno == ERANGE || id < 0 || *endptr != '\0')
        {
            id = 0;
            sylvan_print_error("Invalid id");
            return 0;
        }

        if (id > inf->breakpoint_count)
        {
            sylvan_print_error("Invalid id use: info breakpoint");
            return 0;
        }

        if (isDisable)
        {
            if (sylvan_breakpoint_disable(inf, inf->breakpoints[id].addr))
            {
                sylvan_print_error(sylvan_get_last_error());
                return 0;
            }
        }
        else
        {
            if (sylvan_breakpoint_enable(inf, inf->breakpoints[id].addr))
            {
                sylvan_print_error(sylvan_get_last_error());
                return 0;
            }
        }
        sylvan_print_ok("breakpoint at addr 0x%lx  %s", inf->breakpoints[id].addr, (isDisable ? "disabled" : "enabled"));
        return 0;
    }

    if (command[2] == NULL)
    {
        sylvan_print_error("Invalid Arguments");
        
        sylvan_print_instruction("\tdisable <id>\n\tdisable -a <address>");
        return 0;
    }

    uintptr_t addr = strtol(&command[2][2], &endptr, 16);

    if (errno == ERANGE || addr <= 0 || *endptr != '\0')
    {
        addr = 0;
        sylvan_print_error("Invalid address");
        return 0;
    }

    if (isDisable)
    {
        if (sylvan_breakpoint_disable(inf, addr))
        {
            sylvan_print_error(sylvan_get_last_error());
            return 0;
        }
    }
    else
    {
        if (sylvan_breakpoint_enable(inf, addr))
        {
            sylvan_print_error(sylvan_get_last_error());
            return 0;
        }
    }
    sylvan_print_ok("breakpoint at addr 0x%lx  %s", addr, (isDisable ? "disabled" : "enabled"));
    return 0;
}

/**
 * @brief disables the breakpoint
 */
int handle_disable_breakpoint(char **command, struct sylvan_inferior **inf)
{
    if (!inf)
    {
        sylvan_print_error("Null Inferior Pointer");
        return 0;
    }

    toggle_breakpoint(command, *inf, 1);
    return 0;
}

/**
 * @brief enables the breakpoint
 */
int handle_enable_breakpoint(char **command, struct sylvan_inferior **inf)
{
    if (!inf)
    {
        sylvan_print_error("Null Inferior Pointer");
        return 0;
    }

    toggle_breakpoint(command, *inf, 0);
    return 0;
}

/**
 * @brief Deletes a breakpoint
 */
int handle_delete_breakpoint(char **command, struct sylvan_inferior **inf)
{
    if (!inf)
    {
        sylvan_print_error("Null Inferior Pointer");
        return 0;
    }

    if (command[1] == NULL)
    {
        sylvan_print_error("Invalid Arguments");
        sylvan_print_instruction("\tdelete <id>\n\tdelete -a <address>");
        return 0;
    }

    char *endptr;
    errno = 0;
    printf("asfas\n");
    if (strcmp(command[1], "-a") != 0)
    {
        int id = strtol(command[1], &endptr, 10);
        if (errno == ERANGE || id < 0 || *endptr != '\0')
        {
            sylvan_print_error("Invalid id");
            return 0;
        }

        if (id > (*inf)->breakpoint_count)
        {
            sylvan_print_error("Invalid id use: info_breakpoint");
            return 0;
        }

        if (sylvan_breakpoint_unset((*inf), (*inf)->breakpoints[id].addr))
        {
            sylvan_print_error(sylvan_get_last_error());
            return 0;
        }
        
        sylvan_print_ok("breakpoint at addr 0x%lx  is deleted", (*inf)->breakpoints[id].addr);
        return 0;
    }

    if (command[2] == NULL)
    {
        sylvan_print_error("Invalid Arguments");
        sylvan_print_instruction("\tdelete <id>\n\tdelete -a <address>");
        return 0;
    }

    uintptr_t addr = strtol(&command[2][2], &endptr, 16);

    if (errno == ERANGE || addr == 0 || *endptr != '\0')
    {
        sylvan_print_error("Invalid Address");
        return 0;
    }

    if (sylvan_breakpoint_unset((*inf), addr))
    {
        sylvan_print_error(sylvan_get_last_error());
        return 0;
    }

    sylvan_print_ok("breakpoint at addr 0x%lx  is deleted", addr);
    return 0;
}

/**
 * @brief Handles the 'set alias' command to create a new alias for an existing command
 * @param[in] command The command array (command[1] is the original command, command[2] is the alias name)
 * @param[in] inf The sylvan_inferior being debugged (unused in this function)
 */
int handle_set_alias(char **command, struct sylvan_inferior **inf)
{
    (void)inf;
    if (command[1] == NULL || command[2] == NULL)
    {
        sylvan_print_error("Invalid arguments");
        return 0;
    }

    int alias_id = HASH_COUNT(alias_table) + 1;

    if (insert_alias(command[2], command[3], alias_id, 'u'))
    {
        sylvan_print_error("Failed to add alias");
        return 0;
    }

    sylvan_print_ok("Alias added successfully: '%s' -> '%s'", command[2], command[3]);
    return 0;
}

int handle_info_alias(char **command, struct sylvan_inferior **inf)
{
    (void)inf;
    if (command[1])
    {
        sylvan_print_error("Invalid Arguments");
        return 0;
    }
    struct sylvan_command_alias *cmd_alias, *tmp;

    printf("%sCommand Name         Alias   %s\n", BLUE, RESET);
    printf("%s-------------------- -------%s\n", BLUE, RESET);

    HASH_ITER(hh, alias_table, cmd_alias, tmp)
    {
        printf("%s%-20s%s %s%-8s%s\n", GREEN, cmd_alias->name, RESET, YELLOW, cmd_alias->org_cmd, RESET);
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

    if (!addr_str)
    {
        sylvan_print_error("Invalid: No address provided");
        return 0;
    }

    if (row_str)
    {
        num_rows = strtol(row_str, &endptr, 10);
        if (errno == ERANGE || num_rows <= 0 || *endptr != '\0')
        {
            sylvan_print_error("Invalid lines number");
            return 0;
        }
        if (num_rows > 16)
        {
            printf("%sLimiting the lines to 16%s\n", YELLOW, RESET);
            num_rows = 16;
        }
    }

    if(strncmp(addr_str, "0x", 2) == 0)
    {
        sylvan_print_error("Invalid Address");
        sylvan_print_instruction("Ex: memory_read 0x1234");
        return 0;
    }

    uintptr_t addr = strtol(&addr_str[2], &endptr, 16);

    if (errno == ERANGE || addr <= 0 || *endptr != '\0')
    {
        sylvan_print_error("Invalid address: %s", addr_str);
        return 0;
    }

    uint64_t *data = malloc(num_rows * sizeof(uint64_t));
    for (int i = 0; i < num_rows; i++)
    {
        uintptr_t current_addr = addr + (i * 8);
        if (sylvan_get_memory(*inf, current_addr, &data[i]))
        {
            sylvan_print_error("Error at 0x%016lx: %s", current_addr, sylvan_get_last_error());
            free(data);
            return 0;
        }

        if (!p_table)
        {
            printf("%s0x%016lx:%s     %s0x%016lx%s\n", BLUE, current_addr, RESET, GREEN, data[i], RESET);
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
        sylvan_print_error("Invalid Arguments");
        return 0;
    }
    
    if (!command[1] || !command[2] || (strncmp(command[1], "0x", 2) == 0))
    {
        sylvan_print_error("Enter Valid Address");
        sylvan_print_instruction("\tmemory_write <addr(in hex eg: 0xabcd)> <bytes> ...");
        printf("\t%s<value>: hex (e.g., 0x12, ff), decimal (e.g., 255), or string (e.g., \"hello\")%s\n", YELLOW, RESET);
        return 0;
    }

    char *endptr;
    errno = 0;
    uintptr_t addr = strtol(&command[1][2], &endptr, 16);
    if (errno == ERANGE || addr <= 0 || *endptr != '\0')
    {
        sylvan_print_error("Invalid address: %s", command[2], RESET);
        return 0;
    }

    uint8_t bytes[256];
    size_t size = 0;
    for (int i = 2; command[i] != NULL && size < 256; i++)
    {
        const char *arg = command[i];

        if (arg[0] == '"' && arg[strlen(arg) - 1] == '"')
        {
            size_t len = strlen(arg) - 2; // Exclude quotes
            if (size + len > 256)
            {
                sylvan_print_error("Too many bytes (max 256)");
                return 0;
            }
            for (size_t j = 0; j < len; j++)
            {
                bytes[size++] = (uint8_t)arg[j + 1];
            }
            continue;
        }

        errno = 0;
        int base = (strncmp(arg, "0x", 2) == 0) ? 16 : 10;
        unsigned long value = strtoul(arg, &endptr, base);

        if (errno == ERANGE || value > 0xFF || *endptr != '\0')
        {
            sylvan_print_error("Invalid value: %s", arg);
            return 0;
        }

        bytes[size++] = (uint8_t)value;
    }

    if (sylvan_set_memory(*inf, addr, bytes, size))
    {
        sylvan_print_error(sylvan_get_last_error());
    }

    return 0;
}

int handle_disassemble(char **command, struct sylvan_inferior **inf)
{
    if (!command || !inf || !(*inf))
    {
        sylvan_print_error("Null Inferior Pointer");
        return 0;
    }

    if (!command[1])
    {
        sylvan_print_error("Enter Valid start Address");
        sylvan_print_instruction("\tdissassemble <addr> <endaddr>\n\tdisassemble <function>\n\tdisassemble -c");
        return 0;
    }

    char *endptr;

    uintptr_t start_addr, end_addr;
    int is_func = 1;
    if (strncmp(command[1], "-c", 3) == 0)
    {
        struct user_regs_struct *regs = (struct user_regs_struct *)malloc(sizeof(struct user_regs_struct));
        if (sylvan_get_regs(*inf, regs))
        {
            sylvan_print_error(sylvan_get_last_error());
            return 0;
        }

        start_addr = regs->rip;
        end_addr = start_addr + 15;
        is_func = 0;
    }

    if (command[2])
    {
        errno = 0;
        start_addr = strtol(command[1], &endptr, 16);
        if (errno == ERANGE || start_addr <= 0 || *endptr != '\0')
        {
            sylvan_print_error("Invalid start address: %s", command[1]);
            return 0;
        }

        errno = 0;
        end_addr = strtol(command[2], &endptr, 16);
        if (errno == ERANGE || end_addr <= start_addr || *endptr != '\0')
        {
            sylvan_print_error("Invalid end address: %s", command[2]);
            return 0;
        }
    }

    if (is_func)
    {
        size_t func_sz = 0;
        if (get_function_bounds((*inf)->realpath, command[1], &start_addr, &func_sz))
        {
            return 0;
        }

        end_addr = start_addr + func_sz;
    }

    struct disassembled_instruction *instructions = NULL;
    int count = 0;
    int result = disassemble(*inf, start_addr, end_addr, &instructions, &count);

    if (result == 0)
    {
        print_disassembly(instructions, count);
    }

    struct disassembled_instruction *current = instructions;
    while (current)
    {
        struct disassembled_instruction *next = current->next;
        free(current->opcodes);
        free(current->instruction);
        free(current);
        current = next;
    }

    return 0;
}