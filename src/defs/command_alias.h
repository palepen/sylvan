#ifndef DEFINE_ALIAS
#error "This file is intended for textual inclusion with the DEFINE_COMMAND macro defined"
#endif

DEFINE_ALIAS("h",         "help",               1),  
DEFINE_ALIAS("e",         "exit",               2),  
DEFINE_ALIAS("c",         "continue",           3),  
DEFINE_ALIAS("b",         "breakpoint",         4),  
DEFINE_ALIAS("i",         "info",               5),  
DEFINE_ALIAS("add_inf",   "add_inferior",       6),  
DEFINE_ALIAS("disas",     "disassemble",        7),  
DEFINE_ALIAS("r",         "run",                8),  
DEFINE_ALIAS("si",        "stepi" ,             9),  
DEFINE_ALIAS("fi",        "file" ,              10),  
DEFINE_ALIAS("at",        "attach",             11),  
DEFINE_ALIAS("st",        "set",                12),  
DEFINE_ALIAS("b_dbl",     "disable",            13),  
DEFINE_ALIAS("b_enb",     "enable",             14),  
DEFINE_ALIAS("b_dlt",     "delete",             15),  
DEFINE_ALIAS("i_adr",     "info_address",       16),  
DEFINE_ALIAS("i_reg",     "info_registers", 17),  
DEFINE_ALIAS("i_auxv",    "info_auxv",          18),  
DEFINE_ALIAS("i_bkpts",   "info_breakpoints",   19),  
DEFINE_ALIAS("i_inf",     "info_inferiors",     20),  
DEFINE_ALIAS("s_args",    "set_args",           21),  
DEFINE_ALIAS("s_reg",     "set_reg",            22),