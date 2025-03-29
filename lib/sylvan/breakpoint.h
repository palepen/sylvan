#ifndef SYLVAN_BREAKPOINT_H
#define SYLVAN_BREAKPOINT_H

#include <sylvan/breakpoint.h>

sylvan_code_t sylvan_breakpoint_find_by_addr(struct sylvan_inferior *inf, uintptr_t addr, struct sylvan_breakpoint **breakpointp);

sylvan_code_t sylvan_breakpoint_enable_bp(struct sylvan_inferior *inf, struct sylvan_breakpoint *breakpoint);
sylvan_code_t sylvan_breakpoint_disable_bp(struct sylvan_inferior *inf, struct sylvan_breakpoint *breakpoint);

#endif /* SYLVAN_BREAKPOINT_H */
