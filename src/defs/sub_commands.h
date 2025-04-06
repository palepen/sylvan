#ifndef DEFINE_COMMAND
#error "This file is intended for textual inclusion with the DEFINE_COMMAND macro defined"
#endif


DEFINE_COMMAND(info_registers,      "Display the values of all CPU registers for the current inferior", 
                handle_info_registers,      102, SYLVAN_INFO_COMMAND, 
                "info registers - Display all register values"),
DEFINE_COMMAND(info_args,           "List all argument variables in the current stack frame", 
                handle_info_args,           103, SYLVAN_INFO_COMMAND, 
                "info args - Show current stack frame arguments"),
DEFINE_COMMAND(info_breakpoints,    "List all user-set breakpoints with their status, type, and address", 
                handle_info_breakpoints,    105, SYLVAN_INFO_COMMAND, 
                "info breakpoints - List all breakpoints"),
DEFINE_COMMAND(info_inferiors,      "List all inferiors being managed, including their IDs, PIDs, and paths", 
                handle_info_inferiors,      107, SYLVAN_INFO_COMMAND, 
                "info inferiors - List all managed inferiors"),
DEFINE_COMMAND(info_alias,          "Display all defined command aliases with their original commands", 
                handle_info_alias,          108, SYLVAN_INFO_COMMAND, 
                "info alias - List all command aliases"),
DEFINE_COMMAND(set_args,            "Set command-line arguments for the program in the current inferior", 
                handle_set_args,            201, SYLVAN_SET_COMMAND, 
                "set args <arg1> [arg2...] - Set program arguments (e.g., arg1 arg2)"),
DEFINE_COMMAND(set_reg,             "Assign a value to a specified CPU register (e.g., rax, rbx)", 
                handle_set_reg,             202, SYLVAN_SET_COMMAND, 
                "set reg <register> <value> - Set a register value (e.g., rax 123)"),
DEFINE_COMMAND(set_alias,           "Create an alias for an existing command to simplify usage", 
                handle_set_alias,           203, SYLVAN_SET_COMMAND, 
                "set alias <command> <alias> - Create a command alias (e.g., run r)"),