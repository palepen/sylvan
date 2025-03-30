#ifndef COMMAND_HANDLER_H
#define COMMAND_HANDLER_H
#include "sylvan/inferior.h"

int handle_help(char **command, struct sylvan_inferior **inf);
int handle_exit(char **command, struct sylvan_inferior **inf);
int handle_continue(char **command, struct sylvan_inferior **inf);
int handle_info(char **command, struct sylvan_inferior **inf);
int handle_add_inferior(char **command, struct sylvan_inferior **inf);

int handle_info_address(char **command, struct sylvan_inferior **inf);
int handle_info_args(char **command, struct sylvan_inferior **inf);
int handle_info_auto_load(char **command, struct sylvan_inferior **inf);
int handle_info_auxv(char **command, struct sylvan_inferior **inf);
int handle_info_all_registers(char **command, struct sylvan_inferior **inf);
int handle_info_bookmark(char **command, struct sylvan_inferior **inf);
int handle_info_breakpoints(char **command, struct sylvan_inferior **inf);
int handle_info_copying(char **command, struct sylvan_inferior **inf);
int handle_info_inferiors(char **command, struct sylvan_inferior **inf);
int handle_run(char **command, struct sylvan_inferior **inf);
int handle_step_inst(char **command, struct sylvan_inferior **inf);
int handle_file(char **command, struct sylvan_inferior **inf);
int handle_attach(char **command, struct sylvan_inferior **inf);
int handle_set(char **command, struct sylvan_inferior **inf);
int handle_set_args(char **command, struct sylvan_inferior **inf);

#endif