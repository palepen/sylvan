#ifndef SYLVAN_INCLUDE_SYLVAN_INFERIOR_H
#define SYLVAN_INCLUDE_SYLVAN_INFERIOR_H

#include <sylvan/breakpoint.h>
#include <sylvan/error.h>
#include <stdbool.h>
#include <sys/types.h>
#include <sys/user.h>

typedef enum {
    SYLVAN_INFSTATE_NONE,
    SYLVAN_INFSTATE_TERMINATED,
    SYLVAN_INFSTATE_RUNNING,
    SYLVAN_INFSTATE_EXITED,
    SYLVAN_INFSTATE_STOPPED,
} sylvan_inferior_state_t;


struct sylvan_inferior {
    int id;
    pid_t pid;
    sylvan_inferior_state_t status;
    char *realpath;
    char *args;
    bool is_attached;

    struct sylvan_breakpoint breakpoints[MAX_BREAKPOINTS];
    int breakpoint_count;
};

sylvan_code_t sylvan_inferior_create(struct sylvan_inferior **inf);
sylvan_code_t sylvan_inferior_destroy(struct sylvan_inferior *inf);

sylvan_code_t sylvan_attach(struct sylvan_inferior *inf, pid_t pid);
sylvan_code_t sylvan_detach(struct sylvan_inferior *inf);

sylvan_code_t sylvan_run(struct sylvan_inferior *inf);
sylvan_code_t sylvan_continue(struct sylvan_inferior *inf);
sylvan_code_t sylvan_stepinst(struct sylvan_inferior *inf);

sylvan_code_t sylvan_get_regs(struct sylvan_inferior *inf, struct user_regs_struct *regs);
sylvan_code_t sylvan_set_regs(struct sylvan_inferior *inf, const struct user_regs_struct *regs);

sylvan_code_t sylvan_set_filepath(struct sylvan_inferior *inf, const char *filepath);
sylvan_code_t sylvan_set_args(struct sylvan_inferior *inf, const char *args);

sylvan_code_t sylvan_get_memory(struct sylvan_inferior *inf, uintptr_t addr, uint64_t *data);
sylvan_code_t sylvan_set_memory(struct sylvan_inferior *inf, uintptr_t addr, const void *data, size_t size);

#endif /* SYLVAN_INCLUDE_SYLVAN_INFERIOR_H */
