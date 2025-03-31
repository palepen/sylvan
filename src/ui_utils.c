#include <sys/ioctl.h>
#include <unistd.h>
#include <limits.h>
#include <stdlib.h>
#include <stdio.h>

#include "ui_utils.h"




/**
 * @brief Gets the current terminal width
 * @return Number of columns in terminal, or DEFAULT_TERM_WIDTH if undetectable
 */
int get_terminal_width(void)
{
    struct winsize w = {0};
    int fds[] = {STDOUT_FILENO, STDERR_FILENO};

    for (int i = 0; i < 2; i++)
    {
        if (isatty(fds[i]) && ioctl(fds[i], TIOCGWINSZ, &w) == 0 && w.ws_col > 0)
        {
            return w.ws_col;
        }
    }

    const char *env_cols = getenv("COLUMNS");
    if (env_cols)
    {
        char *end;
        long cols = strtol(env_cols, &end, 10);
        if (end != env_cols && *end == '\0' && cols > 0 && cols <= INT_MAX)
        {
            return (int)cols;
        }
    }

    return DEFAULT_TERM_WIDTH;
}

/**
 * @brief Clears the terminal screen
 */
void clear_screen(void)
{
    printf("\033[2J\033[H");
}
