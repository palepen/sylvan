#include <stdio.h>
#include <string.h>
#include <sys/ptrace.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <signal.h>

#include "command_registry.h"
#include "sylvan/inferior.h"
#include "command_handler.h"

struct sylvan_command_data *command_hash = NULL;

struct sylvan_command_alias *alias_table = NULL;

static struct sylvan_command_data static_commands[] = {
#define DEFINE_COMMAND(name, desc, handler, id) \
    {#name, desc, handler, #handler, SYLVAN_STANDARD_COMMAND, id, {0}}
#include "defs/standard_commands.h"
#undef DEFINE_COMMAND

#define DEFINE_COMMAND(name, desc, handler, id, type) \
    {#name, desc, handler, #handler, type, id, {0}}
#include "defs/sub_commands.h"
#undef DEFINE_COMMAND
    {NULL, NULL, NULL, NULL, 0, 0, {0}}
};

static struct sylvan_command_alias static_aliases[] = {
#define DEFINE_ALIAS(name, org, id) \
    {name, org, id, 's', {0}}
#include "defs/command_alias.h"
#undef DEFINE_ALIAS
    {NULL, NULL, 0, 0, {0}}
};

/**
 * @brief Inserts an alias into the uthash table (supports dynamic allocation)
 * @param[in] name The alias name
 * @param[in] org_cmd The original command name
 * @param[in] id The alias ID
 * @param[in] type The type of alias ('s' for static, 'u' for user-defined)
 * @return 0 on success, 1 on failure
 */
int insert_alias(const char *name, const char *org_cmd, int id, char type)
{
    if (!name || !org_cmd)
    {
        return 1;
    }


    struct sylvan_command_alias *existing = NULL;
    HASH_FIND_STR(alias_table, name, existing);
    if (existing)
    {
        fprintf(stderr, "Error: Alias '%s' already exists\n", name);
        return 1;
    }

    struct sylvan_command_data *orig_cmd = lookup_command(org_cmd);
    if (!orig_cmd)
    {
        fprintf(stderr, "Invalid command: '%s' not found\n", orig_cmd);
        return 1;
    }

    struct sylvan_command_alias *cmd_al = (struct sylvan_command_alias *)malloc(sizeof(struct sylvan_command_alias));
    if (!cmd_al)
    {
        fprintf(stderr, "Error: Failed to allocate memory for alias\n");
        return 1;
    }


    cmd_al->name = strdup(name);
    cmd_al->org_cmd = strdup(org_cmd);
    if (!cmd_al->name || !cmd_al->org_cmd)
    {
        free(cmd_al->name);
        free(cmd_al->org_cmd);
        free(cmd_al);
        fprintf(stderr, "Error: Failed to allocate memory for alias strings\n");
        return 1;
    }

    cmd_al->id = id;
    cmd_al->type = type;


    HASH_ADD_STR(alias_table, name, cmd_al);
    printf("Added alias: %s -> %s (type: %c, id: %d)\n", cmd_al->name, cmd_al->org_cmd, cmd_al->type, cmd_al->id);
    return 0;
}

/**
 * @brief Frees all dynamically allocated aliases in the alias table
 */
void free_alias_table()
{
    struct sylvan_command_alias *current, *tmp;

    HASH_ITER(hh, alias_table, current, tmp)
    {
        if(current->type == 'u')
        {

            HASH_DEL(alias_table, current);
            free(current->name);
            free(current->org_cmd);
            free(current);
        }
    }
    alias_table = NULL;
}
/**
 * @brief Initializes the command and alias hash tables
 */
int init_commands()
{

    command_hash = NULL;
    alias_table = NULL;

    for (size_t i = 0; static_commands[i].name != NULL; i++)
    {
        HASH_ADD_STR(command_hash, name, &static_commands[i]);
    }

    for (size_t i = 0; static_aliases[i].name != NULL; i++)
    {
        HASH_ADD_STR(alias_table, name, &static_aliases[i]);
    }

    return 0;
}

/**
 * @brief Looks up a command in the uthash table
 * @param[in] command_name The name of the command to look up
 * @return Pointer to the command data structure if found, NULL otherwise
 */
struct sylvan_command_data *lookup_command(const char *command_name)
{
    if (!command_name)
    {
        return NULL;
    }

    struct sylvan_command_alias *alias = NULL;
    HASH_FIND_STR(alias_table, command_name, alias);
    if (alias)
    {
        command_name = alias->org_cmd;
    }

    struct sylvan_command_data *cmd = NULL;
    HASH_FIND_STR(command_hash, command_name, cmd);
    return cmd;
}

/**
 * @brief Handles the input commands by the user
 * @param[in] command Command provided by the user
 * @param[in] inf The sylvan_inferior being debugged
 */
int handle_command(char **command, struct sylvan_inferior **inf)
{
    if (!command || !command[0])
    {
        fprintf(stderr, "Error: Invalid command\n");
        return 0;
    }

    struct sylvan_command_data *cmd = lookup_command(command[0]);
    int res;
    if (cmd && cmd->handler)
    {
        return cmd->handler(command, inf);
    }

    printf("Unknown command: %s\n", command[0]);
    printf("Type 'help' for available commands\n");
    return 0;
}