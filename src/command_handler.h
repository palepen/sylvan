#ifndef COMMAND_HANDLER_H
#define COMMAND_HANDLER_H
#include "user_interface.h"
#include <sylvan/inferior.h>

extern int handle_command(char *command, struct sylvan_inferior *inf);

#endif