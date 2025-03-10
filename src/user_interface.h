#ifndef INTERFACE_H
#define INTERFACE_H
#include <sylvan/inferior.h>

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
#define MAX_HISTORY 100         // Maximum stored commands

#define DEFAULT_TERM_WIDTH 80 /**< Default terminal width if detection fails */
#define PROMPT_MAX_LEN 50     /**< Maximum length of command prompt */
#define INITIAL_BUFFER_SIZE 128
#define INITIAL_ARG_COUNT 10

struct command_history
{
    char **command;
    size_t arg_count;
    struct command_history* next;
};

extern void interface_loop(struct sylvan_inferior *inf);

#endif