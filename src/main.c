#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>

#include "lexer.h"
#include "utils.h"

int main(int argc, char *argv[]) {
	if (argc > 2) {
		printf("Warning: extraneous arguments will be ignored.\n");
	} else if (argc == 1) {
		printf("Usage: tinybc [filename]\n");
		return EXIT_SUCCESS;
	}
	
	char *code = read_file(argv[1]);
	
	LexerResult tokens = lex(code);

	print_token_list(tokens.valid);
	print_token_list(tokens.invalid);
	print_lexer_errors(tokens.errors);

	free(code);
	free_lexer_result(tokens);
	
	return EXIT_SUCCESS;
}
