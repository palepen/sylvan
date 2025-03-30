#ifndef DEFINE_COMMAND
#error "This file is intended for textual inclusion with the DEFINE_COMMAND macro defined"
#endif

DEFINE_COMMAND(info address, "Describe where symbol SYM is stored.", handle_info_address, 101, 1),
DEFINE_COMMAND(info all-registers, "List of all registers and their contents.", handle_info_all_registers, 102, 1),
DEFINE_COMMAND(info args, "All argument variables of current stack frame or those matching REGEXPs.", handle_info_args, 103, 1),
DEFINE_COMMAND(info auto-load, "Print current status of auto-loaded files.", handle_info_auto_load, 104, 1),
DEFINE_COMMAND(info auxv, "Display the inferior's auxiliary vector.", handle_info_auxv, 105, 1),
DEFINE_COMMAND(info bookmarks, "Status of user-settable bookmarks.", handle_info_bookmark, 106, 1),
DEFINE_COMMAND(info breakpoints, "Status of specified breakpoints (all user-settable breakpoints if no argument).", handle_info_breakpoints, 107, 1),
DEFINE_COMMAND(info copying, "Conditions for redistributing copies of Sylvan.", handle_info_copying, 108, 1),
DEFINE_COMMAND(info inferiors, "Print a list of inferiors being managed.", handle_info_inferiors, 109, 1),
DEFINE_COMMAND(set args, "Set arguments", handle_set_args, 201, 2),
