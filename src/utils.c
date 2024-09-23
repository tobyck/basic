#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include <math.h>
#include <ctype.h>

#include "utils.h"

void ensure_alloc(void *ptr) {
	if (ptr == NULL) {
		printf("Error: could not allocate memory\n");
		exit(EXIT_FAILURE);
	}
}

char *read_file(char *path) {
	FILE *file = fopen(path, "r");

	if (file == NULL) {
		printf("Error: could not read file %s\n", path);
		exit(EXIT_FAILURE);
	}

	// go to the end of the file, see how long it is, then go back
	fseek(file, 0, SEEK_END);
	size_t file_length = ftell(file);
	fseek(file, 0, SEEK_SET);

	// 0.5GB (probably don't want to load that into memory)
	const size_t MAX_FILE_SIZE = 500000000;

	if (file_length > MAX_FILE_SIZE) {
		printf("Error: file must be less than 0.5GB\n");
		exit(EXIT_FAILURE);
	}

	char *buffer = malloc(file_length + 1);
	ensure_alloc(buffer);

	// read file into memory
	size_t amount_read = fread(buffer, 1, file_length, file);
	fclose(file);

	buffer[file_length] = '\0';

	if (amount_read != file_length) {
		printf("Error: did not manage to read the whole file.\n");
		exit(EXIT_FAILURE);
	}

	return buffer;
}

char *alloc_empty_str() {
	char *string = malloc(1);
	ensure_alloc(string);
	string[0] = '\0';
	return string;
}

void append_str(char **dest, char *src) {
	*dest = realloc(*dest, strlen(*dest) + strlen(src) + 1);
	ensure_alloc(*dest);
	strcat(*dest, src);
}

void append_char(char **dest, char ch) {
	size_t dest_len = strlen(*dest);
	*dest = realloc(*dest, dest_len + 2);
	ensure_alloc(*dest);

	(*dest)[dest_len] = ch;
	(*dest)[dest_len + 1] = '\0';
}

void append_str_and_free(char **dest, char *src) {
	*dest = realloc(*dest, strlen(*dest) + strlen(src) + 1);
	ensure_alloc(*dest);
	strcat(*dest, src);
	free(src);
}

char *num_as_str(size_t number) {
	size_t string_len;
	if (number == 0) string_len = 2;
	else string_len = (int)log10(number) + 2;

	char *string = malloc(string_len);
	ensure_alloc(string);
	snprintf(string, string_len, "%zu\n", number);

	return string;
}

char *char_as_str(char ch) {
	return (char[]){ ch, '\0' };
}

BufferedString empty_buffered_string(size_t step) {
	char *buffer = malloc(step + 1);
	ensure_alloc(buffer);
	buffer[0] = '\0';
	return (BufferedString){ buffer, step, 0, step };
}

void buffered_string_append_char(BufferedString *str, char ch) {
	if (str->length == str->capacity) {
		// allocate an extra <step> bytes and add one for null byte
		str->buffer = realloc(str->buffer, (str->capacity / str->step + 1) * str->step + 1);
		str->capacity += str->step;
	}

	str->buffer[str->length++] = ch;
	str->buffer[str->length] = '\0';
}
