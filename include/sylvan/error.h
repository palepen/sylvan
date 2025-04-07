#ifndef SYLVAN_INCLUDE_SYLVAN_ERROR_H
#define SYLVAN_INCLUDE_SYLVAN_ERROR_H

typedef enum sylvan_code_t {

    /* success - guaranteed to always be 0 */
    SYLVANC_OK                      = 0x000,

    /* errors with no errno */
    SYLVANC_ERROR                   = 0x100,
    SYLVANC_OUT_OF_MEMORY                  ,    /* malloc failed */
    SYLVANC_INVALID_ARGUMENT               ,    /* invalid function argument */
    SYLVANC_INVALID_STATE                  ,    /* operation invalid in current state */
    SYLVANC_FILE_NOT_FOUND                 ,    /* file not found or not accessible */

    /* process codes */
    SYLVANC_PROC_ERROR              = 0x200,
    SYLVANC_PROC_NOT_FOUND                 ,    /* process does not exist */
    SYLVANC_PROC_NOT_ATTACHED              ,    /* process isn't being traced by us */
    SYLVANC_PROC_ALREADY_ATTACHED          ,    /* process is already being traced */
    SYLVANC_PROC_EXITED                    ,    /* process has exited normally */
    SYLVANC_PROC_TERMINATED                ,    /* process was terminated by signal */
    SYLVANC_PROC_RUNNING                   ,    /* process is running (not stopped) */
    SYLVANC_PROC_STOPPED                   ,    /* process is stopped */
    SYLVANC_PROC_ZOMBIE                    ,    /* process is in zombie state */
    SYLVANC_PROC_CHILD                     ,    /* error in child process */

    /* system call codes */
    SYLVANC_SYSTEM_ERROR            = 0x300,
    SYLVANC_FORK_FAILED                    ,    /* fork failed */
    SYLVANC_PIPE_FAILED                    ,    /* pipe creation failed */
    SYLVANC_WAITPID_FAILED                 ,    /* wait for process failed */
    SYLVANC_EXEC_FAILED                    ,    /* exec failed */
    SYLVANC_KILL_FAILED                    ,    /* kill signal failed */
    SYLVANC_NOT_EXECUTABLE                 ,    /* file is not executable */

    /* ptrace codes */
    SYLVANC_PTRACE_ERROR            = 0x400,
    SYLVANC_PTRACE_ATTACH_FAILED           ,    /* could not attach to process */
    SYLVANC_PTRACE_DETACH_FAILED           ,    /* could not detach from process */
    SYLVANC_PTRACE_CONT_FAILED             ,    /* continue failed */
    SYLVANC_PTRACE_STEP_FAILED             ,    /* single-step failed */
    SYLVANC_PTRACE_GETREGS_FAILED          ,    /* could not get regs */
    SYLVANC_PTRACE_SETREGS_FAILED          ,    /* could not set regs */
    SYLVANC_PTRACE_PEEKTEXT_FAILED         ,    /* could not read from memory */
    SYLVANC_PTRACE_POKETEXT_FAILED         ,    /* could not write to memory */
    SYLVANC_PTRACE_PEEKDATA_FAILED         ,    /* could not read from memory */
    SYLVANC_PTRACE_POKEDATA_FAILED         ,    /* could not write to memory */

    /* breakpoint errors */
    SYVLANC_BREAKPOINT_ERROR        = 0x500,
    SYVLANC_BREAKPOINT_ALREADY_EXISTS      ,    /* breakpoint already exits at memory address */
    SYVLANC_BREAKPOINT_NOT_FOUND           ,    /* breakpoint not found */
    SYVLANC_BREAKPOINT_LIMIT_REACHED       ,    /* too many breakpoints */

    /* symbol errors*/
    SYLVANC_SYMBOL_ERROR            = 0x600,
    SYLVANC_ELF_FAILED                     ,    /* could not read the elf file */
    SYLVANC_DWARF_NOT_FOUND                ,    /* not dwarf info */
    SYLVANC_SYMBOL_NOT_FOUND               ,    /* symbol not found */

} sylvan_code_t;

struct sylvan_error_context {
    sylvan_code_t code;     /* error code */
    int os_errno;           /* errno that was set by the system */
    const char *message;    /* human-readable error message */
};

/**
 * @brief a human readable description of the last error
 * 
 * use this if a function returns a non zero error code 
 */
const char *sylvan_get_last_error(void);

#endif /* SYLVAN_INCLUDE_SYLVAN_ERROR_H */
