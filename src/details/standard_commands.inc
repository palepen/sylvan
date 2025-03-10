#ifndef DEFINE_COMMAND
#error "This file is intended for textual inclusion with the DEFINE_COMMAND macro defined"
#endif

DEFINE_COMMAND(help, "list the commands available", handle_help, 1),
DEFINE_COMMAND(exit, "exit the debugger", handle_exit, 2),
DEFINE_COMMAND(continue, "continue the program", handle_continue, 3),
DEFINE_COMMAND(breakpoint, "set breakpoint on the given line", NULL, 4),
DEFINE_COMMAND(info, "List all info commands", handle_info, 5),
