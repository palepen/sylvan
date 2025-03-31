#ifndef UI_UTILS_H
#define UI_UTILS_H

#define RESET   "\033[0m"
#define BLACK   "\033[30m"
#define RED     "\033[31m"
#define GREEN   "\033[32m"
#define YELLOW  "\033[33m"
#define BLUE    "\033[34m"
#define MAGENTA "\033[35m"
#define CYAN    "\033[36m"
#define WHITE   "\033[37m"
#define BOLD    "\033[1m"
#define BG_BLACK   "\033[40m"
#define BG_RED     "\033[41m"
#define BG_GREEN   "\033[42m"
#define BG_BLUE    "\033[44m"
#define GRAY    "\e[38;2;128;128;128m"
#define DEFAULT_TERM_WIDTH 80 /**< Default terminal width if detection fails */


extern int get_terminal_width(void);
extern void clear_screen(void);


#endif