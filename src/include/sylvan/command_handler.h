#ifndef COMMAND_HANDLER_H
#define COMMAND_HANDLER_H
#include <sylvan/user_interface.h>
#include <sylvan/inferior.h>

extern int handle_command(char *command, struct inferior *inf);

#endif