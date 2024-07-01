#include <stdlib.h>
#include <stdio.h>

#include "lexer.h"

int main(int argc, char *argv[]) {
	if (argc > 2) {
		printf("Warning: extraneous arguments will be ignored.\n");
	} else if (argc == 1) {
		printf("Usage: tinybc [filename]\n");
		return EXIT_SUCCESS;
	}
	
	FILE *file_ptr = fopen(argv[1], "r");

	if (file_ptr == NULL) {
		printf("Error: could not read file %s\n", argv[0]);
		return EXIT_FAILURE;
	}
	
	// go to the end of the file, see how long it is, then go back
	fseek(file_ptr, 0, SEEK_END);
	size_t file_length = ftell(file_ptr);
	fseek(file_ptr, 0, SEEK_SET);

	// 0.5GB (probably don't want to load that into memory)
	const int MAX_FILE_SIZE = 500000000;

	if (file_length > MAX_FILE_SIZE) {
		printf("Error: file must be less than 0.5GB\n");
		return EXIT_FAILURE;
	}

	// read all content into memory
	char *buffer = malloc(file_length);

	if (buffer == NULL) {
		printf("Error: could not allocate buffer for file content\n");
		return EXIT_FAILURE;
	}

	size_t amount_read = fread(buffer, 1, file_length, file_ptr);
	fclose(file_ptr);

	if (amount_read != file_length) {
		printf("Error: did not manage to read the whole file.\n");
		return EXIT_FAILURE;
	}

	LexerResult tokens = lex(buffer, file_length);

	if (tokens.error != NULL) {
		printf("Lexer error: %s\n", tokens.error);
		return EXIT_FAILURE;
	} else {
		print_token_list(tokens.tokens);
		free_lexer_result(tokens);
	}
	
	free(buffer);

	return EXIT_SUCCESS;
}
