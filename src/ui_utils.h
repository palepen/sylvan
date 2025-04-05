#ifndef TERM_H
#define TERM_H

#define RESET "\033[0m"
#define BLACK "\033[30m"
#define RED "\033[31m"
#define GREEN "\033[32m"
#define YELLOW "\033[33m"
#define BLUE "\033[34m"
#define MAGENTA "\033[35m"
#define CYAN "\033[36m"
#define WHITE "\033[37m"
#define BOLD "\033[1m"
#define BG_BLACK "\033[40m"
#define BG_RED "\033[41m"
#define BG_GREEN "\033[42m"
#define BG_BLUE "\033[44m"
#define GRAY "\e[38;2;128;128;128m"
#define BRIGHT_RED "\033[91m"
#define BRIGHT_GREEN "\033[92m"
#define BRIGHT_BLUE "\033[94m"
#define LIGHT_GREEN "\033[38;5;65m"

#define TERM_WIDTH_FALLBACK 80
#define TERM_HEIGHT_FALLBACK 24
#define BORDER_COLOR WHITE

/**
 * @brief Column format types
 */
enum table_col_format
{
    TABLE_COL_STR,
    TABLE_COL_INT,
    TABLE_COL_HEX,
    TABLE_COL_HEX_LONG
};

struct term_size
{
    int width;
    int height;
};

/**
 * @brief Column definitions
 */
struct table_col
{
    const char *header;           /**> Column header text */
    int width;                    /**> Desired width (including padding) */
    enum table_col_format format; /**> Data format */
};
/**
 * @brief Table data structure (one row)
 */
struct table_row
{
    const void *data;       /**> Pointer to row data (cast based on format) */
    struct table_row *next; /**> Linked list for rows */
};

struct term_size get_terminal_size(void);
extern void clear_screen(void);

/**
 * @brief Generic table printing function
 */
void print_table(const char *title, struct table_col *cols, int col_count, struct table_row *rows, int row_count);

#endif