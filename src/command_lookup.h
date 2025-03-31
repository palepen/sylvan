#ifndef COMMAND_LOOKUP_H
#define COMMAND_LOOKUP_H
#include "user_interface.h"
#include "sylvan/inferior.h"

enum sylvan_command_type
{
    SYLVAN_STANDARD_COMMAND,
    SYLVAN_INFO_COMMAND,
    SYLVAN_SET_COMMAND,
};

/** @brief Function pointer type for command handlers */
typedef int (*command_handler_t)(char **command, struct sylvan_inferior **inf);

/** @brief Structure for command data with handler, function name, and hash */
struct sylvan_command_data
{
    const char *name;                      ///< Command name
    const char *desc;                      ///< Command description
    command_handler_t handler;             ///< Handler function for the command
    const char *func_name;                 ///< Name of the handler function
    unsigned long long hash;                        ///< Precomputed hash of the command name
    enum sylvan_command_type command_type; ///< INFO_COMMMAND OR STANDARD_COMMAND or SET_COMMAND
    int id;                                ///< Unique command ID
};

/**
 * @brief file/target data structure 
 */

struct debug_target {
    int target_id;       // Unique ID for the target
    char *target_name;   // e.g., "PID 1234"
    char *executable;    // Path to the executable
    char **source_files; // Array of source file paths
    int source_count;    // Number of source files
};

extern struct sylvan_command_data sylvan_commands[];
extern struct sylvan_command_data sylvan_sub_commands[];
extern int handle_command(char **command, struct sylvan_inferior **inf);
extern int init_commands();
#endif