#ifndef INTERFACE_H
#define INTERFACE_H
#include "sylvan/inferior.h"

#define PROMPT_MAX_LEN 50     /**< Maximum length of command prompt */
#define INITIAL_BUFFER_SIZE 128
#define INITIAL_ARG_COUNT 10


extern void interface_loop(struct sylvan_inferior **inf);

#endif