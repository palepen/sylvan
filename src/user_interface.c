#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <limits.h>
#include <string.h>
#include <signal.h>
#include <readline/readline.h>
#include <readline/history.h>

#include "user_interface.h"
#include "handle_command.h"

extern volatile sig_atomic_t interrupted;

static char **get_command(const char *prompt)
{
    if (prompt == NULL)
    {
        fprintf(stderr, "Error: Prompt is NULL\n");
        return NULL;
    }

    char *input = readline(prompt);

    if (interrupted)
    {
        free(input);
        return NULL;
    }

    if (input[0] != '\0')
    {
        add_history(input);
    }

    size_t arg_count = 0, arg_size = INITIAL_ARG_COUNT;
    char **args = malloc(arg_size * sizeof(char *));
    if (args == NULL)
    {
        perror("Error: malloc failed for args");
        free(input);
        return NULL;
    }

    char *token = strtok(input, " ");
    while (token != NULL)
    {
        args[arg_count] = strdup(token);
        if (args[arg_count] == NULL)
        {
            perror("Error: strdup failed");
            // Free all prior args
            for (size_t i = 0; i < arg_count; i++)
            {
                free(args[i]);
            }
            free(args);
            free(input);
            return NULL;
        }

        arg_count++;
        if (arg_count >= arg_size - 1)
        {
            arg_size *= 2;
            char **new_args = realloc(args, arg_size * sizeof(char *));
            if (new_args == NULL)
            {
                perror("Error: realloc failed");
                for (size_t i = 0; i < arg_count; i++)
                {
                    free(args[i]);
                }
                free(args);
                free(input);
                return NULL;
            }
            args = new_args;
        }
        token = strtok(NULL, " ");
    }

    args[arg_count] = NULL;
    if (input)
        free(input);

    return args;
}

/**
 * @brief Frees a dynamically allocated NULL-terminated array of strings.
 * @param args The array of strings to be freed.
 */
static void free_command(char **args)
{
    if (args == NULL)
        return;
    for (size_t i = 0; args[i] != NULL; i++)
    {
        if (args[i])
            free(args[i]);
    }
    free(args);
}

/**
 * @brief Clears the terminal screen
 */
static void clear_screen(void)
{
    printf("\033[2J\033[H");
}

/**
 * @brief Gets the current terminal width
 * @return Number of columns in terminal, or DEFAULT_TERM_WIDTH if undetectable
 */
static int get_terminal_width(void)
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
 * @brief Prints the debugger's heading banner
 */
static void print_heading(void)
{
    clear_screen();
    int width = get_terminal_width();
    const char *title = "SYLVAN DEBUGGER v1.0";
    int title_len = strlen(title);
    int left_pad = (width - title_len) / 2;
    int right_pad = width - title_len - left_pad;

    printf("\n%s╔", CYAN);
    for (int i = 0; i < width - 2; i++)
    {
        printf("═");
    }
    printf("╗%s\n", RESET);

    printf("%s%s%*s%s%*s%s\n",
           BOLD, MAGENTA,
           left_pad, "",
           title,
           right_pad, "",
           RESET);

    printf("%s╚", CYAN);
    for (int i = 0; i < width - 2; i++)
    {
        printf("═");
    }
    printf("╝%s\n\n", RESET);
}


static int event_hook(void)
{
    if (interrupted)
    {
        rl_done = 1; 
        rl_replace_line("", 0);
        rl_crlf();             
        rl_redisplay();
        return 1;
    }
    return 0;
}

/**
 * @brief Runs the main interactive loop for the Sylvan Inferior shell.
 *
 * This function sets up signal handling, initializes the Readline library,
 * configures the command prompt, and enters an interactive loop for command execution.
 * The loop continues until interrupted or an exit command is issued.
 *
 * @param[in,out] inf A pointer to a struct sylvan_inferior object, which maintains
 *                    state information for the shell.
 */
extern void interface_loop(struct sylvan_inferior **inf)
{


    print_heading();
    init_commands();

    rl_initialize();
    rl_catch_signals = 1;
    rl_set_signals();
    rl_event_hook = event_hook;

    char prompt[PROMPT_MAX_LEN];
    snprintf(prompt, sizeof(prompt),
             "%s[%ssylvan%s]%s➤%s ",
             BLUE, MAGENTA, BLUE, YELLOW, RESET);

    char **line = NULL;
    while (!interrupted)
    {
        line = get_command(prompt);
        if (line == NULL)
        {
            if (interrupted)
            {
                printf("Interrupted, exiting\n");
                break;
            }
            continue;
        }

        if (line[0] == NULL || line[0][0] == '\0')
        {
            free_command(line);
            continue;
        }

        if (handle_command(line, inf))
        {
            free_command(line);
            break;
        }
        free_command(line);
    }

    clear_history();
    rl_clear_history();
    rl_free_line_state();
    rl_cleanup_after_signal();
    rl_deprep_terminal();
}
