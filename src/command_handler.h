#ifndef COMMAND_HANDLER_H
#define COMMAND_HANDLER_H
#include "sylvan/inferior.h"

int handle_help(char **command, struct sylvan_inferior *inf);
int handle_exit(char **command, struct sylvan_inferior *inf);
int handle_continue(char **command, struct sylvan_inferior *inf);
int handle_info(char **command, struct sylvan_inferior *inf);

#endif