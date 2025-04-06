#include <sys/ioctl.h>
#include <unistd.h>
#include <limits.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "ui_utils.h"

/**
 * @brief Gets the current terminal dimension
 * @return Number of columns in terminal, or DEFAULT_TERM_WIDTH if undetectable
 */
struct term_size get_terminal_size()
{
    struct term_size size = {TERM_WIDTH_FALLBACK, TERM_HEIGHT_FALLBACK};
    struct winsize w = {0};
    int fds[] = {STDOUT_FILENO, STDERR_FILENO};

    for (int i = 0; i < 2; i++)
    {
        if (isatty(fds[i]) && ioctl(fds[i], TIOCGWINSZ, &w) == 0)
        {
            if (w.ws_col > 0)
                size.width = w.ws_col;
            if (w.ws_row > 0)
                size.height = w.ws_row;
            return size;
        }
    }

    const char *env_cols = getenv("COLUMNS");
    const char *env_rows = getenv("LINES");
    if (env_cols)
    {
        char *end;
        long cols = strtol(env_cols, &end, 10);
        if (end != env_cols && *end == '\0' && cols > 0 && cols <= INT_MAX)
        {
            size.width = (int)cols;
        }
    }
    if (env_rows)
    {
        char *end;
        long rows = strtol(env_rows, &end, 10);
        if (end != env_rows && *end == '\0' && rows > 0 && rows <= INT_MAX)
        {
            size.height = (int)rows;
        }
    }
    return size;
}

/**
 * @brief Clears the terminal screen
 */
void clear_screen(void)
{
    printf("\033[2J\033[H");
}

static void print_table_line(int *widths, int col_count, char left, char mid, char right)
{
    printf("%s%c", BORDER_COLOR, left);
    for (int i = 0; i < col_count; i++)
    {
        for (int j = 0; j < widths[i]; j++)
            printf("-");
        if (i < col_count - 1)
            printf("%c", mid);
    }
    printf("%c%s\n", right, RESET);
}

static int prompt_for_continue(int current_row, int total_rows)
{
    printf("%s-- More -- (%d/%d rows) [Press Enter to continue, q to quit] %s", YELLOW, current_row, total_rows, RESET);
    fflush(stdout);
    int c = getchar();
    if (c == 'q' || c == 'Q')
        return 0;
    while (c != '\n' && c != EOF)
        c = getchar();
    
    printf("\033[1A\033[2K");
    fflush(stdout);
    return 1;
}

void print_table(const char *title, struct table_col *cols, int col_count, struct table_row *rows, int row_count)
{
    if (!title || !cols || col_count <= 0)
        return;

    struct term_size term = get_terminal_size();
    int page_size = term.height - 12; // Reserve for title, headers, prompt
    int total_width = 0;
    int term_width = term.width;
    int widths[col_count];

    // Calculate column widths (use specified width or header length, whichever is larger)
    for (int i = 0; i < col_count; i++)
    {
        widths[i] = cols[i].width > (int)strlen(cols[i].header) ? (int)cols[i].width : (int)strlen(cols[i].header);
        total_width += widths[i] + 1; // +1 for separator
    }

    total_width += 1; // Left border
    if (total_width > term_width)
    {
        // Scale down proportionally if too wide (simplified; could be more sophisticated)
        float scale = (float)term_width / total_width;
        for (int i = 0; i < col_count; i++)
            widths[i] = (int)(widths[i] * scale);
        total_width = term_width;
    }

    // Print title
    printf("\n %s%s%s%s \n", BOLD, CYAN, title, RESET);

    // Top border
    print_table_line(widths, col_count, '+', '+', '+');

    // Headers
    printf("%s│", BORDER_COLOR);
    for (int i = 0; i < col_count; i++)
    {
        printf(" %s%s%-*s%s", BOLD, YELLOW, widths[i] - 1, cols[i].header, RESET);
        if (i < col_count - 1)
            printf("%s│", BORDER_COLOR);
    }
    printf("%s│%s\n", BORDER_COLOR, RESET);

    // Middle separator
    print_table_line(widths, col_count, '+', '+', '+');

    // Rows
    struct table_row *row = rows;
    int rows_printed = 0;
    for (int r = 0; r < row_count && row; r++)
    {
        printf("%s|%s", BORDER_COLOR, WHITE);
        const char *data = (const char *)row->data;
        char buffer[32];
        for (int i = 0; i < col_count; i++)
        {
            switch (cols[i].format)
            {
            case TABLE_COL_STR:
                printf(" %s%-*s%s", GREEN, widths[i] - 1, *(const char **)data, RESET);
                data += sizeof(char *);
                break;
            case TABLE_COL_INT:
                printf(" %-*d", widths[i] - 1, *(int *)data);
                data += sizeof(int);
                break;
            case TABLE_COL_HEX:
                snprintf(buffer, sizeof(buffer), "0x%x", *(unsigned int *)data);
                printf(" %-*s", widths[i] - 1, buffer);
                data += sizeof(unsigned int);
                break;
            case TABLE_COL_HEX_LONG:
                snprintf(buffer, sizeof(buffer), "0x%lx", *(unsigned long *)data);
                printf(" %-*s", widths[i] - 1, buffer);
                data += sizeof(unsigned long);
                break;
            }
            if (i < col_count - 1)
                printf("%s|%s", BORDER_COLOR, WHITE);
        }
        printf("%s|%s\n", BORDER_COLOR, RESET);
        rows_printed++;

        if (rows_printed % page_size == 0 && r < row_count - 1)
        {
            if (!prompt_for_continue(rows_printed, row_count))
                break;
            print_table_line(widths, col_count, '+', '+', '+');
        }
        else if (r < row_count - 1)
        {
            print_table_line(widths, col_count, '+', '+', '+');
        }
        row = row->next;
    }

    print_table_line(widths, col_count, '+', '+', '+');
}