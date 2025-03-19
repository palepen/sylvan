#ifndef SYLVAN_INC_ERROR_H
#define SYLVAN_INC_ERROR_H

typedef enum {
    SYLVANE_OK = 0,
    SYLVANE_INVALID_ARGUMENT,
    SYLVANE_OUT_OF_MEMORY,
    SYLVANE_FILEPATH,
    SYLVANE_NOEXEC,
    SYLVANE_NOFILE,
    SYLVANE_INF_INVALID_STATE,
    SYLVANE_PTRACE_ATTACH,
    SYLVANE_PTRACE_CONT,
    SYLVANE_PTRACE_DETACH,
    SYLVANE_PTRACE_GETREGS,
    SYLVANE_PTRACE_SETREGS,
    SYLVANE_PTRACE_SSTEP,
    SYLVANE_PTRACE_ALREADY_EXITED,
    SYLVANE_PTRACE_NOT_STOPPED,
    SYLVANE_PROC_ATTACH,
    SYLVANE_PROC_FORK,
    SYLVANE_PROC_WAIT,                  /* not sure about this */
    SYLVANE_PROC_KILL,
    SYLVANE_PROC_NOT_ATTACHED,
    SYLVANE_COUNT,                      /* not an error, this is the total no of error codes */
} sylvan_code_t;

struct sylvan_error_context {
    int code;
    int os_errno;
    const char *message;
};

const char *sylvan_get_last_error();

#endif /* SYLVAN_ERROR_H */