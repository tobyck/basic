#ifndef INCLUDE_UTILS_H
#define INCLUDE_UTILS_H

#include <stddef.h>
#include <stdbool.h>

extern void ensure_alloc(void *ptr);
extern char *read_file(char *path);
extern char *alloc_empty_str(void);
extern void append_str(char **dest, char *src);
extern void append_char(char **dest, char ch);
extern void append_str_and_free(char **dest, char *src);
extern char *num_as_str(size_t number);
extern char *char_as_str(char ch);

typedef struct {
	char *buffer;
	size_t capacity;
	size_t length;
	size_t step;
} BufferedString;

extern BufferedString empty_buffered_string(size_t step);
extern void buffered_string_append_char(BufferedString *str, char ch);

#endif  // INCLUDE_UTILS_H
