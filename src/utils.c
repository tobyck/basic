#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include <math.h>

bool str_slice_eq(char *string, size_t slice_start, size_t slice_length, char *slice) {
	for (size_t i = slice_start; i < slice_start + slice_length; i++)
		if (string[i] != slice[i - slice_start])
			return false;

	return true;
}

char *alloc_empty_str() {
	char *string = malloc(1);

	if (string == NULL) {
		printf("Error: could not allocate memory for an empty string (alloc_empty_str)\n");
		exit(EXIT_FAILURE);
	}

	string[0] = '\0';
	return string;
}

void append_str(char **dest, char *src) {
	*dest = realloc(*dest, strlen(*dest) + strlen(src) + 1);
	strcat(*dest, src);
}

void append_char(char **dest, char ch) {
	size_t dest_len = strlen(*dest);
	*dest = realloc(*dest, dest_len + 2);
	(*dest)[dest_len] = ch;
	(*dest)[dest_len + 1] = '\0';
}

void append_str_and_free(char **dest, char *src) {
	*dest = realloc(*dest, strlen(*dest) + strlen(src) + 1);
	strcat(*dest, src);
	free(src);
}

char *alloc_num_as_str(size_t number) {
	int string_len;
	if (number == 0) string_len = 2;
	else string_len = (int)log10(number) + 2;

	char *string = malloc(string_len);

	if (string == NULL) {
		printf("Error: could not allocate memory for the string to write a number to (alloc_num_as_str)\n");
		exit(EXIT_FAILURE);
	}

	snprintf(string, string_len, "%zu\n", number);

	return string;
}

char *alloc_char_as_str(char ch) {
	char *string = calloc(2, sizeof(char));

	if (string == NULL) {
		printf("Error: could not allocate memory for string to write single char to (alloc_char_as_str)\n");
		exit(EXIT_FAILURE);
	}
	
	string[0] = ch;
	string[1] = '\0';

	return string;
}
