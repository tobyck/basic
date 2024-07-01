#ifndef INCLUDE_LEXER_H
#define INCLUDE_LEXER_H

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
} TokenType;

typedef struct {
	TokenType type;
	char *content;
	size_t line;
	size_t column;
} Token;

typedef struct {
	Token *tokens;
	size_t length;
} TokenList;

// the rust is really getting to me
typedef struct {
	TokenList tokens;
	char *error;
} LexerResult;

// operators which we know the meaning of as soon as the lexer sees them (as
// opposed to something like < which could be followed by something else)
#define SIMPLE_OPS "=+-*/%"

extern TokenList empty_token_list();
extern void free_token(Token token);
extern void free_token_list(TokenList tokens);
extern void push_token(TokenList *list, Token token);

extern char *stringify_token_type(TokenType token_type);
extern char *stringify_token(Token token);
extern void print_token_list(TokenList list);

extern LexerResult lex(char *code, size_t code_length);
extern void free_lexer_result(LexerResult result);

#endif // INCLUDE_LEXER_H
