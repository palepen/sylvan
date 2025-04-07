#ifndef SYLVAN_INCLUDE_SYLMBOL_H
#define SYLVAN_INCLUDE_SYLMBOL_H

#include <stdint.h>
#include <stddef.h>

struct symbol {
    char *name;
    uintptr_t addr;
};

struct sylvan_sym_table {
    struct symbol *symbols;
    size_t count;
    size_t capacity;
};

#endif /* SYLVAN_INCLUDE_SYLMBOL_H */
