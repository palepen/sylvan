#include <stdio.h>
#include <string.h>
#include <sys/ptrace.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <signal.h>

#include "handle_command.h"
#include "sylvan/inferior.h"
#include "utils.h"
#include "command_handler.h"

size_t sylvan_command_size = 0;
size_t sylvan_info_command_size = 0;

#define HASH_TABLE_SIZE 128
static void *command_table[HASH_TABLE_SIZE] = {NULL};


struct sylvan_command_data sylvan_commands[] = {
    #define DEFINE_COMMAND(name, desc, handler, id) \
        {#name, desc, handler, "handle_" #name, 0, id}
    
    #include "details/standard_commands.def"
    #undef DEFINE_COMMAND
    {NULL, NULL, NULL, NULL, 0, 0}
};



struct sylvan_info_command sylvan_info_commands[] = {
    #define DEFINE_COMMAND(name, desc, handler, id) \
        {#name, desc, handler, "handle_info_" #name, 0, id}
    #include "details/info_subcommands.def"
    #undef DEFINE_COMMAND
    {NULL, NULL, NULL, NULL,0, 0}
};


static long long hash_string(const char *str)
{
    long long hash = 5381;
    int c;
    while ((c = *str++))
    {
        hash = ((hash << 5) + hash) + c; // hash * 33 + c
    }
    return hash;
}

/**
 * @brief Initializes the command hash table and sets handler functions, function names, and hashes
 */
void init_commands()
{
    int i = 0;

    // Initialize standard commands
    while (sylvan_commands[i].name && sylvan_commands[i].desc)
    {
        sylvan_commands[i].hash = hash_string(sylvan_commands[i].name);
        unsigned long hash = sylvan_commands[i].hash % HASH_TABLE_SIZE;
        while (command_table[hash] != NULL)
        {
            hash = (hash + 1) % HASH_TABLE_SIZE;
        }
        command_table[hash] = &sylvan_commands[i]; // Store pointer to command struct

        i++;
    }

    // Initialize info commands
    i = 0;
    while (sylvan_info_commands[i].name && sylvan_info_commands[i].desc)
    {
        char full_name[256];
        snprintf(full_name, sizeof(full_name), "info %s", sylvan_info_commands[i].name);
        sylvan_info_commands[i].hash = hash_string(full_name);
        unsigned long hash = sylvan_info_commands[i].hash % HASH_TABLE_SIZE;
        while (command_table[hash] != NULL)
        {
            hash = (hash + 1) % HASH_TABLE_SIZE;
        }
        command_table[hash] = &sylvan_info_commands[i]; // Store pointer to info command struct
        i++;
    }
}


/**
 * @brief Handles the input commands by the user
 * @param[in] command Command provided by the user
 * @param[in] inf The sylvan_inferior being debugged
 * @return 0 on success, 1 on failure (e.g., quit)
 */
int handle_command(char **command, struct sylvan_inferior *inf)
{

    long long input_hash = hash_string(command[0]);
    unsigned long hash = input_hash % HASH_TABLE_SIZE;
    size_t original_hash = hash;

    while (command_table[hash] != NULL)
    {
        struct sylvan_command_data *cmd = (struct sylvan_command_data *)command_table[hash];
        if (cmd->hash == input_hash)
        {
            if (strcmp(cmd->name, command[0]) == 0)
            {
                if(cmd->id == 5 && command[1] != NULL)
                {
                    break;
                }
                if (cmd->handler)
                {
                    return cmd->handler(command, inf);
                }
                else
                {
                    printf("Command recognized but not implemented: %s \n", command[0]);
                    return 0;
                }
            }
        }
        hash = (hash + 1) % HASH_TABLE_SIZE;
        if (hash == original_hash)
        break;
    }
 

    if (command[1] != NULL)
    {
        char full_command[256];
        snprintf(full_command, sizeof(full_command), "%s %s", command[0], command[1]);
        input_hash = hash_string(full_command);
        hash = input_hash % HASH_TABLE_SIZE;
        original_hash = hash;

        while (command_table[hash] != NULL)
        {
            struct sylvan_info_command *info_cmd = (struct sylvan_info_command *)command_table[hash];
            if (info_cmd->hash == input_hash && strcmp(info_cmd->name, command[1]) == 0 && info_cmd->id >= 101)
            {
                if (info_cmd->handler)
                {
                    return info_cmd->handler(command, inf);
                }
                else
                {
                    printf("Info command recognized but not implemented: %s %s\n",
                           command[0], command[1]);
                    return 0;
                }
            }
            hash = (hash + 1) % HASH_TABLE_SIZE;
            if (hash == original_hash)
                break;
        }
    }

    printf("Unknown command: %s\n", command[0]);
    printf("Type 'help' for available commands\n");
    return 0;
}

