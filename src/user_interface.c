#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <limits.h>
#include <string.h>
#include <readline/readline.h>
#include <readline/history.h>

#include "user_interface.h"
#include "handle_command.h"

int history_count = 0;
struct command_history *history;

static char **get_command(const char *prompt)
{
    // Read input using readline
    char *input = readline(prompt);
    if (!input) {  // EOF (Ctrl+D) or allocation failure
        return NULL;
    }

    // Add to readline's history (replaces custom add_history)
    if (input[0] != '\0') {
        add_history(input);
        history_count++;
    }

    // Tokenize input into an array of strings
    size_t arg_count = 0, arg_size = INITIAL_ARG_COUNT;
    char **args = malloc(arg_size * sizeof(char *));
    if (!args) {
        perror("Error: malloc failed");
        free(input);
        return NULL;
    }

    char *token = strtok(input, " ");
    while (token) {
        args[arg_count++] = strdup(token);

        // Expand args array if needed
        if (arg_count >= arg_size - 1) {
            arg_size *= 2;
            args = realloc(args, arg_size * sizeof(char *));
            if (!args) {
                perror("Error: realloc failed");
                free(input);
                return NULL;
            }
        }

        token = strtok(NULL, " ");
    }

    args[arg_count] = NULL; 
    free(input);            

    return args;
}

/**
 * @brief Frees a dynamically allocated NULL-terminated array of strings.
 * @param args The array of strings to be freed.
 */
static void free_command(char **args)
{
    if (!args) return;

    for (size_t i = 0; args[i] != NULL; i++) {
        free(args[i]);
    }
    free(args);
}

/**
 * @brief Clears the terminal screen
 */
static void clear_screen(void)
{
    printf("\033[2J\033[H"); // ANSI escape: clear screen and move cursor to top
}

/**
 * @brief Gets the current terminal width
 * @return Number of columns in terminal, or DEFAULT_TERM_WIDTH if undetectable
 */
static int get_terminal_width(void)
{
    struct winsize w = {0};
    int fds[] = {STDOUT_FILENO, STDERR_FILENO};

    // Try to get width from stdout or stderr
    for (int i = 0; i < 2; i++)
    {
        if (isatty(fds[i]) && ioctl(fds[i], TIOCGWINSZ, &w) == 0 && w.ws_col > 0)
        {
            return w.ws_col;
        }
    }

    // Fallback to COLUMNS environment variable
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

    // Top border
    printf("\n%s╔", CYAN);
    for (int i = 0; i < width - 2; i++)
    {
        printf("═");
    }
    printf("╗%s\n", RESET);

    // Title with padding
    printf("%s%s%*s%s%*s%s\n",
           BOLD, MAGENTA,
           left_pad, "",
           title,
           right_pad, "",
           RESET);

    // Bottom border
    printf("%s╚", CYAN);
    for (int i = 0; i < width - 2; i++)
    {
        printf("═");
    }
    printf("╝%s\n\n", RESET);
}


/**
 * @brief Main debugger interface loop
 * @details Provides a command-line interface for debugger operations
 * @param[in,out] proc Pointer to the debug process structure
 */
extern void interface_loop(struct sylvan_inferior **inf)
{
    print_heading();
    init_commands();

    char prompt[PROMPT_MAX_LEN];
    snprintf(prompt, sizeof(prompt),
             "%s[%ssylvan%s]%s➤%s ",
             BLUE, MAGENTA, BLUE, YELLOW, RESET);

    char **line;
    while ((line = get_command(prompt)) != NULL)
    {
        if (strcmp(line[0], "") == 0)
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

    if (!line)
    {
        printf("\n");
    }
}