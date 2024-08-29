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
		case TOKEN_NUMBER: return "NUMBER";
		case TOKEN_BINARY_OP: return "BINARY_OP";
		case TOKEN_UNARY_OP: return "UNARY_OP";
		case TOKEN_OPEN_PAREN: return "OPEN_PAREN";
		case TOKEN_CLOSE_PAREN: return "CLOSE_PAREN";
		case TOKEN_PRINT: return "PRINT";
		case TOKEN_STRING: return "STRING";
		case TOKEN_COMMA: return "COMMA";
		case TOKEN_SEMICOLON: return "SEMICOLON";
		case TOKEN_EOF: return "EOF";
		case TOKEN_INVALID: return "INVALID";
	}
}

void push_token(TokenLinkedList *list, TokenType type, char *literal, size_t line, size_t column) {
	Token *token = malloc(sizeof(Token));
	token->type = type;
	token->literal = literal;
	token->line = line;
	token->column = column;
	token->next = NULL;

	if (list->head == NULL) {
		// if list is empty then the head and tail will be the new token
		list->head = token;
		list->tail = token;
	} else {
		list->tail->next = token;
		list->tail = token;
	}
}

Token *pop_token(TokenLinkedList *list) {
	if (list->head == NULL) {
		printf("Error: cannot pop from empty linked list of tokens\n");
		exit(EXIT_FAILURE);
	}

	Token *ret = list->head; // save the head (which we'll return)
	list->head = ret->next; // update the head to be the node after it

	// set tail to null if list is now empty
	if (list->head == NULL)
		list->tail = NULL;

	return ret;
}

void free_token(Token *token) {
	if (token->literal != NULL) free(token->literal);
	free(token);
}

void free_token_linked_list(TokenLinkedList *list) {
	Token *temp;

	while (list->head != NULL) {
		temp = list->head; // save current node
		list->head = list->head->next; // move forward
		free_token(temp); // free where we just came from
	}

	list->head = NULL;
	list->tail = NULL;
}

LexerErrorList empty_lexer_error_list() {
	LexerError *errors = malloc(0);
	ensure_alloc(errors);
	return (LexerErrorList){ errors, 0 };
}

void push_lexer_error(LexerErrorList *list, LexerError error) {
	list->errors = realloc(list->errors, sizeof(LexerError) * (list->length + 1));
	ensure_alloc(list->errors);
	list->errors[list->length++] = error;
}

Lexer *new_lexer(char *code) {
	Lexer *lexer = malloc(sizeof(Lexer));

	lexer->code = code;
	lexer->result = (LexerResult){
		// these nulls are the heads and tails of linked lists of tokens
		{ NULL, NULL },
		{ NULL, NULL },
		empty_lexer_error_list()
	};
	lexer->current_index = 0;
	lexer->line = 1;
	lexer-> column_start = 0; // index in the code of the first char in current column

	return lexer;
}

inline char peek(Lexer *lexer) { return lexer->code[lexer->current_index]; }
inline char consume(Lexer *lexer) { return lexer->code[lexer->current_index++]; }

inline bool valid_variable_char(Lexer *lexer) {
	char ch = peek(lexer);
	return isalpha(ch) || ch == '_' || ch == '$';
}

bool case_insensitive_match(Lexer *lexer, char *str) {
	for (size_t i = 0; i < strlen(str); i++)
		if (tolower(lexer->code[lexer->current_index + i]) != tolower(str[i]))
			return false;

	return true;
}

// this is called when no valid token has been matched by the lexer
void lexer_invalid_token(Lexer *lexer, size_t line, size_t column) {
	Token previous_invalid = *lexer->result.invalid.tail;
	
	if (
		// if there's already an invalid token
		lexer->result.invalid.head != NULL &&
		// and the current invalid token is right next to the previous one
		previous_invalid.column + strlen(previous_invalid.literal) == column
	)
		// append the char to the previous invalid token
		append_char(&previous_invalid.literal, consume(lexer));

	// otherwise push a new invalid token
	else push_token(
		&lexer->result.invalid,
		TOKEN_INVALID, alloc_char_as_str(consume(lexer)), line, column
	);
}

LexerResult lex(char *code) {
	Lexer *lexer = new_lexer(code);

	while (peek(lexer) != '\0') {
		// consume whitespace
		while (peek(lexer) == ' ' || peek(lexer) == '\t') consume(lexer);

		// consume comments
		if (peek(lexer) == '\'' || case_insensitive_match(lexer, "rem"))
			while (peek(lexer) != '\n')
				consume(lexer);

		// line and column of token that's about to be determined
		size_t l = lexer->line;
		size_t c = lexer->current_index - lexer->column_start + 1;

		if (peek(lexer) == '\n') {
			consume(lexer);
			lexer->line++;
			lexer->column_start = lexer->current_index;
			continue;
		}

		#define single_char_token_case(char, type) \
			case char: \
				push_token(&lexer->result.valid, type, NULL, l, c); \
				consume(lexer); \
				continue;

		#define push_single_char_literal(type) \
			push_token(&lexer->result.valid, type, alloc_char_as_str(consume(lexer)), l, c); \
			continue;

		switch (peek(lexer)) {
			case '+':
			case '*':
			case '/':
			case '^': push_single_char_literal(TOKEN_BINARY_OP);
			case '-': {
				Token *previous_token = lexer->result.valid.tail;
				char *literal = alloc_char_as_str(consume(lexer));
				if (
					previous_token != NULL &&
					(previous_token->type == TOKEN_CLOSE_PAREN ||
					previous_token->type == TOKEN_NUMBER ||
					previous_token->type == TOKEN_NAME)
				) push_token(&lexer->result.valid, TOKEN_BINARY_OP, literal, l, c);
				else push_token(&lexer->result.valid, TOKEN_UNARY_OP, literal, l, c);
				continue;
			}
			single_char_token_case('=', TOKEN_ASSIGN)
			single_char_token_case('(', TOKEN_OPEN_PAREN)
			single_char_token_case(')', TOKEN_CLOSE_PAREN)
			case ',': push_single_char_literal(TOKEN_COMMA);
			case ';': push_single_char_literal(TOKEN_SEMICOLON);
		}

		// keywords

		#define keyword_token(keyword, type) \
			if (case_insensitive_match(lexer, keyword)) { \
				lexer->current_index += strlen(keyword); \
				push_token(&lexer->result.valid, type, NULL, l, c); \
				continue; \
			}

		keyword_token("let", TOKEN_LET)
		keyword_token("print", TOKEN_PRINT)

		// numbers

		// consume leading zeros
		while (peek(lexer) == '0') consume(lexer);

		if (isdigit(peek(lexer)) || peek(lexer) == '.') {
			BufferedString num_as_str = empty_buffered_string(4);
			bool has_decimal = false;

			for (size_t i = 0; isdigit(peek(lexer)) || peek(lexer) == '.'; i++) {
				char ch = consume(lexer);

				if (ch == '.') {
					if (has_decimal)
						push_lexer_error(&lexer->result.errors, (LexerError){
							"A number cannot have two decimal points",
							.line = l,
							.start_column = c,
							.error_column = c + i
						});
					else has_decimal = true;
				}

				buffered_string_append_char(&num_as_str, ch);
			}

			push_token(&lexer->result.valid, TOKEN_NUMBER, num_as_str.buffer, l, c);

			continue;
		}

		// strings

		if (peek(lexer) == '"') {
			consume(lexer); // consume opening quotes
			BufferedString string = empty_buffered_string(8);

			while (peek(lexer) != '"') {
				char ch = peek(lexer);
				// if string ends early without quotes
				if (ch == '\0' || ch == '\n') {
					push_lexer_error(&lexer->result.errors, (LexerError){
						"Expected closing double quotes to match the opening ones",
						.line = l,
						.start_column = c,
						.error_column = lexer->current_index - lexer->column_start + 1
					});
					free(string.buffer);
					goto continue_main_lexer_loop;
				}

				if (ch == '\\') {
					consume(lexer); // consume backslash
					char escaped_char = peek(lexer);
					switch (escaped_char) {
						case 'n': ch = '\n'; break;
						case 't': ch = '\t'; break;
						case '"': ch = '"'; break;
						case '\\': ch = '\\'; break;
						default: ch = escaped_char;
					}
				}

				buffered_string_append_char(&string, ch);
				consume(lexer);
			}

			consume(lexer); // consume closing quotes
			push_token(&lexer->result.valid, TOKEN_STRING, string.buffer, l, c);
			continue;
		}

		// names (vars/functions)

		if (valid_variable_char(lexer)) {
			BufferedString name = empty_buffered_string(4);
			while (valid_variable_char(lexer))
				buffered_string_append_char(&name, consume(lexer));
			push_token(&lexer->result.valid, TOKEN_NAME, name.buffer, l, c);
			continue;
		}

		lexer_invalid_token(lexer, l, c);

		continue_main_lexer_loop:;
	}

	LexerResult result = lexer->result;

	free(lexer);

	return result;
}
