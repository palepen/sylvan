#ifndef COMMAND_REGISTRY_H
#define COMMAND_REGISTRY_H
#include "user_interface.h"
#include "sylvan/inferior.h"
#include "uthash.h"

#define HASH_TABLE_SIZE 256
#define MAX_COMMAND_LENGTH 256

enum sylvan_command_type
{
    SYLVAN_STANDARD_COMMAND,
    SYLVAN_INFO_COMMAND,
    SYLVAN_SET_COMMAND,
};

/**
 * @brief Function pointer type for command handlers */
typedef int (*command_handler_t)(char **command, struct sylvan_inferior **inf);

/**
 * @brief Structure for command data with handler, function name, and hash */
struct sylvan_command_data
{
    char *name;
    char *description;
    command_handler_t handler;
    char *handler_name;
    int type;
    int id;
    char *usage;
    UT_hash_handle hh;
};

struct sylvan_command_alias
{
    char *name;
    char *org_cmd;
    int id;
    char type;
    UT_hash_handle hh;
};

extern struct sylvan_command_data *command_hash;
extern struct sylvan_command_alias *alias_table;

int handle_command(char **command, struct sylvan_inferior **inf);
int insert_alias(const char *name, const char *org_cmd, int id, char type);
struct sylvan_command_data *lookup_command(const char *command_name);
int init_commands();
void free_alias_table();

#endif