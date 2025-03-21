#ifndef SYLVAN_UTILS_H
#define SYLVAN_UTILS_H

#include  "error.h"

/**
 * returns the canonical path of the filepath if it exists
 * *real_path will point to a malloced path if it succeeds
 * real_path is untouched if it fails
 * it does not modify sylvan_last_error
 */
sylvan_code_t sylvan_real_path(const char *filepath, char **real_path);

/**
 * same as sylvan_real_path but with pid
 */
sylvan_code_t sylvan_real_path_pid(pid_t pid, char **real_path);

/**
 * searchs for a command in directories specified by PATH variable
 * *filepath will point to a malloced path if it succeeds
 * filepath is untouched if it fails
 */
sylvan_code_t sylvan_find_in_PATH(const char *command, char **filepath);

/**
 * returns the canonical path of the "filepath", where filepath could be either a file path or a command
 * if the real path of is not found, it searches for it in PATH
 * canonical_path is untouched if it fails
 */
sylvan_code_t sylvan_canonical_path(const char *filepath, char **canonical_path);

#endif /* SYLVAN_UTILS_H */
