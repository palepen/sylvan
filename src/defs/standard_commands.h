#ifndef DEFINE_COMMAND
#error "This file is intended for textual inclusion with the DEFINE_COMMAND macro defined"
#endif

DEFINE_COMMAND(help,            "List all available commands with usage details; use -a or --all to show all types", 
                handle_help,                1,  SYLVAN_STANDARD_COMMAND, 
                "help [-a|--all] - List commands; use -a for all types"),
DEFINE_COMMAND(quit,            "Exit the Sylvan debugger cleanly, terminating the session", 
                handle_exit,                2,  SYLVAN_STANDARD_COMMAND, 
                "quit - Exit the debugger"),
DEFINE_COMMAND(continue,        "Resume execution of the current inferior until the next breakpoint or end", 
                handle_continue,            3,  SYLVAN_STANDARD_COMMAND, 
                "continue - Resume program execution"),
DEFINE_COMMAND(breakpoint,      "Set a breakpoint at a specified address (hex) or function name", 
                handle_breakpoint_set,      4,  SYLVAN_STANDARD_COMMAND, 
                "breakpoint <address|function> - Set a breakpoint (e.g., 0x1234 or main)"),
DEFINE_COMMAND(info,            "Display a list of all info subcommands with descriptions and usage", 
                handle_info,                5,  SYLVAN_STANDARD_COMMAND, 
                "info - List all info subcommands"),
DEFINE_COMMAND(add_inferior,    "Create a new inferior for debugging a separate process or program", 
                handle_add_inferior,        6,  SYLVAN_STANDARD_COMMAND, 
                "add_inferior - Create a new inferior"),
DEFINE_COMMAND(disassemble,     "Disassemble machine code between two addresses or for a function; use -c for current instruction", 
                handle_disassemble,         7,  SYLVAN_STANDARD_COMMAND, 
                "disassemble <start> [end] | -c - Disassemble code (e.g., 0x1000 0x1010 or -c)"),
DEFINE_COMMAND(run,             "Start execution of the program assigned to the current inferior", 
                handle_run,                 8,  SYLVAN_STANDARD_COMMAND, 
                "run - Start the program in the current inferior"),
DEFINE_COMMAND(stepi,           "Execute a single machine instruction and stop", 
                handle_step_inst,           9,  SYLVAN_STANDARD_COMMAND, 
                "stepi - Step one instruction"),
DEFINE_COMMAND(file,            "Assign an executable file to the current inferior for debugging", 
                handle_file,                10, SYLVAN_STANDARD_COMMAND, 
                "file <path> - Assign an executable (e.g., ./a.out)"),
DEFINE_COMMAND(attach,          "Attach the debugger to a running process by its PID", 
                handle_attach,              11, SYLVAN_STANDARD_COMMAND, 
                "attach <pid> - Attach to a process (e.g., 1234)"),
DEFINE_COMMAND(set,             "Display a list of all set subcommands with descriptions and usage", 
                handle_set,                 12, SYLVAN_STANDARD_COMMAND, 
                "set - List all set subcommands"),
DEFINE_COMMAND(disable,         "Disable a breakpoint by its ID or address (use -a <address>)", 
                handle_disable_breakpoint,  13, SYLVAN_STANDARD_COMMAND, 
                "disable <id> | -a <address> - Disable a breakpoint (e.g., 1 or -a 0x1234)"),
DEFINE_COMMAND(enable,          "Enable a breakpoint by its ID or address (use -a <address>)", 
                handle_enable_breakpoint,   14, SYLVAN_STANDARD_COMMAND, 
                "enable <id> | -a <address> - Enable a breakpoint (e.g., 1 or -a 0x1234)"),
DEFINE_COMMAND(delete,          "Delete a breakpoint by its ID or address (use -a <address>)", 
                handle_delete_breakpoint,   15, SYLVAN_STANDARD_COMMAND, 
                "delete <id> | -a <address> - Delete a breakpoint (e.g., 1 or -a 0x1234)"),
DEFINE_COMMAND(memory_read,     "Read memory contents from a specified address; optional -t for table format", 
                handle_read_memory,         16, SYLVAN_STANDARD_COMMAND, 
                "memory_read [-t] <address> [rows] - Read memory (e.g., 0x1000 or -t 0x1000 4)"),
DEFINE_COMMAND(memory_write,    "Write values (hex, decimal, or string) to a specified memory address", 
                handle_write_memory,        17, SYLVAN_STANDARD_COMMAND, 
                "memory_write <address> <value>... - Write to memory (e.g., 0x1000 0x12 \"hello\")"),