#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <string.h>

#include "utils.h"
#include "parser.h"

int main(int argc, char *argv[]) {
	if (argc > 2) {
		printf("Warning: extraneous arguments will be ignored\n");
	} else if (argc == 1) {
		printf("Usage: basic [filename]\n");
		return EXIT_SUCCESS;
	}
	
	char *code = read_file(argv[1]);

	ParserResult parser_result = parse(code);

	if (parser_result.success) {
		free(parser_result.result.ast.statements);
	} else {
		printf("uh oh\n");
	}

	free(code);

	return EXIT_SUCCESS;
}
