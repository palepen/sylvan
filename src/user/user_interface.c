#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <limits.h>
#include <string.h>

#include <sylvan/user_interface.h>
#include <sylvan/command_handler.h>

#define DEFAULT_TERM_WIDTH 80 /**< Default terminal width if detection fails */
#define PROMPT_MAX_LEN 50     /**< Maximum length of command prompt */
int history_count = 0;

struct command_history *history;

/**
 * @brief Reads a line of input from the user dynamically.
 * @param prompt The prompt string displayed before input.
 * @return A dynamically allocated string containing the user's input (must be freed).
 */
static char *get_command(const char *prompt)
{
    if (prompt)
        printf("%s", prompt);

    size_t bufsize = INITIAL_BUFFER_SIZE;
    char *buffer = malloc(bufsize);
    if (!buffer)
    {
        perror("Error: malloc error");
        return NULL;
    }

    size_t pos = 0;
    int c;

    while (1)
    {
        c = getchar();
        if (c == EOF || c == '\n')
        {
            buffer[pos] = '\0'; // Null-terminate the string
            break;
        }

        // Handle backspace
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

    }

    return buffer;
}

/**
 * @brief Frees all stored command history.
 */
static void free_history()
{
    while(history != NULL)
    {
        struct command_history *curr = history;
        history = history->next;
        free(curr->command);
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
 * @brief Adds the current command to the history
 * @return 0 if success or 1 if error
 * @param[in] command current command given
 */
int add_history(char *command)
{
    if(!command)
    {
        fprintf(stderr, "invalid command given\n");
        return 1;
    }
    struct command_history* new_command = (struct command_history*)malloc(sizeof(struct command_history));
    size_t len = strlen(command);
    new_command->command = (char*)malloc(sizeof(len));
    
    if(!new_command || !new_command->command)
    {
        perror("Warning: malloc for add history failed");
        return 1;
    }   
    new_command->next = NULL;
    
    if(!history) 
    {
        history = new_command;
        return 0;        
    }

    while(history->next != NULL)
    {
        history = history->next;
    }
    history->next = new_command;
    return 0;
}


/**
 * @brief Main debugger interface loop
 * @details Provides a command-line interface for debugger operations
 * @param[in,out] proc Pointer to the debug process structure
 */
extern void interface_loop(struct inferior *inf)
{

    if (!inf)
    {
        fprintf(stderr, "Error: Invalid process structure\n");
        return;
    }

    print_heading();

    char prompt[PROMPT_MAX_LEN];
    snprintf(prompt, sizeof(prompt),
             "%s[%ssylvan%s]%s➤%s ",
             BLUE, MAGENTA, BLUE, YELLOW, RESET);

    char *line;
    while ((line = get_command(prompt)) != NULL)
    {

        if(strcmp(line, "") == 0)
        {
            free(line);
            continue;
        }

        if(add_history(line))
        {
            break;   
        }

        if (handle_command(line, inf))
        {
            free(line);
            break;
        }
        free(line);
    }

    if (!line)
    { 
        printf("\n");
    }

    free_history();
}