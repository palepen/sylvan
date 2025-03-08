#include <stdlib.h>
#include <signal.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <limits.h>
#include <sys/ptrace.h>
#include <sys/wait.h>
#include <sylvan/inferior.h>


static int inferior_idx = 0;
static int inferior_count = 0;


struct inferior *inferior_create() {
    struct inferior *inf = (malloc(sizeof(struct inferior)));
    if (inf == NULL)
        return NULL;
    memset(inf, 0, sizeof(struct inferior));
    inf->id = inferior_idx++;
    inferior_count++;
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
    inf->status = INFERIOR_NONE;
    return 0;
}

/**
 * @brief kills a process
 */
int inferior_kill(struct inferior *inf) {
    if (inf == NULL)
        return 0;
    if (inf->status != INFERIOR_RUNNING && inf->status != INFERIOR_STOPPED)
        return 0;
    if (kill(inf->pid, SIGKILL) < 0)
        return -1;
    inf->is_attached = false;
    inf->pid = -1;
    inf->status = INFERIOR_NONE;
    return 0;
}

/**
 * function to reset the status of the inf in between process changes
 * detaches or kills the process depending on inf->is_attached
 * only fields related to process are reset. args for eg, is not reset
 */
static int inferior_reset_state(struct inferior *inf) {
    if (inf == NULL)
        return 0;
    if (inf->status == INFERIOR_RUNNING || inf->status == INFERIOR_STOPPED) {
        if (inf->is_attached) {
            if (inferior_detach(inf) < 0)
                return - 1;
        } else {
            if (inferior_kill(inf) < 0)
                return -1;
        }
    }
    return 0;
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
static char *inferior_realpath_pid(pid_t pid) {
    char exepath[64];
    sprintf(exepath, "/proc/%d/exe", pid);
    return get_realpath_file(exepath);
}

/**
 * @brief attaches to a process given its pid
 */
int inferior_attach_pid(struct inferior *inf, pid_t pid) {
    if (inferior_reset_state(inf) < 0)
        return -1;
    if (ptrace(PTRACE_ATTACH, pid, NULL, NULL) < 0)
        return -1;
    int status;
    if (waitpid(pid, &status, 0) < 0 || !WIFSTOPPED(status))
        return -1;
    inf->pid = pid;
    inf->status = INFERIOR_STOPPED;
    free(inf->realpath);
    // it's okay for the real path to be null since this is an attached process
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
    free(inf->realpath);
    free(inf->args);
    free(inf);
    inferior_count--;
    return 0;
}

static void handle_child(struct inferior *inf) {
    if (ptrace(PTRACE_TRACEME, 0, NULL, NULL) < 0)
        _exit(EXIT_FAILURE);
    
    int len = 4 + strlen(inf->realpath); // 2 quotes + 1 space + 1 '\0' = 4
    if (inf->args)
        len += strlen(inf->args);
    char *cmd = malloc(len);
    if (cmd == NULL)
        _exit(EXIT_FAILURE);
    snprintf(cmd, len, "\"%s\" %s", inf->realpath, inf->args ? inf->args : "");
    char *args[] = {"/bin/sh", "-c", cmd, NULL};
    execvp(args[0], args);
    free(cmd);
    _exit(EXIT_FAILURE);
}

static int handle_parent(pid_t pid, struct inferior *inf) {
    inf->pid = pid;
    inf->status = INFERIOR_RUNNING;
    int status;

    if (waitpid(pid, &status, 0) < 0)
        return -1;

    if (WIFEXITED(status))
        inf->status = INFERIOR_EXITED;  // _exit()
    else
    if (WIFSTOPPED(status))             // kill -TERM and -STOP
        inf->status = INFERIOR_STOPPED;
    else
    if (WIFSIGNALED(status))            // kill -KILL
        inf->status = INFERIOR_TERMINATED;

    return 0;
}

int inferior_run(struct inferior *inf) {
    if (inf == NULL)
        return 0;
    if (inf->realpath == NULL)
        return -1;
    if (inferior_kill(inf) < 0)
        return -1;
    pid_t pid = fork();
    if (pid < 0)
        return -1;
    if (pid == 0)
        handle_child(inf); // this won't return
    return handle_parent(pid, inf);
}

int inferior_set_filepath(struct inferior *inf, const char *filepath) {
    if (inf == NULL)
        return 0;
    if (filepath == NULL)
        return -1;
    char *newpath = get_realpath_file(filepath);
    if (newpath == NULL)
        return -1;
    free(inf->realpath);
    inf->realpath = newpath;
    return 0;
}

int inferior_set_args(struct inferior *inf, const char *args) {
    if (inf == NULL)
        return 0;
    if (args == NULL) {
        free(inf->args);
        inf->args = NULL;
        return 0;
    }
    char *newargs = strdup(args);
    if (newargs == NULL)
        return -1;
    free(inf->args);
    inf->args = newargs;
    return 0;
}

int inferior_continue(struct inferior *inf) {
    if (inf->status != INFERIOR_STOPPED)
        return -1;
    if (ptrace(PTRACE_CONT, inf->pid, NULL, NULL) < 0)
        return -1;
    inf->status = INFERIOR_RUNNING;
    return 0;
}
