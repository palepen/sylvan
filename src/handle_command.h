#ifndef HANDLE_COMMAND_H
#define HANDLE_COMMAND_H
#include "user_interface.h"
#include "sylvan/inferior.h"

enum sylvan_command_type
{
    SYLVAN_INFO_COMMAND,
    SYLVAN_STANDARD_COMMAND 
};


/** @brief Function pointer type for command handlers */
typedef int (*command_handler_t)(char **command, struct sylvan_inferior *inf);

/** @brief Structure for command data with handler, function name, and hash */
struct sylvan_command_data {
    const char *name;         ///< Command name
    const char *desc;         ///< Command description
    command_handler_t handler;///< Handler function for the command
    const char *func_name;    ///< Name of the handler function
    long long hash;           ///< Precomputed hash of the command name
    int id;                   ///< Unique command ID
};

/** @brief Structure for info command data with handler, function name, and hash */
struct sylvan_info_command {
    const char *name;         ///< Info subcommand name (without "info ")
    const char *desc;         ///< Info subcommand description
    command_handler_t handler;///< Handler function for the info subcommand
    const char *func_name;    ///< Name of the handler function
    long long hash;           ///< Precomputed hash of the command name
    int id;                   ///< Unique info command ID
};

extern struct sylvan_command_data sylvan_commands[];
extern struct sylvan_info_command sylvan_info_commands[];
extern int handle_command(char **command, struct sylvan_inferior *inf);
extern void init_commands();
#endif