#ifndef DEFINE_COMMAND
#error "This file is intended for textual inclusion with the DEFINE_COMMAND macro defined"
#endif

DEFINE_COMMAND(address, "Describe where symbol SYM is stored.", handle_info_address, 101),
DEFINE_COMMAND(all-registers, "List of all registers and their contents, for selected stack frame.", handle_info_all_registers, 102),
DEFINE_COMMAND(args, "All argument variables of current stack frame or those matching REGEXPs.", handle_info_args, 103),
DEFINE_COMMAND(auto-load, "Print current status of auto-loaded files.", handle_info_auto_load, 104),
DEFINE_COMMAND(auxv, "Display the inferior's auxiliary vector.", handle_info_auxv, 105),
DEFINE_COMMAND(bookmarks, "Status of user-settable bookmarks.", handle_info_bookmark, 106),
DEFINE_COMMAND(breakpoints, "Status of specified breakpoints (all user-settable breakpoints if no argument).", handle_info_breakpoints, 107),
DEFINE_COMMAND(copying, "Conditions for redistributing copies of Sylvan.", handle_info_copying, 112),
DEFINE_COMMAND(inferiors, "Print a list of inferiors being managed.", handle_info_inferiors, 123),
