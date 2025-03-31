#ifndef SYLVAN_BREAKPOINT_H
#define SYLVAN_BREAKPOINT_H

#include <sylvan/breakpoint.h>

sylvan_code_t sylvan_breakpoint_find_by_addr(struct sylvan_inferior *inf, uintptr_t addr, struct sylvan_breakpoint **breakpointp);

sylvan_code_t sylvan_breakpoint_enable_ptr(struct sylvan_inferior *inf, struct sylvan_breakpoint *breakpoint);
sylvan_code_t sylvan_breakpoint_disable_ptr(struct sylvan_inferior *inf, struct sylvan_breakpoint *breakpoint);

sylvan_code_t sylvan_breakpoint_clearall(struct sylvan_inferior *inf);
sylvan_code_t sylvan_breakpoint_reset_phybp(struct sylvan_inferior *inf);

sylvan_code_t sylvan_breakpoint_setall_phybp(struct sylvan_inferior *inf);
sylvan_code_t sylvan_breakpoint_unsetall_phybp(struct sylvan_inferior *inf);

#endif /* SYLVAN_BREAKPOINT_H */
