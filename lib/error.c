#include <errno.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>
#include "error.h"


static const char *sylvan_error_messages[SYLVANE_COUNT] = {
    [SYLVANE_OK]                    = "Ok",
    [SYLVANE_INVALID_ARGUMENT]      = "Invalid Argument",
    [SYLVANE_OUT_OF_MEMORY]         = "Out of Memory",
    [SYLVANE_NOEXEC]                = "Executable not found",
    [SYLVANE_INF_INVALID_STATE]     = "Operation not allow for inferior's current state",
    [SYLVANE_PTRACE_ATTACH]         = "Could not attach",
    [SYLVANE_PTRACE_CONT]           = "Could not continue",
    [SYLVANE_PTRACE_DETACH]         = "Could not detach",
    [SYLVANE_PROC_ATTACH]           = "Could attach to the process",
    [SYLVANE_PROC_WAIT]             = "Wait error",
    [SYLVANE_PROC_FORK]             = "Could not fork",
    [SYLVANE_PROC_KILL]             = "Could not kill the process",
    [SYLVANE_PROC_NOT_ATTACHED]     = "No attached process",
};

static_assert(
    sizeof(sylvan_error_messages) / sizeof(sylvan_error_messages[0]) == SYLVANE_COUNT,
    __FILE__ ": sylvan_error_messages array is invalid"
);


const char *sylvan_strerror(sylvan_code_t code) {
    if (0 > code || code >= SYLVANE_COUNT)
        return "Unknown Error";
    return sylvan_error_messages[code];
}

static char sylvan_errmsg_buffer[2048];

struct sylvan_error_context sylvan_last_error;

/**
 * @brief last error message = sylvan_strerror(code)
*/
sylvan_code_t sylvan_set_code(sylvan_code_t code) {
    sylvan_last_error.code      = code;
    sylvan_last_error.os_errno  = errno;
    sylvan_last_error.message   = sylvan_strerror(code);
    return code;
}

/**
 * @brief last error message = strerror(errno)
*/
sylvan_code_t sylvan_set_errno(sylvan_code_t code) {
    sylvan_last_error.code      = code;
    sylvan_last_error.os_errno  = errno;
    sylvan_last_error.message   = strerror(errno);
    return code;
}

/**
 * @brief last error message = fmt + ": " + strerror(errno)
*/
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

/**
 * @brief last error message = fmt
*/
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

const char *sylvan_get_last_error() {
    return sylvan_last_error.message ? sylvan_last_error.message : ""; // TODO: this is a tmp fix
}
