#include <errno.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>
#include "error.h"


static char sylvan_errmsg_buffer[2048];

struct sylvan_error_context sylvan_last_error = {
    .code       = SYLVANC_OK,
    .os_errno   = 0,
    .message    = sylvan_errmsg_buffer,
};

const char *sylvan_strerror(sylvan_code_t code) {
    switch (code) {
        case SYLVANC_OK:                        return "Ok";

        case SYLVANC_ERROR:                     return "Something went wrong";
        case SYLVANC_OUT_OF_MEMORY:             return "Out of memory";
        case SYLVANC_INVALID_ARGUMENT:          return "Invalid argument";
        case SYLVANC_INVALID_STATE:             return "Invalid operation in current state";
        case SYLVANC_FILE_NOT_FOUND:            return "File not found or not accessible";
        case SYLVANC_NOT_EXECUTABLE:            return "File is not executable";

        case SYLVANC_PROC_NOT_FOUND:            return "Process does not exist";
        case SYLVANC_PROC_NOT_ATTACHED:         return "Process is not being traced";
        case SYLVANC_PROC_ALREADY_ATTACHED:     return "Process is already being traced";
        case SYLVANC_PROC_EXITED:               return "Process has exited normally";
        case SYLVANC_PROC_TERMINATED:           return "Process was terminated by signal";
        case SYLVANC_PROC_RUNNING:              return "Process is running (not stopped)";
        case SYLVANC_PROC_STOPPED:              return "Process is stopped";
        case SYLVANC_PROC_ZOMBIE:               return "Process is in zombie state";
        case SYLVANC_PROC_CHILD:                return "Error in child process";

        case SYLVANC_SYSTEM_ERROR:              return "System error";
        case SYLVANC_FORK_FAILED:               return "Fork failed";
        case SYLVANC_PIPE_FAILED:               return "Pipe creation failed";
        case SYLVANC_WAITPID_FAILED:            return "Wait for process failed";
        case SYLVANC_EXEC_FAILED:               return "Exec failed";
        case SYLVANC_KILL_FAILED:               return "Kill signal failed";

        case SYLVANC_PTRACE_ERROR:              return "Ptrace operation failed";
        case SYLVANC_PTRACE_ATTACH_FAILED:      return "Could not attach to process";
        case SYLVANC_PTRACE_DETACH_FAILED:      return "Could not detach from process";
        case SYLVANC_PTRACE_CONT_FAILED:        return "Could not continue process";
        case SYLVANC_PTRACE_STEP_FAILED:        return "Single step failed";
        case SYLVANC_PTRACE_GETREGS_FAILED:     return "Get registers failed";
        case SYLVANC_PTRACE_SETREGS_FAILED:     return "Set registers failed";

    }
    return "Unknown error";
}

sylvan_code_t sylvan_set_code(sylvan_code_t code) {
    sylvan_last_error.code      = code;
    sylvan_last_error.os_errno  = errno;
    sylvan_last_error.message   = sylvan_strerror(code);
    return code;
}

sylvan_code_t sylvan_set_errno(sylvan_code_t code) {
    sylvan_last_error.code = code;
    sylvan_last_error.os_errno = errno;
    sylvan_last_error.message = strerror(errno);
    return code;
}

sylvan_code_t sylvan_set_errno_msg(sylvan_code_t code, const char *fmt, ...) {
    char *ptr = sylvan_errmsg_buffer;
    size_t rem = sizeof(sylvan_errmsg_buffer);

    va_list args;
    va_start(args, fmt);
    int count = vsnprintf(ptr, rem, fmt, args);
    va_end(args);

    if ((size_t)count < rem - 3) {
        ptr += count;
        rem -= count;
        snprintf(ptr, rem, ": %s", strerror(errno));
    }

    sylvan_errmsg_buffer[sizeof(sylvan_errmsg_buffer) - 1] = '\0';
    sylvan_last_error.code = code;
    sylvan_last_error.os_errno = errno;
    sylvan_last_error.message = sylvan_errmsg_buffer;
    return code;
}

sylvan_code_t sylvan_set_message(sylvan_code_t code, const char *fmt, ...) {
    va_list args;
    va_start(args, fmt);
    vsnprintf(sylvan_errmsg_buffer, sizeof(sylvan_errmsg_buffer) - 1, fmt, args);
    va_end(args);
    sylvan_last_error.code = code;
    sylvan_last_error.os_errno = errno;
    sylvan_last_error.message = sylvan_errmsg_buffer;
    return code;
}

const char *sylvan_get_last_error(void) {
    return sylvan_last_error.message;
}
