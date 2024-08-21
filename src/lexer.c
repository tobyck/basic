#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>

#include "lexer.h"
#include "utils.h"

char *stringify_token_type(TokenType token_type) {
	switch (token_type) {
		case TOKEN_LET: return "LET";
		case TOKEN_NAME: return "NAME";
		case TOKEN_ASSIGN: return "ASSIGN";
		case TOKEN_INT: return "INT";
		case TOKEN_NEGATE: return "NEGATE";
		case TOKEN_BINARY_OP: return "BINARY_OP";
		case TOKEN_UNARY_OP: return "UNARY_OP";
		case TOKEN_OPEN_PAREN: return "OPEN_PAREN";
		case TOKEN_CLOSE_PAREN: return "CLOSE_PAREN";
		case TOKEN_PRINT: return "PRINT";
		case TOKEN_STRING: return "STRING";
		case TOKEN_COMMA: return "COMMA";
		case TOKEN_END_STATEMENT: return "END_STATEMENT";
		case TOKEN_EOF: return "EOF";
		case TOKEN_INVALID: return "INVALID";
	}
}

char *stringify_token(Token token) {
	char *string = alloc_empty_str();

	append_str(&string, stringify_token_type(token.type));
	
	if (token.literal != NULL) {
		append_str(&string, " \"");
		append_str(&string, token.literal);
		append_str(&string, "\"");
	}

	append_str(&string, " at ");
	append_str_and_free(&string, alloc_num_as_str(token.line));
	append_char(&string, ':');
	append_str_and_free(&string, alloc_num_as_str(token.column));

	return string;
}

TokenList empty_token_list() {
	Token *tokens = malloc(0);

	if (tokens == NULL) {
		printf("Error: could not allocate memory for token list\n");
		exit(EXIT_FAILURE);
	}

	return (TokenList){ tokens, 0 };
}

void push_token(TokenList *list, Token token) {
	list->tokens = realloc(list->tokens, sizeof(Token) * (list->length + 1));

	if (list->tokens == NULL) {
		printf("Error: could not reallocate memory for a new token\n");
		exit(EXIT_FAILURE);
	}

	list->tokens[list->length++] = token;
}

inline Token last_token(TokenList tokens) {
	return tokens.tokens[tokens.length - 1];
}

void print_token_list(TokenList list) {
	if (list.length == 0) {
		printf("[]\n");
		return;
	}

	printf("[");

	for (size_t i = 0; i < list.length; i++) {
		char *stringified_token = stringify_token(list.tokens[i]);
		printf("\n  %s", stringified_token);
		free(stringified_token);
		if (i < list.length - 1) printf(",");
		else printf("\n]\n");
	}
}

void free_token_list(TokenList tokens) {
	for (size_t i = 0; i < tokens.length; i++)
		if (tokens.tokens[i].literal != NULL)
			free(tokens.tokens[i].literal);

	free(tokens.tokens);
}

LexerErrorList empty_lexer_error_list() {
	LexerError *errors = malloc(0);

	if (errors == NULL) {
		printf("Error: could not allocate memory for lexer error list\n");
		exit(EXIT_FAILURE);
	}

	return (LexerErrorList){ errors, 0 };
}

void push_lexer_error(LexerErrorList *list, LexerError error) {
	list->errors = realloc(list->errors, sizeof(LexerError) * (list->length + 1));

	if (list->errors == NULL) {
		printf("Error: could not reallocate memory for new lexer error\n");
		return;
	}

	list->errors[list->length++] = error;
}

void print_lexer_errors(LexerErrorList errors) {
	if (errors.length == 0) {
		printf("[]\n");
		return;
	}

	printf("[\n");

	for (size_t i = 0; i < errors.length; i++) {
		LexerError err = errors.errors[i];
		printf(
			"  %s (erroneous token starts at: %zu:%zu, error at %zu:%zu)",
			err.message,
			err.line,
			err.start_column,
			err.line,
			err.error_column
		);
		if (i < errors.length - 1) printf(",\n");
		else printf("\n]\n");
	}
}

void free_lexer_error_list(LexerErrorList errors) {
	for (size_t i = 0; i < errors.length; i++)
		free(errors.errors[i].message);

	free(errors.errors);
}

Lexer *new_lexer(char *code) {
	Lexer *lexer = malloc(sizeof(Lexer));

	lexer->code = code;

	lexer->result = (LexerResult){
		empty_token_list(),
		empty_token_list(),
		empty_lexer_error_list()
	};

	lexer->current_index = 0;
	lexer->line = 1;
	lexer-> column_start = 0;

	return lexer;
}

inline char peek(Lexer *lexer) { return lexer->code[lexer->current_index]; }
inline char peek_nth(Lexer *lexer, size_t n) { return lexer->code[lexer->current_index + n]; }
inline char consume(Lexer *lexer) { return lexer->code[lexer->current_index++]; }

void lexer_invalid_token(Lexer *lexer, size_t line, size_t column) {
	Token previous_invalid = last_token(lexer->result.invalid);
	
	if (
		lexer->result.invalid.length > 0 && // if there's already an invalid token
		// and its column + length = the current column, append to previous token
		// i.e. this invalid char is directly next to the previous invalid token
		previous_invalid.column + strlen(previous_invalid.literal) == column
	)
		// append the char to the previous invalid token
		append_char(&lexer->result.invalid.tokens[lexer->result.invalid.length - 1].literal, consume(lexer));

	// otherwise push a new invalid token
	else push_token(&lexer->result.invalid, (Token){
		TOKEN_INVALID, alloc_char_as_str(consume(lexer)), line, column
	});
}

bool case_insensitive_match(Lexer *lexer, char *str) {
	for (size_t i = 0; i < strlen(str); i++)
		if (tolower(lexer->code[lexer->current_index + i]) != tolower(str[i]))
			return false;

	return true;
}

LexerResult lex(char *code) {
	Lexer *lexer = new_lexer(code);

	while (peek(lexer) != '\0') {
		// consume whitespace
		while (peek(lexer) == ' ' || peek(lexer) == '\t') consume(lexer);

		// consume comments
		if (case_insensitive_match(lexer, "rem"))
			while (peek(lexer) != '\n')
				consume(lexer);

		// line and column of token that's about to be determined
		size_t l = lexer->line;
		size_t c = lexer->current_index - lexer->column_start + 1;

		if (peek(lexer) == '\n') {
			consume(lexer);
			lexer->line++;
			lexer->column_start = lexer->current_index;

			// if there are no tokens or the previous token was an END_STATEMENT then we're not actually
			// ending a statement, so only push token if that's not the case
			if (lexer->result.valid.length > 0 && last_token(lexer->result.valid).type != TOKEN_END_STATEMENT)
				push_token(&lexer->result.valid, (Token){ TOKEN_END_STATEMENT, NULL, l, c });

			continue;
		}
		
		// simple single char tokens

		// simple case where a single char is mapped to single token with no additional info
		#define simple_token_case(char, type) \
			case char: \
				push_token(&lexer->result.valid, (Token){ type, NULL, l, c }); \
				consume(lexer); \
				continue;

		switch (peek(lexer)) {
			case '+':
			case '*':
			case '/':
			case '%':
				push_token(&lexer->result.valid, (Token){
					TOKEN_BINARY_OP, alloc_char_as_str(consume(lexer)), l, c
				});
				continue;
			case '-': {
				char *literal = alloc_char_as_str(consume(lexer));
				switch (last_token(lexer->result.valid).type) {
					case TOKEN_CLOSE_PAREN:
					case TOKEN_INT:
					case TOKEN_NAME:
						push_token(&lexer->result.valid, (Token){
							TOKEN_BINARY_OP, literal, l, c
						});
						break;
					default:
						push_token(&lexer->result.valid, (Token){ TOKEN_UNARY_OP, literal, l, c });
				}
				continue;
			}
			simple_token_case('=', TOKEN_ASSIGN)
			simple_token_case('(', TOKEN_OPEN_PAREN)
			simple_token_case(')', TOKEN_CLOSE_PAREN)
			simple_token_case(',', TOKEN_COMMA)
		}

		// keywords

		#define keyword_token(keyword, type) \
			if (case_insensitive_match(lexer, keyword)) { \
				lexer->current_index += strlen(keyword); \
				push_token(&lexer->result.valid, (Token){ type, NULL, l, c }); \
				continue; \
			}

		keyword_token("let", TOKEN_LET)
		keyword_token("print", TOKEN_PRINT)

		// numbers

		if (isdigit(peek(lexer))) {
			char *literal = alloc_empty_str();

			while (isdigit(peek(lexer)))
				append_char(&literal, consume(lexer));

			if (peek(lexer) == '.' && peek(lexer)) {
				push_lexer_error(&lexer->result.errors, (LexerError){
					strdup("Tiny BASIC does not support decimal numbers"),
					.line = l,
					.start_column = c,
					.error_column = lexer->current_index - lexer->column_start + 1
				});
				consume(lexer);
				free(literal);
			} else
				push_token(&lexer->result.valid, (Token){ TOKEN_INT, literal, l, c });

			continue;
		}

		// strings

		if (peek(lexer) == '"') {
			consume(lexer); // consume opening quotes
			char *string = alloc_empty_str();

			while (peek(lexer) != '"') {
				// if string ends early without quotes
				if (peek(lexer) == '\0' || peek(lexer) == '\n') {
					push_lexer_error(&lexer->result.errors, (LexerError){
						strdup("Expected closing double quotes to match the opening ones"),
						.line = l,
						.start_column = c,
						.error_column = lexer->current_index - lexer->column_start + 1
					});
					free(string);
					goto continue_main_lexer_loop;
				}

				append_char(&string, consume(lexer));
			}

			consume(lexer); // consume closing quotes
			push_token(&lexer->result.valid, (Token){ TOKEN_STRING, string, l, c });
			continue;
		}

		// names (vars/functions)

		if (isalpha(peek(lexer))) {
			char *name = alloc_char_as_str(consume(lexer));
			while (isalpha(peek(lexer)))
				append_char(&name, consume(lexer));
			push_token(&lexer->result.valid, (Token){
				TOKEN_NAME, name, l, c
			});
			continue;
		}

		lexer_invalid_token(lexer, l, c);

		continue_main_lexer_loop:;
	}

	LexerResult result = lexer->result;

	free(lexer);

	return result;
}

void free_lexer_result(LexerResult result) {
	free_token_list(result.valid);
	free_token_list(result.invalid);
	free_lexer_error_list(result.errors);
}
