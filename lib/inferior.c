#include <ctype.h>
#include <errno.h>
#include <stdlib.h>
#include <signal.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <limits.h>
#include <sys/ptrace.h>
#include <sys/wait.h>
#include <sylvan/inferior.h>
#include "error.h"

static int sylvan_inferior_idx = 0;
static int sylvan_inferior_count = 0;


/**
 * @brief creates an inferior
 */
sylvan_code_t sylvan_inferior_create(struct sylvan_inferior **inf) {
    *inf = (malloc(sizeof(struct sylvan_inferior)));
    if (*inf == NULL)
        return sylvan_set_code(SYLVANE_OUT_OF_MEMORY);
    memset(*inf, 0, sizeof(struct sylvan_inferior));
    (*inf)->id = sylvan_inferior_idx++;
    sylvan_inferior_count++;
    return SYLVANE_OK;
}

/**
 * @brief detaches from a process
 */
sylvan_code_t sylvan_detach(struct sylvan_inferior *inf) {
    if (inf == NULL)
        return sylvan_set_code(SYLVANE_INVALID_ARGUMENT);
    if (!inf->is_attached)
        return sylvan_set_code(SYLVANE_PROC_NOT_ATTACHED);
    if (ptrace(PTRACE_DETACH, inf->pid, NULL, NULL) < 0)
        return sylvan_set_errno_msg(SYLVANE_PTRACE_DETACH, "ptrace detach");
    inf->is_attached = false;
    inf->pid = -1;
    inf->status = SYLVAN_INFSTATE_NONE;
    return SYLVANE_OK;
}

/**
 * @brief kills a process
 */
sylvan_code_t sylvan_kill(struct sylvan_inferior *inf) {
    if (inf == NULL)
        return sylvan_set_code(SYLVANE_INVALID_ARGUMENT);
    if (inf->status != SYLVAN_INFSTATE_RUNNING && inf->status != SYLVAN_INFSTATE_STOPPED)
        return SYLVANE_OK;
    if (kill(inf->pid, SIGKILL) && errno != ESRCH)
        return sylvan_set_errno_msg(SYLVANE_PROC_KILL, "kill");
    inf->is_attached = false;
    inf->pid = -1;
    inf->status = SYLVAN_INFSTATE_NONE;
    return SYLVANE_OK;
}

/**
 * function to reset the status of the inf in between process changes
 * detaches or kills the process depending on inf->is_attached
 * only fields related to process are reset. args for eg, is not reset
 */
static sylvan_code_t reset_inferior_state(struct sylvan_inferior *inf) {
    if (inf == NULL)
        return sylvan_set_code(SYLVANE_INVALID_ARGUMENT);
    sylvan_code_t code = SYLVANE_OK;
    if (inf->status == SYLVAN_INFSTATE_RUNNING || inf->status == SYLVAN_INFSTATE_STOPPED) {
        if (inf->is_attached) {
            if ((code = sylvan_detach(inf)))
                return code;
        } else {
            if ((code = sylvan_kill(inf)))
                return code;
        }
    }
    return code;
}


/**
 * @brief finds the real (absolute) path of a file given its path
 * @param filepath file path
 * @return a malloced string of real path or NULL in case of failure
 */
static char *get_realpath_file(const char *filepath) {
    return realpath(filepath, NULL);
}

/**
 * @brief finds the real (absolute) path of the process's executable given its pid
 * @param pid process id
 * @return a malloced string of real path or NULL in case of failure
 */
static char *get_realpath_pid(pid_t pid) {
    char exepath[64];
    sprintf(exepath, "/proc/%d/exe", pid);
    return get_realpath_file(exepath);
}

/**
 * @brief attaches to a process given its pid
 */
sylvan_code_t sylvan_attach_pid(struct sylvan_inferior *inf, pid_t pid) {
    if (inf == NULL)
        return sylvan_set_code(SYLVANE_INVALID_ARGUMENT);
    sylvan_code_t code = SYLVANE_OK;
    if ((code = reset_inferior_state(inf)))
        return code;
    if (ptrace(PTRACE_ATTACH, pid, NULL, NULL) < 0)
        return sylvan_set_errno_msg(SYLVANE_PTRACE_ATTACH, "ptrace attach");
    int status;
    if (waitpid(pid, &status, 0) < 0)
        return sylvan_set_errno_msg(SYLVANE_PROC_ATTACH, "waitpid");
    if (!WIFSTOPPED(status))
        return sylvan_set_message(SYLVANE_PROC_ATTACH, "The process did not stop");
    inf->pid = pid;
    inf->status = SYLVAN_INFSTATE_STOPPED;
    free(inf->realpath);
    // it's okay for the real path to be null since this is an attached process
    inf->realpath = get_realpath_pid(inf->pid);
    inf->is_attached = true;
    return code;
}

/**
 * @brief deletes an inferior
 */
sylvan_code_t sylvan_inferior_destroy(struct sylvan_inferior *inf) {
    if (inf == NULL)
        return sylvan_set_code(SYLVANE_INVALID_ARGUMENT);
    sylvan_code_t code;
    if ((code = reset_inferior_state(inf)))
        return code;
    free(inf->realpath);
    free(inf->args);
    free(inf);
    sylvan_inferior_count--;
    return SYLVANE_OK;
}

static void handle_child(struct sylvan_inferior *inf) {
    
    // TODO: send the errno to parent + terminal handling
    if (ptrace(PTRACE_TRACEME, 0, NULL, NULL) < 0)
        _exit(EXIT_FAILURE);
    
    char **argv = NULL;
    int argc = 0;
    
    argv = malloc(sizeof(char *) * (argc + 2));
    if (argv == NULL)
        _exit(EXIT_FAILURE);
    
    argv[argc++] = strdup(inf->realpath);
    // TODO: do a single malloc at start and a single realloc at the end
    if (inf->args && *inf->args) {
        const char *p = inf->args;
        char *arg_buffer = malloc(strlen(inf->args) + 1);
        if (arg_buffer == NULL) {
            free(argv[0]);
            free(argv);
            _exit(EXIT_FAILURE);
        }
        
        while (*p) {
            while (*p && isspace(*p))
                p++;
            if (!*p)
                break;
            
            char *dest = arg_buffer;
            int double_quotes = 0;
            int single_quotes = 0;
            
            while (*p) {
                if (*p == '\\' && *(p + 1)) {
                    *dest++ = *(p + 1);
                    p += 2;
                } else if (*p == '"' && !single_quotes) {
                    double_quotes = !double_quotes;
                    p++;
                } else if (*p == '\'' && !double_quotes) {
                    single_quotes = !single_quotes;
                    p++;
                } else if (isspace(*p) && !double_quotes && !single_quotes) {
                    break;
                } else {
                    *dest++ = *p++;
                }
            }
            
            *dest = '\0';
            
            argv = realloc(argv, sizeof(char *) * (argc + 2));
            if (argv == NULL) {
                free(arg_buffer);
                for (int i = 0; i < argc; i++)
                    free(argv[i]);
                free(argv);
                _exit(EXIT_FAILURE);
            }
            
            argv[argc++] = strdup(arg_buffer);
        }
        
        free(arg_buffer);
    }
    
    argv[argc] = NULL;
    
    execvp(inf->realpath, argv);

    for (int i = 0; i < argc; i++)
        free(argv[i]);
    free(argv);
    
    _exit(EXIT_FAILURE);
}

static sylvan_code_t handle_parent(pid_t pid, struct sylvan_inferior *inf) {
    inf->pid = pid;
    inf->status = SYLVAN_INFSTATE_RUNNING;
    int status;

    if (waitpid(pid, &status, 0) < 0)
        return sylvan_set_errno_msg(SYLVANE_PROC_WAIT, "waitpid");

    if (WIFEXITED(status))
        inf->status = SYLVAN_INFSTATE_EXITED;   // _exit()
    else
    if (WIFSTOPPED(status))                     // kill -TERM and -STOP
        inf->status = SYLVAN_INFSTATE_STOPPED;
    else
    if (WIFSIGNALED(status))                    // kill -KILL
        inf->status = SYLVAN_INFSTATE_TERMINATED;

    return SYLVANE_OK;
}

sylvan_code_t sylvan_run(struct sylvan_inferior *inf) {
    if (inf == NULL)
        return sylvan_set_code(SYLVANE_INVALID_ARGUMENT);
    if (inf->realpath == NULL)
        return sylvan_set_code(SYLVANE_NOEXEC);
    sylvan_code_t code = SYLVANE_OK;
    if ((code = sylvan_kill(inf)))
        return code;
    pid_t pid = fork();
    if (pid < 0)
        return sylvan_set_errno_msg(SYLVANE_PROC_FORK, "fork");
    if (pid == 0)
        handle_child(inf); // this won't return
    return handle_parent(pid, inf);
}

/**
 * @param filepath path to the executable; can't be null
 */
sylvan_code_t sylvan_set_filepath(struct sylvan_inferior *inf, const char *filepath) {
    if (inf == NULL)
        return sylvan_set_code(SYLVANE_INVALID_ARGUMENT);
    if (filepath == NULL)
        return sylvan_set_code(SYLVANE_INVALID_ARGUMENT);
    char *newpath = get_realpath_file(filepath);
    if (newpath == NULL)
        return sylvan_set_code(SYLVANE_NOEXEC);
    free(inf->realpath);
    inf->realpath = newpath;
    return SYLVANE_OK;
}

/**
 * @param args arguments given to the executable; set it to null to remove any
 */
sylvan_code_t sylvan_set_args(struct sylvan_inferior *inf, const char *args) {
    if (inf == NULL)
        return sylvan_set_code(SYLVANE_INVALID_ARGUMENT);
    if (args == NULL) {
        free(inf->args);
        inf->args = NULL;
        return SYLVANE_OK;
    }
    char *newargs = strdup(args);
    if (newargs == NULL)
        return sylvan_set_code(SYLVANE_OUT_OF_MEMORY);
    free(inf->args);
    inf->args = newargs;
    return SYLVANE_OK;
}

static sylvan_code_t check_inf_stop(struct sylvan_inferior *inf) {
    switch (inf->status) {
        case SYLVAN_INFSTATE_NONE:
            return sylvan_set_message(SYLVANE_INF_INVALID_STATE, "Program isn't running");
        case SYLVAN_INFSTATE_EXITED:
            return sylvan_set_message(SYLVANE_INF_INVALID_STATE, "Program has already exited");
        case SYLVAN_INFSTATE_TERMINATED:
            return sylvan_set_message(SYLVANE_INF_INVALID_STATE, "Program has been terminated");
        case SYLVAN_INFSTATE_RUNNING:
            return sylvan_set_message(SYLVANE_INF_INVALID_STATE, "Program is already running");
        default:
            return SYLVANE_OK;
    }
}

static void ptrace_error(struct sylvan_inferior *inf) {
    switch (errno) {
        case ESRCH:
            inf->is_attached = false;
            inf->status = SYLVAN_INFSTATE_EXITED;
            break;
    }
}

sylvan_code_t sylvan_continue(struct sylvan_inferior *inf) {
    if (inf == NULL)
        return sylvan_set_code(SYLVANE_INVALID_ARGUMENT);
    sylvan_code_t code = check_inf_stop(inf);
    if (code)
        return code;
    if (ptrace(PTRACE_CONT, inf->pid, NULL, NULL) < 0) {
        ptrace_error(inf);
        return sylvan_set_errno_msg(SYLVANE_PTRACE_CONT, "ptrace cont");
    }
    inf->status = SYLVAN_INFSTATE_RUNNING;
    int status;
    pid_t result = waitpid(inf->pid, &status, 0);
    if (result < 0)
        return sylvan_set_errno_msg(SYLVANE_PROC_WAIT, "waitpid");

    if (WIFEXITED(status))
        inf->status = SYLVAN_INFSTATE_EXITED;
    else
    if (WIFSIGNALED(status))
        inf->status = SYLVAN_INFSTATE_TERMINATED;
    else
    if (WIFSTOPPED(status))
        inf->status = SYLVAN_INFSTATE_STOPPED;
    return SYLVANE_OK;
}

sylvan_code_t sylvan_stepinst(struct sylvan_inferior *inf) {
    if (inf == NULL)
        return sylvan_set_code(SYLVANE_INVALID_ARGUMENT);
    sylvan_code_t code = check_inf_stop(inf);
    if (code)
        return code;
    if (ptrace(PTRACE_SINGLESTEP, inf->pid, NULL, NULL) < 0) {
        ptrace_error(inf);
        return sylvan_set_errno_msg(SYLVANE_PTRACE_SSTEP, "ptrace single step");
    }
    return SYLVANE_OK;
}
