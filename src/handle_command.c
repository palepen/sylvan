#include <stdio.h>
#include <string.h>
#include <sys/ptrace.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <signal.h>

#include "handle_command.h"
#include "sylvan/inferior.h"
#include "command_handler.h"

#define HASH_TABLE_SIZE 256
#define MAX_COMMAND_LENGTH 256

// Command data structures
struct sylvan_command_data sylvan_commands[] = {
#define DEFINE_COMMAND(name, desc, handler, id) \
    {#name, desc, handler, "handle_" #name, 0, SYLVAN_STANDARD_COMMAND, id}

#include "details/standard_commands.h"
#undef DEFINE_COMMAND
    {NULL, NULL, NULL, NULL, 0, 0, 0}};

struct sylvan_command_data sylvan_info_commands[] = {
#define DEFINE_COMMAND(name, desc, handler, id) \
    {#name, desc, handler, "handle_info_" #name, 0, SYLVAN_INFO_COMMAND, id}
#include "details/info_subcommands.h"
#undef DEFINE_COMMAND
    {NULL, NULL, NULL, NULL, 0, 1, 0}};

// Global variables
size_t sylvan_command_size = 0;
size_t sylvan_info_command_size = 0;
static struct sylvan_command_data *command_table[HASH_TABLE_SIZE] = {NULL};

/**
 * @brief Computes a hash value for a string using the DJB2 algorithm
 * @param[in] str The string to hash
 * @return The hash value
 */
static unsigned long hash_string(const char *str)
{
    if (!str)
        return 0;

    unsigned long hash = 5381;
    int c;

    while ((c = *str++))
    {
        hash = ((hash << 5) + hash) + c; // hash * 33 + c
    }

    return hash;
}

/**
 * @brief Inserts a command into the hash table
 * @param[in] cmd Pointer to the command to insert
 * @param[in] full_name The full name of the command (including prefix for info commands)
 */
static int insert_command(struct sylvan_command_data *cmd, const char *full_name)
{
    if (!cmd || !full_name)
        return -1;

    cmd->hash = hash_string(full_name);
    unsigned long idx = cmd->hash % HASH_TABLE_SIZE;
    unsigned long original_idx = idx;

    // Linear probing for collision resolution
    while (command_table[idx] != NULL)
    {
        idx = (idx + 1) % HASH_TABLE_SIZE;
        if (idx == original_idx)
        {
            fprintf(stderr, "Error: Command hash table is full\n");
            return -1;
        }
    }

    command_table[idx] = cmd;
    return 0;
}

/**
 * @brief Counts the number of commands in an array
 * @param[in] commands Array of command data structures
 * @return The number of commands
 */
static size_t count_commands(struct sylvan_command_data *commands)
{
    size_t count = 0;
    if (!commands)
        return count;

    while (commands[count].name && commands[count].desc)
    {
        count++;
    }

    return count;
}

/**
 * @brief Initializes the command hash table
 */
int init_commands()
{
    memset(command_table, 0, sizeof(command_table));

    // Count commands
    sylvan_command_size = count_commands(sylvan_commands);
    sylvan_info_command_size = count_commands(sylvan_info_commands);

    // Initialize standard commands
    for (size_t i = 0; i < sylvan_command_size; i++)
    {
        if (insert_command(&sylvan_commands[i], sylvan_commands[i].name) == -1)
        {
            fprintf(stderr, "Failed to insert command: %s\n", sylvan_commands[i].name);
            return -1;
        }
    }

    // Initialize info commands
    for (size_t i = 0; i < sylvan_info_command_size; i++)
    {
        char full_name[MAX_COMMAND_LENGTH];
        int result = snprintf(full_name, sizeof(full_name), "info %s", sylvan_info_commands[i].name);

        if (result < 0 || result >= (int)sizeof(full_name))
        {
            fprintf(stderr, "Error creating full name for info command: %s\n", sylvan_info_commands[i].name);
            return -1;
        }

        if (insert_command(&sylvan_info_commands[i], full_name) == -1)
        {
            fprintf(stderr, "Failed to insert info command: %s\n", full_name);
            return -1;
        }
    };
    return 0;
}

/**
 * @brief Looks up a command in the hash table
 * @param[in] command_name The name of the command to look up
 * @return Pointer to the command data structure if found, NULL otherwise
 */
static struct sylvan_command_data *lookup_command(const char *command_name)
{
    if (!command_name)
        return NULL;

    unsigned long hash_value = hash_string(command_name);
    unsigned long idx = hash_value % HASH_TABLE_SIZE;
    unsigned long original_idx = idx;

    while (command_table[idx] != NULL)
    {
        struct sylvan_command_data *cmd = command_table[idx];

        if (cmd->hash == hash_value && strcmp(cmd->name, command_name) == 0)
        {
            return cmd;
        }

        idx = (idx + 1) % HASH_TABLE_SIZE;
        if (idx == original_idx)
            break;
    }

    return NULL;
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

    // First, try to find the command directly
    struct sylvan_command_data *cmd = lookup_command(command[0]);
    if (cmd && !(cmd->id == 5 && command[1] != NULL))
    {
        if (cmd->handler)
        {
            return cmd->handler(command, inf);
        }
        else
        {
            printf("Command recognized but not implemented: %s\n", command[0]);
            return 0;
        }
    }
    
    // If not found or special case, try info command lookup
    if (command[1] != NULL && strcmp(command[0], "info") == 0)
    {
        char full_command[MAX_COMMAND_LENGTH];
        int result = snprintf(full_command, sizeof(full_command), "%s %s", command[0], command[1]);

        if (result < 0 || result >= (int)sizeof(full_command))
        {
            fprintf(stderr, "Error creating full command name\n");
            return 0;
        }

        unsigned long hash_value = hash_string(full_command);
        unsigned long idx = hash_value % HASH_TABLE_SIZE;
        unsigned long original_idx = idx;

        while (command_table[idx] != NULL)
        {
            struct sylvan_command_data *info_cmd = command_table[idx];

            if (info_cmd->hash == hash_value &&
                strcmp(info_cmd->name, command[1]) == 0 &&
                info_cmd->id >= 101)
            {
                if (info_cmd->handler)
                {
                    return info_cmd->handler(command, inf);
                }
                else
                {
                    printf("Info command recognized but not implemented: %s %s\n", command[0], command[1]);
                    return 0;
                }
            }

            idx = (idx + 1) % HASH_TABLE_SIZE;
            if (idx == original_idx)
                break;
        }
    }

    // Command not found
    printf("Unknown command: %s\n", command[0]);
    printf("Type 'help' for available commands\n");
    return 0;
}

