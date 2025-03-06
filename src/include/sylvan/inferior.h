#ifndef INFERIOR_H
#define INFERIOR_H

#include <sylvan/cmd.h>


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
    enum inferior_state state;
    char *realpath;
    char *args;
    bool is_attached;
};

struct inferior *inferior_create();
int inferior_detach(struct inferior *inf);
int inferior_kill(struct inferior *inf);
int inferior_attach_pid(struct inferior *inf, pid_t pid);
int inferior_destroy(struct inferior *inf);

#endif
