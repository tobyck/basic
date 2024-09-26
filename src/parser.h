#ifndef INCLUDE_PARSER_H
#define INCLUDE_PARSER_H

#include <stdint.h>

#include "lexer.h"
#include "utils.h"

struct ExprList;

typedef struct {
	enum {
		EXPR_NUMBER,
		EXPR_STRING,
		EXPR_VAR,
		EXPR_CALL
	} type;
	union {
		char *number_literal;
		char *string_literal;
		char *variable;
		struct {
			char *name_string;
			char name_char;
			struct ExprList *args;
		} call;
	} expr;
} Expr;

extern void free_expr(Expr expr);

typedef struct ExprList {
	Expr *exprs;
	size_t length;
	bool store_delimiters;
	BufferedString delimiters;
} ExprList;

extern ExprList *new_expr_list_from(size_t length, ...);
extern ExprList *empty_expr_list(bool store_delimiters);
extern void push_expr(ExprList *exprs, Expr expr);
extern void free_expr_list(ExprList *exprs);

typedef struct {
	enum {
		STATEMENT_ASSIGNMENT,
		STATEMENT_PRINT
	} type;
	union {
		struct {
			char *variable;
			Expr expr;
		} assignment;
		ExprList *print;
	} statement;
} Statement;

typedef struct {
	Statement *statements;
	size_t length;
} AST;

extern AST new_ast(void);

typedef struct {
	uint8_t left;
	uint8_t right;
} BindingPower;

typedef struct {
	bool success;
	union {
		Expr expr;
		Error error;
	} result;
} ParseExprResult;

typedef struct {
	bool success;
	union {
		ExprList *exprs;
		Error error;
	} result;
} ParseExprListResult;

typedef struct {
	Error *errors;
	size_t length;
} ErrorList;

typedef struct {
	bool success;
	union {
		AST ast;
		ErrorList errors;
	} result;
} ParserResult;

extern ParseExprResult expected_expression_error(Lexer *lexer);
extern ParseExprResult parse_expr(Lexer *lexer, bool allow_string);

extern BindingPower get_binding_power(Token token);
extern bool token_ends_expr(TokenResult token_result);
extern bool token_ends_expr_list(TokenResult token_result);
extern ParseExprResult parse_math_expr(Lexer *lexer, uint8_t min_binding_power);

extern ParseExprListResult parse_expr_list(
	Lexer *lexer,
	bool allow_string,
	bool store_delimiters
);

extern ParserResult parse(char *code);

#endif // INCLUDE_PARSER_H
