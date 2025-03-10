#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <limits.h>
#include <string.h>

#include "user_interface.h"
#include "handle_command.h"

int history_count = 0;
struct command_history *history;


/**
 * @brief Reads a command from standard input and splits it into an array of arguments.
 * @param prompt The prompt message displayed before reading input (can be NULL).
 * @return A dynamically allocated NULL-terminated array of strings (command and arguments).
 *         Returns NULL on memory allocation failure.
 * @note The caller is responsible for freeing the returned array and its elements.
 */
static char **get_command(const char *prompt)
{
    if (prompt)
        printf("%s", prompt);

    size_t bufsize = INITIAL_BUFFER_SIZE;
    char *buffer = malloc(bufsize);
    if (!buffer)
    {
        perror("Error: malloc failed");
        return NULL;
    }

    size_t pos = 0;
    int c;

    while (1)
    {
        c = getchar();
        if (c == EOF || c == '\n')
        {
            buffer[pos] = '\0'; // Null-terminate input
            break;
        }

        // Handle backspace (ASCII 127 or '\b')
        if (c == 127 || c == '\b')
        {
            if (pos > 0)
            {
                pos--;
                printf("\b \b"); // Move cursor back, clear character
                fflush(stdout);
            }
            continue;
        }

        buffer[pos++] = c;

        // Expand buffer if needed
        if (pos >= bufsize - 1)
        {
            bufsize *= 2;
            buffer = realloc(buffer, bufsize);
            if (!buffer)
            {
                perror("Error: realloc failed");
                return NULL;
            }
        }
    }

    // Tokenize input into an array of strings
    size_t arg_count = 0, arg_size = INITIAL_ARG_COUNT;
    char **args = malloc(arg_size * sizeof(char *));
    if (!args)
    {
        perror("Error: malloc failed");
        free(buffer);
        return NULL;
    }

    char *token = strtok(buffer, " ");
    while (token)
    {
        args[arg_count++] = strdup(token);

        // Expand args array if needed
        if (arg_count >= arg_size - 1)
        {
            arg_size *= 2;
            args = realloc(args, arg_size * sizeof(char *));
            if (!args)
            {
                perror("Error: realloc failed");
                free(buffer);
                return NULL;
            }
        }

        token = strtok(NULL, " ");
    }

    args[arg_count] = NULL; // NULL-terminate argument list
    free(buffer);           // Free original input buffer

    return args;
}

/**
 * @brief Frees a dynamically allocated NULL-terminated array of strings.
 *
 * @param args The array of strings to be freed.
 */
static void free_command(char **args)
{
    if (!args)
        return;

    for (size_t i = 0; args[i] != NULL; i++)
        free(args[i]);

    free(args);
}

/**
 * @brief Frees all stored command history.
 */
static void free_history()
{
    while (history != NULL)
    {
        struct command_history *curr = history;
        history = history->next;
        free_command(curr->command);
        free(curr);
    }

    history_count = 0;
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
 * @brief Adds a command with arguments to the command history.
 *
 * @param command A NULL-terminated array of strings (command + arguments).
 * @return 0 on success, 1 on failure.
 */
static int add_history(char **command)
{
    if (!command || !command[0])
    {
        fprintf(stderr, "Invalid command given\n");
        return 1;
    }

    // Count arguments
    int arg_count = 0;
    while (command[arg_count] != NULL)
    {
        arg_count++;
    }

    // Allocate memory for new history entry
    struct command_history *new_command = malloc(sizeof(struct command_history));
    if (!new_command)
    {
        perror("Warning: malloc for add_history failed");
        return 1;
    }

    // Allocate memory for argument list
    new_command->command = malloc((arg_count + 1) * sizeof(char *));
    if (!new_command->command)
    {
        perror("Warning: malloc for command array failed");
        free(new_command);
        return 1;
    }

    // Copy each argument
    for (int i = 0; i < arg_count; i++)
    {
        new_command->command[i] = strdup(command[i]);
        if (!new_command->command[i])
        {
            perror("Warning: malloc for command string failed");
            // Free allocated memory before returning
            for (int j = 0; j < i; j++)
                free(new_command->command[j]);
            free(new_command->command);
            free(new_command);
            return 1;
        }
    }
    new_command->command[arg_count] = NULL; // Null-terminate the array
    new_command->arg_count = arg_count;
    new_command->next = NULL;

    // If history is empty, set this as the first command
    if (!history)
    {
        history = new_command;
        return 0;
    }

    // Traverse to the end of the history list
    struct command_history *current = history;
    while (current->next != NULL)
    {
        current = current->next;
    }

    // Append new command to history
    current->next = new_command;
    return 0;
}

/**
 * @brief Main debugger interface loop
 * @details Provides a command-line interface for debugger operations
 * @param[in,out] proc Pointer to the debug process structure
 */
extern void interface_loop(struct sylvan_inferior *inf)
{

    if (!inf)
    {
        fprintf(stderr, "Error: Invalid process structure\n");
        return;
    }

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

        if (add_history(line))
        {
            break;
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

    free_history();
}