#ifndef SYLVAN_SYMBOL_H
#define SYLVAN_SYMBOL_H

#include <sylvan/symbol.h>

sylvan_code_t sylvan_sym_init(struct sylvan_inferior *inf);

sylvan_code_t sylvan_sym_destroy(struct sylvan_inferior *inf);

sylvan_code_t sylvan_sym_load_tables(struct sylvan_inferior *inf);

sylvan_code_t sylvan_get_function_addr(struct sylvan_inferior *inf, const char *name, uintptr_t *addr);

#endif /* SYLVAN_SYMBOL_H */
