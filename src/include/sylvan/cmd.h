#ifndef CMD_H
#define CMD_H

#include <stdbool.h>
#include <sys/types.h>


struct cmd_args {
    const char *file_args;
    const char *filepath;
    bool is_attached;
    pid_t pid;
};

#endif
