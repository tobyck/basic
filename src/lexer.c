#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>

#include "lexer.h"
#include "utils.h"

void push_token(Token **head, TokenType type, char *literal, size_t line, size_t column) {
	// create new token which will be the head
	Token *token = malloc(sizeof(Token));
	token->type = type;
	token->literal = literal;
	token->line = line;
	token->column = column;
	token->next = *head; // make the new head point to the old one

	*head = token;
}

Token pop_token(Token **head) {
	Token *ret = *head;
	if (ret)
		*head = ret->next;
	return *ret;
}

void free_token_linked_list(Token *head) {
	Token *temp;
	while (head != NULL) {
		temp = head; // save the current node before we move forward
		head = head->next;

		// free node (and literal if applicable)
		if (temp->literal != NULL) free(temp->literal);
		free(temp);
	}
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

void free_lexer_error_list(LexerErrorList errors) {
	for (size_t i = 0; i < errors.length; i++)
		free(errors.errors[i].message);

	free(errors.errors);
}

Lexer *new_lexer(char *code) {
	Lexer *lexer = malloc(sizeof(Lexer));

	lexer->code = code;
	// these two nulls are the heads of the linkend lists of valid/invalid tokens
	lexer->result = (LexerResult){ NULL, NULL, empty_lexer_error_list() };
	lexer->current_index = 0;
	lexer->line = 1;
	lexer-> column_start = 0;

	return lexer;
}

inline char peek(Lexer *lexer) { return lexer->code[lexer->current_index]; }
inline char peek_nth(Lexer *lexer, size_t n) { return lexer->code[lexer->current_index + n]; }
inline char consume(Lexer *lexer) { return lexer->code[lexer->current_index++]; }

bool case_insensitive_match(Lexer *lexer, char *str) {
	for (size_t i = 0; i < strlen(str); i++)
		if (tolower(lexer->code[lexer->current_index + i]) != tolower(str[i]))
			return false;

	return true;
}

// this is called when no valid token has been matched by the lexer
void lexer_invalid_token(Lexer *lexer, size_t line, size_t column) {
	// result.invalid points to the head of a linked list, so we can dereference
	// that to get the first token
	Token previous_invalid = *lexer->result.invalid;
	
	if (
		lexer->result.invalid != NULL && // if there's already an invalid token and
		// the current invalid token is right next to the previous one
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
			// ending a statement, so only push a token if that's not the case
			if (lexer->result.valid != NULL && (*lexer->result.valid).type != TOKEN_END_STATEMENT)
				push_token(&lexer->result.valid, TOKEN_END_STATEMENT, NULL, l, c);

			continue;
		}

		// simple case where a single char is mapped to single token with no additional info
		#define simple_token_case(char, type) \
			case char: \
				push_token(&lexer->result.valid, type, NULL, l, c); \
				consume(lexer); \
				continue;

		switch (peek(lexer)) {
			case '+':
			case '*':
			case '/':
			case '%':
				push_token(&lexer->result.valid, TOKEN_BINARY_OP, alloc_char_as_str(consume(lexer)), l, c);
				continue;
			case '-': {
				char *literal = alloc_char_as_str(consume(lexer));
				switch ((*lexer->result.valid).type) {
					case TOKEN_CLOSE_PAREN:
					case TOKEN_INT:
					case TOKEN_NAME:
						push_token(&lexer->result.valid, TOKEN_BINARY_OP, literal, l, c);
						break;
					default:
						push_token(&lexer->result.valid, TOKEN_UNARY_OP, literal, l, c);
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
				push_token(&lexer->result.valid, type, NULL, l, c); \
				continue; \
			}

		keyword_token("let", TOKEN_LET)
		keyword_token("print", TOKEN_PRINT)

		// numbers

		if (isdigit(peek(lexer))) {
			char *literal = alloc_empty_str();

			while (isdigit(peek(lexer)))
				append_char(&literal, consume(lexer));

			if (peek(lexer) == '.') {
				push_lexer_error(&lexer->result.errors, (LexerError){
					strdup("Tiny BASIC does not support decimal numbers"),
					.line = l,
					.start_column = c,
					.error_column = lexer->current_index - lexer->column_start + 1
				});
				consume(lexer);
				free(literal);
			} else
				push_token(&lexer->result.valid, TOKEN_INT, literal, l, c);

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
			push_token(&lexer->result.valid, TOKEN_STRING, string, l, c);
			continue;
		}

		// names (vars/functions)

		if (isalpha(peek(lexer))) {
			char *name = alloc_char_as_str(consume(lexer));
			while (isalpha(peek(lexer)))
				append_char(&name, consume(lexer));
			push_token(&lexer->result.valid, TOKEN_NAME, name, l, c);
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
	free_token_linked_list(result.valid);
	free_token_linked_list(result.invalid);
	free_lexer_error_list(result.errors);
}
