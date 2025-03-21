#ifndef SYLVAN_ERROR_H
#define SYLVAN_ERROR_H

#include <sylvan/sylvan.h>

const char *sylvan_strerror(sylvan_code_t code);
sylvan_code_t sylvan_set_code(sylvan_code_t code);
sylvan_code_t sylvan_set_message(sylvan_code_t code, const char *fmt, ...);
sylvan_code_t sylvan_set_errno(sylvan_code_t code);
sylvan_code_t sylvan_set_errno_msg(sylvan_code_t code, const char *fmt, ...);

#endif /* SYLVAN_ERROR_H */
