#ifndef INCLUDE_LEXER_H
#define INCLUDE_LEXER_H

#include <stdbool.h>

typedef enum {
	TOKEN_LET,
	TOKEN_NAME,
	TOKEN_ASSIGN,
	TOKEN_INT,
	TOKEN_NEGATE,
	TOKEN_BINARY_OP,
	TOKEN_UNARY_OP,
	TOKEN_OPEN_PAREN,
	TOKEN_CLOSE_PAREN,
	TOKEN_PRINT,
	TOKEN_STRING,
	TOKEN_COMMA,
	TOKEN_END_STATEMENT,
	TOKEN_EOF,
	TOKEN_INVALID
} TokenType;

typedef struct Token {
	TokenType type;
	char *literal;
	size_t line;
	size_t column;
	struct Token *next;
} Token;

typedef struct {
	Token *head;
	Token *tail;
} TokenLinkedList;

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
	TokenLinkedList valid;
	TokenLinkedList invalid;
	LexerErrorList errors;
} LexerResult;

typedef struct {
	char *code;
	LexerResult result;
	size_t current_index;
	size_t line;
	size_t column_start;
} Lexer;

extern char *stringify_token_type(TokenType token_type);

// token linked list
extern void push_token(
	TokenLinkedList *list,
	TokenType type,
	char *literal,
	size_t line,
	size_t column
);
extern Token *pop_token(TokenLinkedList *list);
extern void free_token(Token *token);
extern void free_token_linked_list(TokenLinkedList list);

// lexer errors
extern LexerErrorList empty_lexer_error_list();
extern void push_lexer_error(LexerErrorList *list, LexerError error);

// lexer
extern Lexer *new_lexer(char *code);
extern inline char peek(Lexer *lexer);
extern inline char peek_nth(Lexer *lexer, size_t n);
extern inline char consume(Lexer *lexer);
extern bool case_insensitive_match(Lexer *lexer, char *str);
extern void lexer_invalid_token(Lexer *lexer, size_t line, size_t column);

extern LexerResult lex(char *code);

#endif // INCLUDE_LEXER_H
