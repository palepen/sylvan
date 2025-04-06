#ifndef DEFINE_COMMAND
#error "This file is intended for textual inclusion with the DEFINE_COMMAND macro defined"
#endif

DEFINE_COMMAND(help,            "list the commands available",                                  handle_help,                1),
DEFINE_COMMAND(exit,            "exit the debugger",                                            handle_exit,                2),
DEFINE_COMMAND(continue,        "continue the program",                                         handle_continue,            3),
DEFINE_COMMAND(breakpoint,      "set breakpoint on the given line",                             handle_breakpoint_set,      4),
DEFINE_COMMAND(info,            "List all info commands",                                       handle_info,                5),
DEFINE_COMMAND(add_inferior,    "Adds a new inferior",                                          handle_add_inferior,        6),
DEFINE_COMMAND(disassemble,     "Display object file information for the current inferior.",    NULL,                       7),
DEFINE_COMMAND(run,             "Starts a program",                                             handle_run,                 8),
DEFINE_COMMAND(stepi,           "Single step instruction",                                      handle_step_inst,           9),
DEFINE_COMMAND(file,            "Add a executable to inferior",                                 handle_file,                10),
DEFINE_COMMAND(attach,          "Attach a process to inferior",                                 handle_attach,              11),
DEFINE_COMMAND(set,             "List all set commands",                                        handle_set,                 12),
DEFINE_COMMAND(disable,         "Disable a breakpoint",                                         handle_disable_breakpoint,  13),
DEFINE_COMMAND(enable,          "enable a breakpoint",                                          handle_enable_breakpoint,   14),
DEFINE_COMMAND(delete,          "deletes a breakpoint",                                         handle_delete_breakpoint,   15),
DEFINE_COMMAND(memory_read,     "read a memory location",                                       handle_read_memory,         16),
DEFINE_COMMAND(memory_write,    "read a memory location",                                       handle_write_memory,        16),