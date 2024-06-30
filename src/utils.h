#ifndef INCLUDE_UTILS_H
#define INCLUDE_UTILS_H

#include <stdbool.h>

extern bool str_slice_eq(char *string, size_t slice_start, size_t slice_length, char *slice);
extern char *alloc_empty_str();
extern void append_str(char **dest, char *src);
extern void append_char(char **dest, char ch);
extern void append_str_and_free(char **dest, char *src);
extern char *alloc_num_as_str(size_t number);
extern char *alloc_char_as_str(char ch);

#endif  // INCLUDE_UTILS_H
