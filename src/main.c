#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <string.h>

#include "utils.h"
#include "debug.h"
#include "lexer.h"

int main(int argc, char *argv[]) {
	if (argc > 2) {
		printf("Warning: extraneous arguments will be ignored\n");
	} else if (argc == 1) {
		printf("Usage: basic [filename]\n");
		return EXIT_SUCCESS;
	}
	
	char *code = read_file(argv[1]);

	Lexer *lexer = new_lexer(code, 3);
	peek_token(lexer);
	peek_token(lexer);
	next_token(lexer);
	next_token(lexer);
	print_token_buffer(lexer->tokens);
	free_lexer(lexer);
	free(code);

	return EXIT_SUCCESS;
}
