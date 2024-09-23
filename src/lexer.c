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
	}
}

void free_token(Token token) {
	if (token.literal_type == LITERAL_STRING)
		free(token.string_literal);
}

Lexer *new_lexer(char *code, size_t buffer_capacity) {
	Lexer *lexer = malloc(sizeof(Lexer));
	ensure_alloc(lexer);

	lexer->code = code;
	lexer->current_index = 0;
	lexer->line = 1;
	lexer->column_start = 0;

	Token *tokens = malloc(buffer_capacity * sizeof(Token));
	ensure_alloc(tokens);

	lexer->tokens = (TokenBuffer){
		tokens,
		buffer_capacity,
		.length = 0,
		.next_index = 0,
		.peeked_count = 0
	};

	return lexer;
}

void free_lexer(Lexer *lexer) {
	for (size_t i = 0; i < lexer->tokens.length; i++)
		if (lexer->tokens.tokens[i].literal_type == LITERAL_STRING)
			free(lexer->tokens.tokens[i].string_literal);

	free(lexer->tokens.tokens);
	free(lexer);
}

Token *get_previous_token(TokenBuffer buffer) {
	if (buffer.length == 0) return NULL;
	// this is so ugly :cry:
	else return &buffer.tokens[(buffer.next_index == 0 ? buffer.length : buffer.next_index) - 1];
}

inline char peek(Lexer *lexer) { return lexer->code[lexer->current_index]; }
inline char consume(Lexer *lexer) { return lexer->code[lexer->current_index++]; }

bool case_insensitive_match(Lexer *lexer, char *str) {
	for (size_t i = 0; i < strlen(str); i++)
		if (tolower(lexer->code[lexer->current_index + i]) != tolower(str[i]))
			return false;

	return true;
}

inline bool valid_variable_char(Lexer *lexer) {
	char ch = peek(lexer);
	return isalpha(ch) || ch == '_' || ch == '$';
}

TokenResult _get_next_token(Lexer *lexer) {
	// consume whitespace
	while (peek(lexer) == ' ' || peek(lexer) == '\t') consume(lexer);

	// consume comments
	if (peek(lexer) == '\'' || case_insensitive_match(lexer, "rem"))
		while (peek(lexer) != '\n')
			consume(lexer);

	if (peek(lexer) == '\n') {
		consume(lexer);
		lexer->line++;
		lexer->column_start = lexer->current_index;
	}

	// line and column of token that's about to be determined
	size_t l = lexer->line;
	size_t c = lexer->current_index - lexer->column_start + 1;

	// single char tokens

	#define single_char_token(token_type) (TokenResult){ \
		true, { .token = { token_type, LITERAL_CHAR, NULL, consume(lexer), .line = l, .column = c } } \
	}

	switch (peek(lexer)) {
		case '\0': return (TokenResult){ true, { .token = { TOKEN_EOF, LITERAL_NONE, .line = l, .column = c } } };
		case '+':
		case '*':
		case '/': return single_char_token(TOKEN_BINARY_OP);
		case '-': {
			Token *previous_token = get_previous_token(lexer->tokens);
			if (
				previous_token != NULL &&
				(previous_token->type == TOKEN_CLOSE_PAREN ||
				previous_token->type == TOKEN_NUMBER ||
				previous_token->type == TOKEN_NAME)
			) return single_char_token(TOKEN_BINARY_OP);
			else return single_char_token(TOKEN_UNARY_OP);
		}
		case '=': return single_char_token(TOKEN_ASSIGN);
		case '(': return single_char_token(TOKEN_OPEN_PAREN);
		case ')': return single_char_token(TOKEN_CLOSE_PAREN);
		case ',': return single_char_token(TOKEN_COMMA);
		case ';': return single_char_token(TOKEN_SEMICOLON);
	}

	// keywords

	#define match_keyword_token(keyword, token_type) \
		if (case_insensitive_match(lexer, keyword)) { \
			lexer->current_index += strlen(keyword); \
			return (TokenResult){ true, { .token = { token_type, LITERAL_NONE, .line = l, .column = c } } }; \
		}

	match_keyword_token("let", TOKEN_LET)
	match_keyword_token("print", TOKEN_PRINT)

	// numbers

	while (peek(lexer) == '0') consume(lexer); // consume leading zeros

	if (isdigit(peek(lexer)) || peek(lexer) == '.') {
		BufferedString num_as_str = empty_buffered_string(4);
		bool has_decimal = false;

		for (size_t i = 0; isdigit(peek(lexer)) || peek(lexer) == '.'; i++) {
			char ch = consume(lexer);

			if (ch == '.') {
				if (has_decimal)
					return (TokenResult){
						false, { .error = { "A number cannot have two decimal points", l, c, c + i } }
					};
				else
					has_decimal = true;
			}

			buffered_string_append_char(&num_as_str, ch);
		}

		return (TokenResult){ true, { .token = { TOKEN_NUMBER, LITERAL_STRING, num_as_str.buffer, '\0', l, c } } };
	}

	// strings

	if (peek(lexer) == '"') {
		consume(lexer); // consume opening quotes
		BufferedString string = empty_buffered_string(8);

		while (peek(lexer) != '"') {
			char ch = peek(lexer);

			// if string ends early without quotes
			if (ch == '\0' || ch == '\n') {
				free(string.buffer);
				return (TokenResult){ false, { .error = {
					"Expected closing double quotes to match the opening ones",
					l, c, lexer->current_index - lexer->column_start + 1
				} } };
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
		return (TokenResult){ true, { .token = { TOKEN_STRING, LITERAL_STRING, string.buffer, '\0', l, c } } };
	}

	// names (vars/functions)

	if (valid_variable_char(lexer)) {
		BufferedString name = empty_buffered_string(4);
		while (valid_variable_char(lexer))
			buffered_string_append_char(&name, consume(lexer));
		return (TokenResult){ true, { .token = { TOKEN_NAME, LITERAL_STRING, name.buffer, '\0', l, c } } };
	}

	return (TokenResult){ false, { .error = { "Invalid token", l, c, c } } };
}

void _write_token_result(Lexer *lexer, TokenResult token_result, size_t index) {
	TokenBuffer *buffer = &lexer->tokens;

	if (token_result.success) {
		Token token = token_result.result.token;

		if (buffer->length < buffer->capacity)
			buffer->length++;
		else
			free_token(buffer->tokens[index]);

		buffer->tokens[index].type = token.type;
		buffer->tokens[index].literal_type = token.literal_type;
		buffer->tokens[index].string_literal = token.string_literal;
		buffer->tokens[index].char_literal = token.char_literal;
		buffer->tokens[index].line = token.line;
		buffer->tokens[index].column = token.column;
	}
}

TokenResult peek_token(Lexer *lexer) {
	TokenResult token_result = _get_next_token(lexer);
	_write_token_result(lexer, token_result, lexer->tokens.next_index + (lexer->tokens.peeked_count++));
	return token_result;
}

TokenResult next_token(Lexer *lexer) {
	TokenResult token_result;

	if (lexer->tokens.peeked_count == 0) {
		token_result = _get_next_token(lexer);
		_write_token_result(lexer, token_result, lexer->tokens.next_index);
	} else {
		token_result = (TokenResult){ true, {
			.token = lexer->tokens.tokens[lexer->tokens.next_index + lexer->tokens.peeked_count]
		} };
		lexer->tokens.peeked_count--;
	}

	lexer->tokens.next_index += 1;
	lexer->tokens.next_index %= lexer->tokens.capacity;

	return token_result;
}
