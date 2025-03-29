#include <assert.h>
#include <errno.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include "error.h"
#include "utils.h"

/**
 * see lib/utils.h
 */
sylvan_code_t sylvan_real_path(const char *filepath, char **real_path) {

    assert(filepath != NULL && real_path != NULL);

    char *path = realpath(filepath, NULL);
    if (path == NULL) {
        if (errno == ENOENT)
            return sylvan_set_code(SYLVANC_FILE_NOT_FOUND);
        return sylvan_set_errno_msg(SYLVANC_SYSTEM_ERROR, "real path");
    }

    *real_path = path;
    return SYLVANC_OK;
}

/**
 * see lib/utils.h
 */
sylvan_code_t sylvan_find_in_PATH(const char *command, char **filepath) {

    assert(command != NULL && filepath != NULL);

    char *path = getenv("PATH");
    if (path == NULL)
        return sylvan_set_code(SYLVANC_FILE_NOT_FOUND);

    size_t command_len = strlen(command);
    path = strdup(path);
    if (path == NULL)
        return sylvan_set_code(SYLVANC_OUT_OF_MEMORY);

    char *token = strtok(path, ":");
    while (token) {
        size_t token_len = strlen(token);
        size_t path_len = token_len + 1 + command_len + 1; // path + '/' + command + '\0'
        char *full_path = malloc(path_len);
        if (full_path == NULL) {
            free(path);
            return sylvan_set_code(SYLVANC_OUT_OF_MEMORY);
        }

        memcpy(full_path, token, token_len);
        full_path[token_len] = '/';
        memcpy(full_path + token_len + 1, command, command_len + 1);

        if (access(full_path, X_OK) == 0) {
            *filepath = full_path;
            free(path);
            return SYLVANC_OK;
        }

        free(full_path);
        token = strtok(NULL, ":");
    }

    free(path);
    return sylvan_set_code(SYLVANC_FILE_NOT_FOUND);
}

/**
 * see lib/utils.h
 */
sylvan_code_t sylvan_canonical_path(const char *filepath, char **canonical_path) {
    assert(filepath != NULL);

    char *path;
    sylvan_code_t code = sylvan_real_path(filepath, &path);
    if (!code) {
        *canonical_path = path;
        return SYLVANC_OK;
    }

    if (code != SYLVANC_FILE_NOT_FOUND)
        return  code;
    if ((code = sylvan_find_in_PATH(filepath, &path)))
        return code;

    *canonical_path = path;
    return SYLVANC_OK;
}

/**
 * see lib/utils.h
 */
sylvan_code_t sylvan_real_path_pid(pid_t pid, char **real_path) {
    char exepath[64];
    sprintf(exepath, "/proc/%d/exe", pid);
    return sylvan_real_path(exepath, real_path);
}
