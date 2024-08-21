#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include <math.h>
#include <ctype.h>

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
	const int MAX_FILE_SIZE = 500000000;

	if (file_length > MAX_FILE_SIZE) {
		printf("Error: file must be less than 0.5GB\n");
		exit(EXIT_FAILURE);
	}

	char *buffer = malloc(file_length + 1);

	if (buffer == NULL) {
		printf("Error: could not allocate buffer for file content\n");
		exit(EXIT_FAILURE);
	}

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

	if (string == NULL) {
		printf("Error: could not allocate memory for an empty string (alloc_empty_str)\n");
		exit(EXIT_FAILURE);
	}

	string[0] = '\0';
	return string;
}

void append_str(char **dest, char *src) {
	*dest = realloc(*dest, strlen(*dest) + strlen(src) + 1);

	if (*dest == NULL) {
		printf("Could not allocate memory to make space to concatenate a string (append_str)\n");
		exit(EXIT_FAILURE);
	}

	strcat(*dest, src);
}

void append_char(char **dest, char ch) {
	size_t dest_len = strlen(*dest);
	*dest = realloc(*dest, dest_len + 2);

	if (*dest == NULL) {
		printf("Could not allocate memory to make space to concatenate a char (append_char)\n");
		exit(EXIT_FAILURE);
	}

	(*dest)[dest_len] = ch;
	(*dest)[dest_len + 1] = '\0';
}

void append_str_and_free(char **dest, char *src) {
	*dest = realloc(*dest, strlen(*dest) + strlen(src) + 1);

	if (*dest == NULL) {
		printf("Could not allocate memory to make space to concatenate a string (append_str_and_free)\n");
		exit(EXIT_FAILURE);
	}

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
