#ifndef AUXV_H
#define AUXV_H

#include "sylvan/inferior.h"


struct auxv_entry {
    long type;
    unsigned long value;
};


struct auxv_name {
    long type;
    const char *name;
    const char *desc;
};

unsigned char *target_read_auxv(struct sylvan_inferior *inf, size_t *len);
struct auxv_entry *parse_auxv(const unsigned char *data, size_t len, int is_64bit);
void print_auxv_entry(struct auxv_entry *entry);

#endif