#ifndef SYLVAN_ERROR_H
#define SYLVAN_ERROR_H

#include <sylvan/sylvan.h>


/* returns a human readable description of error code */
const char *sylvan_strerror(sylvan_code_t code);

/* set error with just an error code */
sylvan_code_t sylvan_set_code(sylvan_code_t code);

/* set error with code and custom message */
sylvan_code_t sylvan_set_message(sylvan_code_t code, const char *fmt, ...);

/* set error with code and current errno value */
sylvan_code_t sylvan_set_errno(sylvan_code_t code);

/* set error with code, custom message prefix, and errno details */
sylvan_code_t sylvan_set_errno_msg(sylvan_code_t code, const char *fmt, ...);


#endif /* SYLVAN_ERROR_H */
