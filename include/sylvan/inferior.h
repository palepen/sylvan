#ifndef SYLVAN_INC_INFERIOR_H
#define SYLVAN_INC_INFERIOR_H

#include <sylvan/error.h>
#include <stdbool.h>
#include <sys/types.h>

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
};

sylvan_code_t sylvan_inferior_create(struct sylvan_inferior **inf);
sylvan_code_t sylvan_inferior_destroy(struct sylvan_inferior *inf);

sylvan_code_t sylvan_detach(struct sylvan_inferior *inf);
sylvan_code_t sylvan_kill(struct sylvan_inferior *inf);
sylvan_code_t sylvan_attach_pid(struct sylvan_inferior *inf, pid_t pid);
sylvan_code_t sylvan_run(struct sylvan_inferior *inf);
sylvan_code_t sylvan_set_filepath(struct sylvan_inferior *inf, const char *filepath);
sylvan_code_t sylvan_set_args(struct sylvan_inferior *inf, const char *args);
sylvan_code_t sylvan_continue(struct sylvan_inferior *inf);

#endif /* SYLVAN_INFERIOR_H */
