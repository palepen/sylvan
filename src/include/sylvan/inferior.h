#ifndef INFERIOR_H
#define INFERIOR_H

#include <stdbool.h>
#include <sys/types.h>

enum inferior_state {
    INFERIOR_NONE,
    INFERIOR_TERMINATED,
    INFERIOR_RUNNING,
    INFERIOR_EXITED,
    INFERIOR_STOPPED,
};

struct inferior {
    int id;
    pid_t pid;
    enum inferior_state status;
    char *realpath;
    char *args;
    bool is_attached;
};

struct inferior *inferior_create();
int inferior_detach(struct inferior *inf);
int inferior_kill(struct inferior *inf);
int inferior_attach_pid(struct inferior *inf, pid_t pid);
int inferior_destroy(struct inferior *inf);
int inferior_run(struct inferior *inf);
int inferior_set_filepath(struct inferior *inf, const char *filepath);
int inferior_set_args(struct inferior *inf, const char *args);
int inferior_continue(struct inferior *inf);

#endif
