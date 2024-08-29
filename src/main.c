#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>

#include "utils.h"
#include "lexer.h"
#include "parser.h"
#include "debug.h"

int main(int argc, char *argv[]) {
	if (argc > 2) {
		printf("Warning: extraneous arguments will be ignored\n");
	} else if (argc == 1) {
		printf("Usage: basic [filename]\n");
		return EXIT_SUCCESS;
	}
	
	char *code = read_file(argv[1]);
	bool printing_enabled = true;
	
	LexerResult tokens = lex(code);

	if (printing_enabled) {
		printf("valid tokens: ");
		print_token_linked_list(tokens.valid);
		printf("invalid tokens: ");
		print_token_linked_list(tokens.invalid);
		printf("lexer errors: ");
		print_lexer_errors(tokens.errors);
	}

	free(code);
	free_token_linked_list(&tokens.invalid);
	free(tokens.errors.errors);

	ParserResult parser_result = parse(tokens.valid);

	if (printing_enabled) {
		printf("ast: ");
		print_ast(parser_result.ast);
		printf("parse errors: ");
		print_parse_errors(parser_result.errors);
	}

	free_ast(parser_result.ast);
	free_parse_error_list(parser_result.errors);
	
	return EXIT_SUCCESS;
}
