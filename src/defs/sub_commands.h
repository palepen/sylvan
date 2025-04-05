#ifndef DEFINE_COMMAND
#error "This file is intended for textual inclusion with the DEFINE_COMMAND macro defined"
#endif

DEFINE_COMMAND(info_address,        "Describe where symbol SYM is stored.",                                             handle_info_address,        101, 1),
DEFINE_COMMAND(info_registers,      "List of all registers and their contents.",                                        handle_info_registers,      102, 1),
DEFINE_COMMAND(info_args,           "All argument variables of current stack frame or those matching REGEXPs.",         handle_info_args,           103, 1),
DEFINE_COMMAND(info_bookmarks,      "Status of user-settable bookmarks.",                                               handle_info_bookmark,       104, 1),
DEFINE_COMMAND(info_breakpoints,    "Status of specified breakpoints (all user-settable breakpoints if no argument).",  handle_info_breakpoints,    105, 1),
DEFINE_COMMAND(info_copying,        "Conditions for redistributing copies of Sylvan.",                                  handle_info_copying,        106, 1),
DEFINE_COMMAND(info_inferiors,      "Print a list of inferiors being managed.",                                         handle_info_inferiors,      107, 1),
DEFINE_COMMAND(info_alias,          "Print all the alises present.",                                                    handle_info_alias,          108, 1),
DEFINE_COMMAND(set_args,            "Set arguments",                                                                    handle_set_args,            201, 2),
DEFINE_COMMAND(set_reg,             "Set a value to register",                                                          handle_set_reg,             202, 2),
DEFINE_COMMAND(set_alias,           "add alias for a command",                                                          handle_set_alias,           203, 2),
