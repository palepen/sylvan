#include <assert.h>
#include <stddef.h>
#include <sys/ptrace.h>

#include <sylvan/breakpoint.h>
#include "error.h"

/**
 * finds the breakpoint which corresponds to addr, sets breakpointp if breakpointp is not NULL
 * return SYLVANC_OK if found else SYVLANC_BREAKPOINT_NOT_FOUND
 */
sylvan_code_t sylvan_breakpoint_find_by_addr(struct sylvan_inferior *inf, uintptr_t addr, struct sylvan_breakpoint **breakpointp) {
    assert(inf != NULL);            /* inf cannot be NULL in an internal function */

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
 * enables breakpoint given breakpoint ptr
 */
sylvan_code_t sylvan_breakpoint_enable_bp(struct sylvan_inferior *inf, struct sylvan_breakpoint *breakpoint) {
    if (!inf || !breakpoint)
        return sylvan_set_code(SYLVANC_INVALID_ARGUMENT);

    long og_data = ptrace(PTRACE_PEEKTEXT, inf->pid, (void *) breakpoint->addr, NULL);
    if (og_data == -1)
        return sylvan_set_errno_msg(SYLVANC_PTRACE_PEEKTEXT_FAILED, "ptrace peek text");

    long new_data = (og_data & ~0xFF) | 0xCC;
    if (ptrace(PTRACE_POKETEXT, inf->pid, (void*) breakpoint->addr, (void *)new_data) == -1)
        return sylvan_set_errno_msg(SYLVANC_PTRACE_PEEKTEXT_FAILED, "ptrace poke text");

    breakpoint->og_byte = og_data & 0xFF;
    breakpoint->is_enabled = true;

    return SYLVANC_OK;
}

/**
 * disables breakpoint given breakpoint ptr
 */
sylvan_code_t sylvan_breakpoint_disable_bp(struct sylvan_inferior *inf, struct sylvan_breakpoint *breakpoint) {
    if (!inf || !breakpoint)
        return sylvan_set_code(SYLVANC_INVALID_ARGUMENT);

    if (!breakpoint->is_enabled)
        return SYLVANC_OK;

    long data = ptrace(PTRACE_PEEKTEXT, inf->pid, (void *) breakpoint->addr, NULL);
    if (data == -1)
        return sylvan_set_errno_msg(SYLVANC_PTRACE_PEEKTEXT_FAILED, "ptrace peek text");

    data = (data & ~0xFF) | breakpoint->og_byte;

    if (ptrace(PTRACE_POKETEXT, inf->pid, (void *) breakpoint->addr, (void *)data) == -1)
        return sylvan_set_errno_msg(SYLVANC_PTRACE_POKETEXT_FAILED, "ptrace poke text");

    breakpoint->is_enabled = false;

    return SYLVANC_OK;
}

/**
 * enables breakpoint given breakpoint addr
 */
sylvan_code_t sylvan_breakpoint_enable_addr(struct sylvan_inferior *inf, uintptr_t addr) {
    if (!inf)
        return sylvan_set_code(SYLVANC_INVALID_ARGUMENT);

    struct sylvan_breakpoint *breakpoint;
    if (sylvan_breakpoint_find_by_addr(inf, addr, &breakpoint))
        return sylvan_set_code(SYVLANC_BREAKPOINT_NOT_FOUND);

    return sylvan_breakpoint_enable_bp(inf , breakpoint);
}

/**
 * disables breakpoint given breakpoint addr
 */
sylvan_code_t sylvan_breakpoint_disable_addr(struct sylvan_inferior *inf, uintptr_t addr) {
    if (!inf)
        return sylvan_set_code(SYLVANC_INVALID_ARGUMENT);

    struct sylvan_breakpoint *breakpoint;
    if (sylvan_breakpoint_find_by_addr(inf, addr, &breakpoint))
        return sylvan_set_code(SYVLANC_BREAKPOINT_NOT_FOUND);

    return sylvan_breakpoint_disable_bp(inf , breakpoint);
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
    breakpoint->is_enabled = true;

    return SYLVANC_OK;
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
    if ((code = sylvan_breakpoint_disable_bp(inf, breakpoint)))
        return code;

    struct sylvan_breakpoint *breakpoints = inf->breakpoints;

    // breakpoints[i] = breakpoints[n - 1]
    if (--inf->breakpoint_count)
        breakpoints[breakpoint - breakpoints] = breakpoints[inf->breakpoint_count];

    return SYLVANC_OK;
}
