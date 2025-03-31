#ifndef SYLVAN_INCLUDE_BREAKPOINT_H
#define SYLVAN_INCLUDE_BREAKPOINT_H

#include <stdint.h>
#include <sylvan/error.h>
#include <stdbool.h>

#define MAX_BREAKPOINTS 256

struct sylvan_breakpoint {
    uintptr_t addr;
    uint8_t og_byte;
    bool is_enabled_log;
    bool is_enabled_phy;
};

struct sylvan_inferior;

sylvan_code_t sylvan_breakpoint_enable(struct sylvan_inferior *inf, uintptr_t addr);
sylvan_code_t sylvan_breakpoint_disable(struct sylvan_inferior *inf, uintptr_t addr);

sylvan_code_t sylvan_breakpoint_set(struct sylvan_inferior *inf, uintptr_t addr);
sylvan_code_t sylvan_breakpoint_unset(struct sylvan_inferior *inf, uintptr_t addr);


#endif /* SYLVAN_INCLUDE_BREAKPOINT_H */
