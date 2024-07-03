#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>

#include "lexer.h"
#include "utils.h"

TokenList empty_token_list() {
	Token *tokens = malloc(0);

	if (tokens == NULL) {
		printf("Error: could not allocate memory for token list\n");
		exit(EXIT_FAILURE);
	}

	return (TokenList){ tokens, 0 };
}

void free_token(Token token) {
	if (token.content != NULL) {
		free(token.content);
	}
}

void free_token_list(TokenList tokens) {
	for (size_t i = 0; i < tokens.length; i++) {
		free_token(tokens.tokens[i]);
	}

	free(tokens.tokens);
}

void push_token(TokenList *list, Token token) {
	list->tokens = realloc(list->tokens, sizeof(Token) * (list->length + 1));

	if (list->tokens == NULL) {
		printf("Could not reallocate memory for a new token\n");
		exit(EXIT_FAILURE);
	}

	list->tokens[list->length++] = token;
}

char *stringify_token_type(TokenType token_type) {
	switch (token_type) {
		case TOKEN_LET: return "LET";
		case TOKEN_VAR: return "VAR";
		case TOKEN_EQ: return "EQ";
		case TOKEN_INT: return "INT";
		case TOKEN_ADD: return "ADD";
		case TOKEN_SUB: return "SUB";
		case TOKEN_MUL: return "MUL";
		case TOKEN_DIV: return "DIV";
		case TOKEN_MOD: return "MOD";
		case TOKEN_OPEN_PAREN: return "OPEN_PAREN";
		case TOKEN_CLOSE_PAREN: return "CLOSE_PAREN";
		case TOKEN_END_STATEMENT: return "END_STATEMENT";
	}
}

char *stringify_token(Token token) {
	char *string = alloc_empty_str();

	append_str(&string, "{ type: ");
	append_str(&string, stringify_token_type(token.type));
	append_str(&string, ", content: ");
	
	if (token.content == NULL) {
		append_str(&string, "NULL");
	} else {
		append_str(&string, "\"");
		append_str(&string, token.content);
		append_str(&string, "\"");
	}

	append_str(&string, " line: ");
	append_str_and_free(&string, alloc_num_as_str(token.line));
	append_str(&string, ",");

	append_str(&string, " column: ");
	append_str_and_free(&string, alloc_num_as_str(token.column));

	append_str(&string, " }");

	return string;
}

void print_token_list(TokenList list) {
	printf("[");

	for (size_t i = 0; i < list.length; i++) {
		char *stringified_token = stringify_token(list.tokens[i]);
		printf("\n  %s", stringified_token);
		free(stringified_token);
		if (i < list.length - 1) printf(",");
		else printf("\n]\n");
	}
}

LexerResult lex(char *code, size_t code_length) {
	TokenList tokens = empty_token_list();

	size_t line = 1;
	size_t column_start = 0;

	for (size_t i = 0; i < code_length; i++) {
		char ch = code[i];
		
		if (ch == '\n') {
			// only push token if there are already some tokens and the previous token wasn't the same
			if (tokens.length != 0 && tokens.tokens[tokens.length - 1].type != TOKEN_END_STATEMENT)
				push_token(&tokens, (Token){ TOKEN_END_STATEMENT, NULL, line, i - column_start + 1 });

			line++;
			column_start = i + 1;
		} else if (ch == ' ' || ch == '\t') {
			continue;
		} else if (is_multi_char_token(code, i, 3, "REM")) {
			for (; code[i + 1] != '\n'; i++);
		} else if (is_multi_char_token(code, i, 3, "LET")) {
			push_token(&tokens, (Token){ TOKEN_LET, NULL, line, i - column_start + 1 });
			i += 2;
		} else if (strchr(SIMPLE_TOKENS, ch)) {
			TokenType type;

			switch (ch) {
				case '=': type = TOKEN_EQ; break;
				case '+': type = TOKEN_ADD; break;
				case '-': type = TOKEN_SUB; break;
				case '*': type = TOKEN_MUL; break;
				case '/': type = TOKEN_DIV; break;
				case '%': type = TOKEN_MOD; break;
				case '(': type = TOKEN_OPEN_PAREN; break;
				case ')': type = TOKEN_CLOSE_PAREN; break;
				default:
					return (LexerResult){ tokens, .error = strdup("unknown operator encounterd from SIMPLE_TOKENS (internal error)") };
			}

			push_token(&tokens, (Token){ type, NULL, line, i - column_start + 1 });
		} else if (isdigit(ch)) {
			char *int_as_str = alloc_empty_str();
			size_t column = i - column_start + 1;

			for (; isdigit(code[i]); i++) append_char(&int_as_str, code[i]);
			i--; // once we reach a non-digit we need to go back to avoid skipping the next char

			push_token(&tokens, (Token){ TOKEN_INT, int_as_str, line, column });
		} else if (isalpha(ch)) {
			push_token(&tokens, (Token){ TOKEN_VAR, alloc_char_as_str(toupper(ch)), line, i - column_start + 1 });
		} else {
			size_t column = i - column_start + 1;

			// feels like way too much heap allocation going on here but oh well
			char *error_message = alloc_empty_str();
			append_str(&error_message, "invalid token encountered on line ");
			append_str_and_free(&error_message, alloc_num_as_str(line));
			append_str(&error_message, " column ");
			append_str_and_free(&error_message, alloc_num_as_str(column));

			return (LexerResult){ tokens, error_message };
		}
	}

	return (LexerResult){ tokens, .error = NULL };
}

void free_lexer_result(LexerResult result) {
	free_token_list(result.tokens);

	if (result.error != NULL)
		free(result.error);
}
