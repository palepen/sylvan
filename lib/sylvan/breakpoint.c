#include <assert.h>
#include <stddef.h>
#include <sys/ptrace.h>

#include <sylvan/breakpoint.h>
#include "sylvan.h"
#include "error.h"

#define isactive(inf) (inf->status == SYLVAN_INFSTATE_RUNNING || inf->status == SYLVAN_INFSTATE_STOPPED)

/**
 * finds the breakpoint which corresponds to addr, sets breakpointp if breakpointp is not NULL
 * return SYLVANC_OK if found else SYVLANC_BREAKPOINT_NOT_FOUND
 */
SYLVAN_INTERNAL sylvan_code_t
sylvan_breakpoint_find_by_addr(struct sylvan_inferior *inf, uintptr_t addr, struct sylvan_breakpoint **breakpointp) {

    assert(inf); // should have been checked by the caller

    int breakpoint_count = inf->breakpoint_count;
    struct sylvan_breakpoint *breakpoints = inf->breakpoints;
    for (int i = 0; i < breakpoint_count; ++i)
        if (breakpoints[i].addr == addr) {
            if (breakpointp)
                *breakpointp = breakpoints + i;
            return SYLVANC_OK;
        }

    return SYVLANC_BREAKPOINT_NOT_FOUND;
}

/**
 * creates a physical breakpoint (replaces the byte at breakpoint addr with 0xCC in the actual process)
 */
SYLVAN_INTERNAL sylvan_code_t
sylvan_breakpoint_create_phybp(struct sylvan_inferior *inf, struct sylvan_breakpoint *breakpoint) {

    assert(inf && breakpoint && isactive(inf)); // should have been checked by the caller

    if (breakpoint->is_enabled_phy)
        return SYLVANC_OK;

    long og_data = ptrace(PTRACE_PEEKTEXT, inf->pid, (void *) breakpoint->addr, NULL);
    if (og_data == -1)
        return sylvan_set_errno_msg(SYLVANC_PTRACE_PEEKTEXT_FAILED, "ptrace peek text");

    long new_data = (og_data & ~0xFF) | 0xCC;
    if (ptrace(PTRACE_POKETEXT, inf->pid, (void*) breakpoint->addr, (void *)new_data) == -1)
        return sylvan_set_errno_msg(SYLVANC_PTRACE_PEEKTEXT_FAILED, "ptrace poke text");

    breakpoint->og_byte = og_data & 0xFF;
    breakpoint->is_enabled_phy = true;

    return SYLVANC_OK;
}

/**
 * removes the physical breakpoint (replaces 0xCC with the original byte at the breakpoint addr in the actual process)
 */
SYLVAN_INTERNAL sylvan_code_t
sylvan_breakpoint_remove_phybp(struct sylvan_inferior *inf, struct sylvan_breakpoint *breakpoint) {

    assert(inf && breakpoint && isactive(inf)); // should have been checked by the caller

    if (!breakpoint->is_enabled_phy)
        return SYLVANC_OK;

    long data = ptrace(PTRACE_PEEKTEXT, inf->pid, (void *) breakpoint->addr, NULL);
    if (data == -1)
        return sylvan_set_errno_msg(SYLVANC_PTRACE_PEEKTEXT_FAILED, "ptrace peek text");

    data = (data & ~0xFF) | breakpoint->og_byte;

    if (ptrace(PTRACE_POKETEXT, inf->pid, (void *) breakpoint->addr, (void *)data) == -1)
        return sylvan_set_errno_msg(SYLVANC_PTRACE_POKETEXT_FAILED, "ptrace poke text");

    breakpoint->is_enabled_phy = false;

    return SYLVANC_OK;
}

SYLVAN_INTERNAL sylvan_code_t
sylvan_breakpoint_enable_ptr(struct sylvan_inferior *inf, struct sylvan_breakpoint *breakpoint) {

    assert(inf && breakpoint);

    breakpoint->is_enabled_log = true;

    if (isactive(inf))
        return sylvan_breakpoint_create_phybp(inf, breakpoint);

    return SYLVANC_OK;
}

SYLVAN_INTERNAL sylvan_code_t
sylvan_breakpoint_disable_ptr(struct sylvan_inferior *inf, struct sylvan_breakpoint *breakpoint) {

    assert(inf && breakpoint);

    breakpoint->is_enabled_log = false;

    if (isactive(inf))
        return sylvan_breakpoint_remove_phybp(inf, breakpoint);

    return SYLVANC_OK;
}

/**
 * enables the breakpoint at given addr
 */
sylvan_code_t sylvan_breakpoint_enable(struct sylvan_inferior *inf, uintptr_t addr) {
    if (!inf)
        return sylvan_set_code(SYLVANC_INVALID_ARGUMENT);

    struct sylvan_breakpoint *breakpoint;
    if (sylvan_breakpoint_find_by_addr(inf, addr, &breakpoint))
        return sylvan_set_code(SYVLANC_BREAKPOINT_NOT_FOUND);

    return sylvan_breakpoint_enable_ptr(inf, breakpoint);
}

/**
 * disables the breakpoint at given addr
 */
sylvan_code_t sylvan_breakpoint_disable(struct sylvan_inferior *inf, uintptr_t addr) {
    if (!inf)
        return sylvan_set_code(SYLVANC_INVALID_ARGUMENT);

    struct sylvan_breakpoint *breakpoint;
    if (sylvan_breakpoint_find_by_addr(inf, addr, &breakpoint))
        return sylvan_set_code(SYVLANC_BREAKPOINT_NOT_FOUND);

    return sylvan_breakpoint_disable_ptr(inf, breakpoint);
}

/**
 * sets a breakpoint and enables it
 */
sylvan_code_t sylvan_breakpoint_set(struct sylvan_inferior *inf, uintptr_t addr) {
    if (!inf)
        return sylvan_set_code(SYLVANC_INVALID_ARGUMENT);

    int breakpoint_count = inf->breakpoint_count;
    if (breakpoint_count == MAX_BREAKPOINTS)
        return sylvan_set_code(SYVLANC_BREAKPOINT_LIMIT_REACHED);

    if (sylvan_breakpoint_find_by_addr(inf, addr, NULL) != SYVLANC_BREAKPOINT_NOT_FOUND)
        return sylvan_set_code(SYVLANC_BREAKPOINT_ALREADY_EXISTS);

    struct sylvan_breakpoint *breakpoint = inf->breakpoints + inf->breakpoint_count++;
    breakpoint->addr = addr;
    breakpoint->is_enabled_log = true;

    if (!isactive(inf))
        return SYLVANC_OK;

    return sylvan_breakpoint_enable_ptr(inf, breakpoint);
}

/**
 * disables breakpoint and unsets it
 */
sylvan_code_t sylvan_breakpoint_unset(struct sylvan_inferior *inf, uintptr_t addr) {
    if (!inf)
        return sylvan_set_code(SYLVANC_INVALID_ARGUMENT);

    struct sylvan_breakpoint *breakpoint;
    if (sylvan_breakpoint_find_by_addr(inf, addr, &breakpoint))
        return sylvan_set_code(SYVLANC_BREAKPOINT_NOT_FOUND);

    sylvan_code_t code;
    if ((code = sylvan_breakpoint_disable_ptr(inf, breakpoint)))
        return code;

    struct sylvan_breakpoint *breakpoints = inf->breakpoints;

    // breakpoints[i] = breakpoints[n - 1]
    if (--inf->breakpoint_count)
        breakpoints[breakpoint - breakpoints] = breakpoints[inf->breakpoint_count];

    return SYLVANC_OK;
}

SYLVAN_INTERNAL sylvan_code_t
sylvan_breakpoint_clearall(struct sylvan_inferior *inf) {
    if (!inf)
        return sylvan_set_code(SYLVANC_INVALID_ARGUMENT);

    if (!isactive(inf)) {
        inf->breakpoint_count = 0;
        return SYLVANC_OK;
    }
    sylvan_code_t code;
    struct sylvan_breakpoint *breakpoints = inf->breakpoints;
    while (inf->breakpoint_count) {
        if ((code = sylvan_breakpoint_remove_phybp(inf, breakpoints + inf->breakpoint_count - 1)))
            return code;
        inf->breakpoint_count--;
    }

    return SYLVANC_OK;
}

SYLVAN_INTERNAL sylvan_code_t
sylvan_breakpoint_reset_phybp(struct sylvan_inferior *inf) {
    if (!inf)
        return sylvan_set_code(SYLVANC_INVALID_ARGUMENT);

    struct sylvan_breakpoint *breakpoints = inf->breakpoints;

    int breakpoint_count = inf->breakpoint_count;
    for (int i = 0; i < breakpoint_count; ++i)
        breakpoints[i].is_enabled_phy = false;

    return SYLVANC_OK;
}

SYLVAN_INTERNAL sylvan_code_t
sylvan_breakpoint_setall_phybp(struct sylvan_inferior *inf) {

    assert(inf && isactive(inf));// should have been checked by the caller

    sylvan_code_t code;
    int breakpoint_count = inf->breakpoint_count;
    struct sylvan_breakpoint *breakpoints = inf->breakpoints;
    for (int i = 0; i < breakpoint_count; ++i)
        if ((code = sylvan_breakpoint_create_phybp(inf, breakpoints + i)))
            return code;

    return SYLVANC_OK;
}

SYLVAN_INTERNAL sylvan_code_t
sylvan_breakpoint_unsetall_phybp(struct sylvan_inferior *inf) {
    
    assert(inf && isactive(inf));// should have been checked by the caller

    sylvan_code_t code;
    int breakpoint_count = inf->breakpoint_count;
    struct sylvan_breakpoint *breakpoints = inf->breakpoints;
    for (int i = 0; i < breakpoint_count; ++i)
        if ((code = sylvan_breakpoint_remove_phybp(inf, breakpoints + i)))
            return code;

    return SYLVANC_OK;
}
