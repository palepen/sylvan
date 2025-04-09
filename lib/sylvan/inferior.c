#include <assert.h>
#include <ctype.h>
#include <errno.h>
#include <stdlib.h>
#include <signal.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <limits.h>
#include <sys/ptrace.h>
#include <sys/user.h>
#include <sys/wait.h>
#include <wordexp.h>
#include <sys/personality.h>

#include <sylvan/inferior.h>
#include "breakpoint.h"
#include "error.h"
#include "utils.h"
#include "symbol.h"

static int sylvan_inferior_idx = 0;
static int sylvan_inferior_count = 0;


/**
 * checks for a change in the state of the process and updates the inferior state
 */
static sylvan_code_t sylvan_update_inf_status(struct sylvan_inferior *inf, int *status, bool blocking) {

    assert(inf != NULL); /* inf should not be NULL in an internal library function */

    if (inf->pid <= 0)
        return sylvan_set_message(SYLVANC_INVALID_STATE, "Program is not being run");

    int result, status_;
    do {
        result = waitpid(inf->pid, &status_, blocking ? 0 : WNOHANG);
    } while (result == -1 && errno == EINTR);

    if (!result)   /* no change in state */
        return SYLVANC_OK;

    if (result == -1) {
        if (errno != ECHILD)
            return sylvan_set_errno_msg(SYLVANC_WAITPID_FAILED, "waitpid");

        if (kill(inf->pid, 0) != -1)
            return sylvan_set_message(SYLVANC_PROC_NOT_ATTACHED, "Process %d exists but is not being traced", inf->pid);

        if (errno != ESRCH)
            return sylvan_set_errno_msg(SYLVANC_SYSTEM_ERROR, "Can't check process status");

        inf->status = SYLVAN_INFSTATE_NONE;
        inf->pid = 0;
        inf->is_attached = false;
        return sylvan_set_message(SYLVANC_PROC_NOT_FOUND, "Process %d doesn't exist", inf->pid);
    }

    /* there's a status change */
    if (WIFEXITED(status_)) {
        int pid = inf->pid;
        inf->status = SYLVAN_INFSTATE_EXITED;
        inf->pid = 0;
        return sylvan_set_message(SYLVANC_PROC_EXITED, "Process %d exited with code %d", pid, WEXITSTATUS(status_));
    }
    if (WIFSIGNALED(status_)) {
        inf->status = SYLVAN_INFSTATE_TERMINATED;
        inf->pid = 0;
        return sylvan_set_message(SYLVANC_PROC_TERMINATED, "Process %d terminated by signal %d", inf->pid, WTERMSIG(status_));
    }
    if (WIFSTOPPED(status_)) {
        // temporary fix to get it to return the breakpoint addr
        inf->status = SYLVAN_INFSTATE_STOPPED;
        if (blocking) {
            siginfo_t info;
            if (ptrace(PTRACE_GETSIGINFO, inf->pid, NULL, &info) < 0)
                return sylvan_set_errno_msg(SYLVANC_PTRACE_ERROR, "ptrace get siginfo");

            struct user_regs_struct regs;
            if (ptrace(PTRACE_GETREGS, inf->pid, NULL, &regs) < 0)
                return sylvan_set_errno_msg(SYLVANC_PTRACE_GETREGS_FAILED, "ptrace get regs");

            if (info.si_code != SI_KERNEL)
                return sylvan_set_message(SYLVANC_PROC_STOPPED, "program stopped at %#lx", regs.rip);
            
            struct sylvan_breakpoint *breakpoint;
            if (sylvan_breakpoint_find_by_addr(inf, regs.rip - 1, &breakpoint))
                return SYLVANC_OK;

            int idx = breakpoint - inf->breakpoints;
            return sylvan_set_message(SYVLANC_BREAKPOINT_HIT, "breakpoint %d at %#lx", idx, breakpoint->addr);
        }
        return SYLVANC_OK;
    }
    if (WIFCONTINUED(status_))
        inf->status = SYLVAN_INFSTATE_RUNNING;
    else
        assert(0); /* this shouldn't happen */

    if (status)
        *status = status_;

    return SYLVANC_OK;

}

/**
 * updates inferior status based on wait status
 */
static void sylvan_update_wait_status(int status, struct sylvan_inferior *inf) {

    assert(inf != NULL); /* inf should not be NULL in an internal library function */

    if (WIFSTOPPED(status))
        inf->status = SYLVAN_INFSTATE_STOPPED;
    else
    if (WIFEXITED(status))
        inf->status = SYLVAN_INFSTATE_EXITED;
    else
    if (WIFSIGNALED(status))
        inf->status = SYLVAN_INFSTATE_TERMINATED;
    else
    if (WIFCONTINUED(status))
        inf->status = SYLVAN_INFSTATE_RUNNING;
    else
        assert(0); /* this shouldn't happen */
}

/**
 * kills the associated process
*/
static sylvan_code_t sylvan_kill(struct sylvan_inferior *inf) {
    if (inf == NULL)
        return sylvan_set_code(SYLVANC_INVALID_ARGUMENT);
    
    if (inf->status != SYLVAN_INFSTATE_RUNNING && inf->status != SYLVAN_INFSTATE_STOPPED)
        return SYLVANC_OK;

    if (kill(inf->pid, SIGKILL) < 0) {
        if (errno != ESRCH)
            return sylvan_set_errno_msg(SYLVANC_KILL_FAILED, "kill");
        inf->is_attached = false;
        inf->pid = 0;
        inf->status = SYLVAN_INFSTATE_NONE;
        return SYLVANC_OK;
    }
    
    int status;
    int result;
    do {
        result = waitpid(inf->pid, &status, 0);
    } while (result == -1 && errno == EINTR);
    
    if (result == -1)
        return sylvan_set_errno_msg(SYLVANC_WAITPID_FAILED, "waitpid");

    inf->is_attached = false;
    inf->pid = 0;
    inf->status = SYLVAN_INFSTATE_NONE;
    
    return SYLVANC_OK;
}

/**
 * kills or detaches depending on inf->is_attached
 */
static sylvan_code_t sylvan_terminate_or_detach(struct sylvan_inferior *inf) {
    
    assert(inf != NULL); /* inf should not be NULL in an internal library function */

    if (inf->status != SYLVAN_INFSTATE_RUNNING && inf->status != SYLVAN_INFSTATE_STOPPED)
        return SYLVANC_OK;

    sylvan_code_t code;

    if (!inf->is_attached)
        code = sylvan_kill(inf);
    else
        code = sylvan_detach(inf);
    
    if (code == SYLVANC_OK || code == SYLVANC_PROC_NOT_FOUND || code == SYLVANC_PROC_EXITED || code == SYLVANC_PROC_TERMINATED)
        return SYLVANC_OK;

    return code;

}


/**
 * creates an inferior
 */
sylvan_code_t sylvan_inferior_create(struct sylvan_inferior **infp) {
    if (!infp)
        return sylvan_set_code(SYLVANC_INVALID_ARGUMENT);

    struct sylvan_inferior *inf = malloc(sizeof(struct sylvan_inferior));
    if (!inf)
        return sylvan_set_code(SYLVANC_OUT_OF_MEMORY);

    memset(inf, 0, sizeof(struct sylvan_inferior));

    sylvan_code_t code;
    if ((code = sylvan_sym_init(inf))) {
        free(inf);
        return code;
    }

    inf->id = sylvan_inferior_idx++;
    sylvan_inferior_count++;

    *infp = inf;

    return SYLVANC_OK;
}

/**
 * kills or detaches associated process and deletes the inferior
 */
sylvan_code_t sylvan_inferior_destroy(struct sylvan_inferior *inf) {
    if (inf == NULL)
        return sylvan_set_code(SYLVANC_INVALID_ARGUMENT);

    sylvan_code_t code;
    if ((code = sylvan_terminate_or_detach(inf)))
        return code;

    if ((code = sylvan_sym_destroy(inf)))
        return code;

    free(inf->realpath);
    free(inf->args);
    free(inf);

    sylvan_inferior_count--;

    return SYLVANC_OK;
}


/**
 * attaches to a process
 */
sylvan_code_t sylvan_attach(struct sylvan_inferior *inf, pid_t pid) {
    if (inf == NULL)
        return sylvan_set_code(SYLVANC_INVALID_ARGUMENT);

    if (kill(pid, 0) == -1) {
        if (errno == ESRCH)
            return sylvan_set_message(SYLVANC_PROC_NOT_FOUND, "Process %d does not exist", pid);
        return sylvan_set_errno_msg(SYLVANC_SYSTEM_ERROR, "Check process existence");
    }

    sylvan_code_t code;
    if ((code = sylvan_terminate_or_detach(inf)))
        return code;
    
    if (ptrace(PTRACE_ATTACH, pid, NULL, NULL) < 0) {
        if (errno == EPERM)
            return sylvan_set_message(SYLVANC_PTRACE_ATTACH_FAILED, "Permission denied to attach to process %d", pid);
        if (errno == ESRCH)
            return sylvan_set_message(SYLVANC_PROC_NOT_FOUND, "Process %d does not exist", pid);
        return sylvan_set_errno_msg(SYLVANC_PTRACE_ATTACH_FAILED, "ptrace attach");
    }
    
    char *path = NULL;
    sylvan_real_path_pid(pid, &path); /* don't care if it fails */
    
    int status;
    int result;
    do {
        result = waitpid(pid, &status, 0);
    } while (result == -1 && errno == EINTR);
    
    if (result == -1) {
        free(path);
        if (errno == ECHILD)
            return sylvan_set_message(SYLVANC_PROC_NOT_FOUND, "Process %d disappeared during attach", pid);
        return sylvan_set_errno_msg(SYLVANC_WAITPID_FAILED, "waitpid");
    }

    if (WIFSTOPPED(status))
        inf->status = SYLVAN_INFSTATE_STOPPED;
    else
    if (WIFCONTINUED(status))
        inf->status = SYLVAN_INFSTATE_RUNNING;
    else {
        free(path);
        if (WIFEXITED(status))
            return sylvan_set_message(SYLVANC_PROC_EXITED, "Process %d exited during attach", pid);
        if (WIFSIGNALED(status))
            return sylvan_set_message(SYLVANC_PROC_TERMINATED, "Process %d terminated during attach", pid);
        assert(0); /* this shouldn't happen */
    }

    free(inf->realpath);
    inf->pid = pid;
    inf->is_attached = true;
    inf->realpath = path;

    if ((code = sylvan_sym_load_tables(inf)))
        return code;

    if ((code = sylvan_breakpoint_reset_phybp(inf)))
        return code;

    if ((code = sylvan_breakpoint_setall_phybp(inf)))
        return code;

    return SYLVANC_OK;
}

/**
 * detaches from the associated process if inf->is_attached is true
 */
sylvan_code_t sylvan_detach(struct sylvan_inferior *inf) {
    if (inf == NULL)
        return sylvan_set_code(SYLVANC_INVALID_ARGUMENT);

    if (!inf->is_attached)
        return sylvan_set_message(SYLVANC_PROC_NOT_ATTACHED, "Process is not being traced");

    sylvan_code_t code = sylvan_update_inf_status(inf, NULL, false);
    if (code == SYLVANC_PROC_EXITED || code == SYLVANC_PROC_TERMINATED)
        return SYLVANC_OK;

    if (code)
        return code;

    if ((code = sylvan_breakpoint_unsetall_phybp(inf)))
        return code;

    if (ptrace(PTRACE_DETACH, inf->pid, NULL, NULL) < 0)
        if (errno != ESRCH)
            return sylvan_set_errno_msg(SYLVANC_PTRACE_DETACH_FAILED, "ptrace detach");

    inf->is_attached = false;
    inf->pid = 0;
    inf->status = SYLVAN_INFSTATE_NONE;

    return SYLVANC_OK;
}


/**
 * sends code and errno to parent and exits
 */
static void exit_child(int wd, sylvan_code_t code) {
    if (dprintf(wd, "%d %d", code, errno) < 0)
        _exit(1);
    if (close(wd) < 0)
        _exit(1);
    _exit(0);
}

/**
 * well, handles child process
 */
static void handle_child(struct sylvan_inferior *inf, int fd[2]) {

    if (close(fd[0]) < 0)
        _exit(1);

    int wd = fd[1];

    if (setpgid(0, 0))
        exit_child(wd, SYLVANC_SYSTEM_ERROR);

    if (ptrace(PTRACE_TRACEME, 0, NULL, NULL) < 0)
        exit_child(wd, SYLVANC_PTRACE_ERROR);

    size_t args_len = inf->args == NULL ? 0 : strlen(inf->args);
    size_t path_len = strlen(inf->realpath);

    char *args = malloc(path_len + 1 + (args_len ? args_len + 1 : 0));
    if (args == NULL) {
        errno = ENOMEM;
        exit_child(wd, SYLVANC_OUT_OF_MEMORY);
    }

    memcpy(args, inf->realpath, path_len);

    char **argv = (char*[]){ args, NULL };

    if (inf->args != NULL) {
        args[path_len] = ' ';
        memcpy(args + path_len + 1, inf->args, args_len + 1);

        wordexp_t p;
        int res;
        if ((res = wordexp(args, &p, WRDE_NOCMD))) {
            if (res == WRDE_NOSPACE)
                exit_child(wd, SYLVANC_OUT_OF_MEMORY);
            else
                exit_child(wd, SYLVANC_INVALID_ARGUMENT);
        }
        argv = p.we_wordv;
    }

    if (close(wd) < 0)
        _exit(1);

    execvp(inf->realpath, argv);

    _exit(1);
}

/**
 * handles parent process
 */
static sylvan_code_t handle_parent(pid_t pid, struct sylvan_inferior *inf, int fd[2]) {

    if (close(fd[1]) < 0)
        return sylvan_set_errno_msg(SYLVANC_SYSTEM_ERROR, "close pipe");

    int rd = fd[0];

    char buf[128];
    ssize_t bytes_read;
    do {
        bytes_read = read(rd, buf, sizeof(buf));
    } while (bytes_read == -1 && errno == EINTR);

    if (close(rd) < 0)
        return sylvan_set_errno_msg(SYLVANC_SYSTEM_ERROR, "close pipe");

    if (bytes_read < 0)
        return sylvan_set_errno_msg(SYLVANC_SYSTEM_ERROR, "read from pipe");

    if (bytes_read > 0) {
        int code;
        int child_errno;
        if (sscanf(buf, "%d %d", &code, &child_errno) != 2)
            return sylvan_set_message(SYLVANC_SYSTEM_ERROR, "parse child error message");

        switch ((errno = child_errno)) {
            case SYLVANC_PTRACE_ERROR:      return sylvan_set_errno_msg(SYLVANC_PTRACE_ERROR, "ptrace in child");
            case SYLVANC_OUT_OF_MEMORY:     return sylvan_set_code(SYLVANC_OUT_OF_MEMORY);
            case SYLVANC_INVALID_ARGUMENT:  return sylvan_set_message(SYLVANC_INVALID_ARGUMENT, "Invalid arguments for child process");
            case SYLVANC_FILE_NOT_FOUND:    return sylvan_set_message(SYLVANC_FILE_NOT_FOUND, "Executable file not found");
            return sylvan_set_code(code);
        }
        return sylvan_set_code(code);
    }

    int status;
    int res;
    do {
        res = waitpid(pid, &status, 0);
    } while (res == -1 && errno == EINTR);

    if (res == -1)
        return sylvan_set_errno_msg(SYLVANC_WAITPID_FAILED, "waitpid");

    if (WIFEXITED(status))
        return sylvan_set_message(SYLVANC_PROC_CHILD, "Child process exited with code %d", WEXITSTATUS(status));

    sylvan_update_wait_status(status, inf);
    inf->pid = pid;
    inf->is_attached = false;

    sylvan_code_t code;
    if ((code = sylvan_breakpoint_reset_phybp(inf)))
        return code;

    if ((code = sylvan_breakpoint_setall_phybp(inf)))
        return code;

    if (ptrace(PTRACE_CONT, inf->pid, NULL, NULL) < 0)
        return sylvan_set_errno_msg(SYLVANC_PTRACE_CONT_FAILED, "ptrace cont");

    return sylvan_update_inf_status(inf, NULL, true);
}

/**
 * runs a new process. replaces existing process
 */
sylvan_code_t sylvan_run(struct sylvan_inferior *inf) {
    if (inf == NULL)
        return sylvan_set_code(SYLVANC_INVALID_ARGUMENT);

    if (inf->realpath == NULL)
        return sylvan_set_message(SYLVANC_FILE_NOT_FOUND, "No executable path specified");

    if (access(inf->realpath, X_OK))
        return sylvan_set_errno_msg(SYLVANC_NOT_EXECUTABLE, "File '%s' is not executable", inf->realpath);

    sylvan_code_t code;
    if ((code = sylvan_kill(inf)))
        return code;

    int fd[2];
    if (pipe(fd) < 0)
        return sylvan_set_errno_msg(SYLVANC_PIPE_FAILED, "pipe");
    
    pid_t pid = fork();
    if (pid < 0) {
        close(fd[0]);
        close(fd[1]);
        return sylvan_set_errno_msg(SYLVANC_FORK_FAILED, "fork");
    }

    if (pid == 0)
        handle_child(inf, fd); // never returns

    return handle_parent(pid, inf, fd);
}

/**
 * helper function to handle breakpoint at current instruction address
 */
static sylvan_code_t sylvan_handle_breakpoint_at_current_addr(struct sylvan_inferior *inf, int *wstatus) {
    sylvan_code_t code;
    
    struct user_regs_struct regs;
    if (ptrace(PTRACE_GETREGS, inf->pid, NULL, &regs) < 0)
        return sylvan_set_errno_msg(SYLVANC_PTRACE_GETREGS_FAILED, "ptrace get regs");

    struct sylvan_breakpoint *breakpoint;
    if (sylvan_breakpoint_find_by_addr(inf, regs.rip - 1, &breakpoint) == SYVLANC_BREAKPOINT_NOT_FOUND)
        return SYVLANC_BREAKPOINT_NOT_FOUND;

    if (!breakpoint->is_enabled_phy)
        return SYVLANC_BREAKPOINT_NOT_FOUND; 

    regs.rip--;

    if (ptrace(PTRACE_SETREGS, inf->pid, NULL, &regs) < 0)
        return sylvan_set_errno_msg(SYLVANC_PTRACE_SETREGS_FAILED, "ptrace set regs");

    if ((code = sylvan_breakpoint_disable_ptr(inf, breakpoint)))
        return code;

    if (ptrace(PTRACE_SINGLESTEP, inf->pid, NULL, NULL) < 0)
        return sylvan_set_errno_msg(SYLVANC_PTRACE_STEP_FAILED, "ptrace single step");

    if ((code = sylvan_update_inf_status(inf, wstatus, true)))
        return code;

    if ((code = sylvan_breakpoint_enable_ptr(inf, breakpoint)))
        return code;

    return SYLVANC_OK;
}

/**
 * validates process state before operations
 */
static sylvan_code_t sylvan_validate_process_state(struct sylvan_inferior *inf, int *wstatus) {
    if (inf == NULL)
        return sylvan_set_code(SYLVANC_INVALID_ARGUMENT);
    
    sylvan_code_t code;
    if ((code = sylvan_update_inf_status(inf, wstatus, false)))
        return code;

    if ((inf->status != SYLVAN_INFSTATE_STOPPED)) {
        if (inf->status == SYLVAN_INFSTATE_RUNNING)
            return sylvan_set_message(SYLVANC_PROC_RUNNING, "Process %d is already running", inf->pid);
        return sylvan_set_message(SYLVANC_INVALID_STATE, "Process %d is not in a stopped state", inf->pid);
    }
    
    return SYLVANC_OK;
}

/**
 * continues the stopped process
 */
sylvan_code_t sylvan_continue(struct sylvan_inferior *inf) {
    sylvan_code_t code;
    
    if ((code = sylvan_validate_process_state(inf, NULL)))
        return code;

    if ((code = sylvan_handle_breakpoint_at_current_addr(inf, NULL)) && code != SYVLANC_BREAKPOINT_NOT_FOUND)
        return code;

    if (ptrace(PTRACE_CONT, inf->pid, NULL, NULL) < 0)
        return sylvan_set_errno_msg(SYLVANC_PTRACE_CONT_FAILED, "ptrace cont");

    if ((code = sylvan_update_inf_status(inf, NULL, true)))
        return code;

    return SYLVANC_OK;
}

/**
 * steps through a single instruction
 */
sylvan_code_t sylvan_stepinst(struct sylvan_inferior *inf) {
    sylvan_code_t code;
    
    if ((code = sylvan_validate_process_state(inf, NULL)))
        return code;

    if ((code = sylvan_handle_breakpoint_at_current_addr(inf, NULL)) && code != SYVLANC_BREAKPOINT_NOT_FOUND)
        return code;

    if (!code)
        return SYLVANC_OK;
    
    if (ptrace(PTRACE_SINGLESTEP, inf->pid, NULL, NULL) < 0)
        return sylvan_set_errno_msg(SYLVANC_PTRACE_STEP_FAILED, "ptrace single step");

    return sylvan_update_inf_status(inf, NULL, true);
}
/**
 * gets cpu regs
 */
sylvan_code_t sylvan_get_regs(struct sylvan_inferior *inf, struct user_regs_struct *regs) {
    if (inf == NULL || regs == NULL)
        return sylvan_set_code(SYLVANC_INVALID_ARGUMENT);
    
    sylvan_code_t code;
    if ((code = sylvan_update_inf_status(inf, NULL, false)))
        return code;

    if (inf->status != SYLVAN_INFSTATE_STOPPED && inf->status != SYLVAN_INFSTATE_RUNNING)
        return sylvan_set_message(SYLVANC_INVALID_STATE, "Cannot get registers: process is not running or stopped");

    if (ptrace(PTRACE_GETREGS, inf->pid, NULL, regs) < 0)
        return sylvan_set_errno_msg(SYLVANC_PTRACE_GETREGS_FAILED, "ptrace get regs");
    
    return SYLVANC_OK;
}

/**
 * sets cpu regs
 */
sylvan_code_t sylvan_set_regs(struct sylvan_inferior *inf, const struct user_regs_struct *regs) {
    if (inf == NULL || regs == NULL)
        return sylvan_set_code(SYLVANC_INVALID_ARGUMENT);
    
    sylvan_code_t code;
    if ((code = sylvan_update_inf_status(inf, NULL, false)))
        return code;
    
    if (inf->status != SYLVAN_INFSTATE_STOPPED && inf->status != SYLVAN_INFSTATE_RUNNING)
        return sylvan_set_message(SYLVANC_INVALID_STATE,  "Cannot set registers: process is not running or stopped");
    
    if (ptrace(PTRACE_SETREGS, inf->pid, NULL, regs) < 0)
        return sylvan_set_errno_msg(SYLVANC_PTRACE_SETREGS_FAILED, "ptrace set regs");
    
    return SYLVANC_OK;
}


/**
 * set executable path for the inferior
 */
sylvan_code_t sylvan_set_filepath(struct sylvan_inferior *inf, const char *filepath) {
    if (inf == NULL)
        return sylvan_set_code(SYLVANC_INVALID_ARGUMENT);

    if (filepath == NULL) {
        free(inf->realpath);
        inf->realpath = NULL;
        return SYLVANC_OK;
    }
    
    char *newpath;
    sylvan_code_t code;
    if ((code = sylvan_canonical_path(filepath, &newpath)) != SYLVANC_OK)
        return code;
    
    if (access(newpath, X_OK) != 0) {
        free(newpath);
        int len = strlen(filepath);
        if (len <= 256)
            return sylvan_set_errno_msg(SYLVANC_NOT_EXECUTABLE, "File '%s' is not executable", filepath);
        return sylvan_set_errno_msg(SYLVANC_NOT_EXECUTABLE, "File '%.256s...' is not executable", filepath);
    }

    free(inf->realpath);
    inf->realpath = newpath;

    if ((code = sylvan_sym_load_tables(inf)))
        return code;

    return SYLVANC_OK;
}

/**
 * sets the args. unsets them if NULL is passed
 */
sylvan_code_t sylvan_set_args(struct sylvan_inferior *inf, const char *args) {
    if (inf == NULL)
        return sylvan_set_code(SYLVANC_INVALID_ARGUMENT);

    if (args == NULL) {
        free(inf->args);
        inf->args = NULL;
        return SYLVANC_OK;
    }

    char *newargs = strdup(args);
    if (newargs == NULL)
        return sylvan_set_code(SYLVANC_OUT_OF_MEMORY);
    
    free(inf->args);
    inf->args = newargs;
    
    return SYLVANC_OK;
}

/**
 * reads a memory location
 */
sylvan_code_t sylvan_get_memory(struct sylvan_inferior *inf, uintptr_t addr, uint64_t *data){
    if (inf == NULL)
        return sylvan_set_code(SYLVANC_INVALID_ARGUMENT);
    
    errno = 0;
    uint64_t _data;
    _data = ptrace(PTRACE_PEEKDATA, inf->pid, (void *)(addr), NULL);
    if (errno)
        return sylvan_set_errno_msg(SYLVANC_PTRACE_PEEKDATA_FAILED, "Cannot read address %lx", (addr));

    *data = _data;

    return SYLVANC_OK;
}


/**
 * set a memory location to given bytes
 */
sylvan_code_t sylvan_set_memory(struct sylvan_inferior *inf, uintptr_t addr, const void *data, size_t size) {
    
    if (inf == NULL || data == NULL)
        return sylvan_set_code(SYLVANC_INVALID_ARGUMENT);
    
    if (size == 0)
        return sylvan_set_code(SYLVANC_OK); 
    
    if (addr == 0)
        return sylvan_set_errno_msg(SYLVANC_INVALID_ARGUMENT, "Invalid address 0x%lx", addr);
    

    const uint8_t *bytes = (const uint8_t *)data;
    size_t offset = 0;

    while (offset + 8 <= size) {
        uint64_t chunk = 0;
        memcpy(&chunk, bytes + offset, 8);

        if (ptrace(PTRACE_POKEDATA, inf->pid, (void *)(addr + offset), (void *)chunk) < 0) 
            return sylvan_set_errno_msg(SYLVANC_PTRACE_POKEDATA_FAILED, "cannot write at 0x%lx: %s", addr + offset);
        
        offset += 8;
    }
    
    if (offset < size) {
        size_t remaining = size - offset;
        uint64_t current_data = 0;
        if (sylvan_get_memory(inf, (uintptr_t){addr + offset}, &current_data)) 
            return sylvan_set_errno_msg(SYLVANC_PTRACE_POKEDATA_FAILED, "cannot write at 0x%lx: %s", addr + offset);        
        
        uint64_t new_data = 0;
        memcpy(&new_data, bytes + offset, remaining);

        uint64_t mask = (1ULL << (remaining * 8)) - 1;
        new_data = (current_data & ~mask) | (new_data & mask);

        if (ptrace(PTRACE_POKEDATA, inf->pid, (void *)(addr + offset), (void *)new_data) < 0) 
            return sylvan_set_errno_msg(SYLVANC_PTRACE_POKEDATA_FAILED, "cannot write at 0x%lx: %s", addr + offset);
        
    }

    return SYLVANC_OK;
}

sylvan_code_t sylvan_set_breakpoint_function(struct sylvan_inferior *inf, const char *function) {
    if (!inf || !function)
        return sylvan_set_code(SYLVANC_INVALID_ARGUMENT);

    uintptr_t addr;
    sylvan_code_t code;
    if ((code = sylvan_get_label_addr(inf, function, &addr)))
        return code;

    if ((code = sylvan_breakpoint_set(inf, addr)))
        return code;

    return SYLVANC_OK;
}
