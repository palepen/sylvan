#include <stdatomic.h>
#include <stdlib.h>
#include <signal.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <limits.h>
#include <sys/ptrace.h>
#include <sylvan/inferior.h>


static int inferior_idx = 0;
static int inferior_count;


struct inferior *inferior_create() {
    struct inferior *inf = (malloc(sizeof(struct inferior)));
    if (inf == NULL)
        return NULL;
    memset(inf, 0, sizeof(struct inferior));
    inf->id = atomic_fetch_add(&inferior_idx, 1);
    atomic_fetch_add(&inferior_count, 1);
    return inf;
}

/**
 * @brief detaches from a process
 */
int inferior_detach(struct inferior *inf) {
    if (inf == NULL)
        return 0;
    if (!inf->is_attached)
        return 0;
    if (ptrace(PTRACE_DETACH, inf->pid, NULL, NULL) < 0)
        return -1;
    inf->is_attached = false;
    inf->pid = -1;
    inf->state = INFERIOR_NONE;
    return 0;
}

/**
 * @brief kills a process
 */
int inferior_kill(struct inferior *inf) {
    if (inf == NULL)
        return 0;
    if (inf->state != INFERIOR_RUNNING && inf->state != INFERIOR_STOPPED)
        return 0;
    if (kill(inf->pid, SIGTERM) < 0)
        return -1;
    inf->is_attached = false;
    inf->pid = -1;
    inf->state = INFERIOR_NONE;
    return 0;
}

/**
 * function to reset the state of the inf in between process changes
 * detaches or kills the process depending on inf->is_attached
 * only fields related to process are reset. args for eg, is not reset
 */
static int inferior_reset_state(struct inferior *inf) {
    if (inf == NULL)
        return 0;
    if (inf->state == INFERIOR_RUNNING || inf->state == INFERIOR_STOPPED) {
        if (inf->is_attached) {
            if (inferior_detach(inf) < 0)
                return - 1;
        } else {
            if (inferior_kill(inf) < 0)
                return -1;
        }
    }
    free(inf->realpath);
    return 0;
}


/**
 * @brief finds the real (absolute) path of a file given its path
 * @param filepath file path
 * @return a malloced string of real path or NULL in case of failure
 */
inline static char *inferior_realpath_file(const char *filepath) {
    return realpath(filepath, NULL);
}

/**
 * @brief finds the real (absolute) path of the process's executable given its pid
 * @param pid process id
 * @return a malloced string of real path or NULL in case of failure
 */
static char *inferior_realpath_pid(pid_t pid) {
    char exepath[64];
    sprintf(exepath, "/proc/%d/exe", pid);
    return inferior_realpath_file(exepath);
}

/**
 * @brief attaches to a process given its pid
 */
int inferior_attach_pid(struct inferior *inf, pid_t pid) {
    if (inferior_reset_state(inf) < 0)
        return -1;
    if (ptrace(PTRACE_ATTACH, pid, NULL, NULL) < 0)
        return -1;
    inf->pid = pid;
    inf->state = INFERIOR_STOPPED;
    inf->realpath = inferior_realpath_pid(inf->pid);
    inf->is_attached = true;
    return 0;
}

/**
 * @brief deletes an inferior
 */
int inferior_destroy(struct inferior *inf) {
    if (inf == NULL)
        return 0;
    if (inferior_reset_state(inf) < 0)
        return -1;
    free(inf->args);
    free(inf);
    atomic_fetch_add(&inferior_count, -1);
    return 0;
}
