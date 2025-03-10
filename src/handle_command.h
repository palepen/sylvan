#ifndef COMMAND_HANDLER_H
#define COMMAND_HANDLER_H
#include "user_interface.h"
#include "sylvan/inferior.h"

enum command_type
{
    SYLVAN_INFO_COMMAND,
    SYLVAN_STANDARD_COMMAND 
};
struct command_data
{
    char *name;
    char *desc;
    size_t hash;
    enum command_type cmd_type;
};


extern struct command_data commands_available[];

extern int handle_command(char **command, struct sylvan_inferior *inf);
extern int create_hash();

#endif