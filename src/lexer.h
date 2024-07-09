#ifndef INCLUDE_LEXER_H
#define INCLUDE_LEXER_H

#include <stdbool.h>

typedef enum {
	TOKEN_LET,
	TOKEN_VAR,
	TOKEN_EQ,
	TOKEN_INT,
	TOKEN_ADD,
	TOKEN_SUB,
	TOKEN_MUL,
	TOKEN_DIV,
	TOKEN_MOD,
	TOKEN_OPEN_PAREN,
	TOKEN_CLOSE_PAREN,
	TOKEN_PRINT,
	TOKEN_STRING,
	TOKEN_COMMA,
	TOKEN_END_STATEMENT,
	TOKEN_EOF,
	TOKEN_INVALID
} TokenType;

typedef struct {
	TokenType type;
	char *literal;
	size_t line;
	size_t column;
} Token;

typedef struct {
	Token *tokens;
	size_t length;
} TokenList;

typedef struct {
	char *message;
	size_t line;
	size_t start_column; // where erroneous token begins
	size_t error_column; // column where which causes error
} LexerError;

typedef struct {
	LexerError *errors;
	size_t length;
} LexerErrorList;

typedef struct {
	TokenList valid;
	TokenList invalid;
	LexerErrorList errors;
} LexerResult;

typedef struct {
	char *code;
	LexerResult result;
	size_t current_index;
	size_t line;
	size_t column_start;
} Lexer;

// tokens
extern char *stringify_token_type(TokenType token_type);
extern char *stringify_token(Token token);

// token lists
extern TokenList empty_token_list();
extern void push_token(TokenList *list, Token token);
extern inline Token last_token(TokenList tokens);
extern void print_token_list(TokenList list);
extern void free_token_list(TokenList tokens);

// lexer errors
extern LexerErrorList empty_lexer_error_list();
extern void push_lexer_error(LexerErrorList *list, LexerError error);
extern void print_lexer_errors(LexerErrorList errors);
extern void free_lexer_error_list(LexerErrorList errors);

// lexer
extern Lexer *new_lexer(char *code);
extern inline char peek(Lexer *lexer);
extern inline char peek_nth(Lexer *lexer, size_t n);
extern inline char consume(Lexer *lexer);
extern bool case_insensitive_match(Lexer *lexer, char *str);
extern void lexer_invalid_token(Lexer *lexer, size_t line, size_t column);

extern LexerResult lex(char *code);
extern void free_lexer_keep_result(Lexer *lexer);
extern void free_lexer_result(LexerResult result);

#endif // INCLUDE_LEXER_H
