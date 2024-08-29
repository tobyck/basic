#ifndef INCLUDE_AST_H
#define INCLUDE_AST_H

#include <stdint.h>

#include "utils.h"
#include "lexer.h"

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
			char *name;
			struct ExprList *args;
		} call;
	} expr;
} Expr;

extern void free_expr(Expr expr);

typedef struct ExprList {
	Expr *exprs;
	size_t length;
	bool stored_delimiters;
	BufferedString delimiters;
} ExprList;

extern ExprList *empty_expr_list(bool store_delimiters);
extern ExprList *new_expr_list_from(size_t count, ...);
extern void push_expr(ExprList *exprs, Expr expr);
extern void free_expr_list(ExprList *exprs);

typedef struct {
	enum {
		STATEMENT_ASSIGNMENT,
		STATEMENT_PRINT
	} type;
	union {
		struct {
			char variable;
			Expr expr;
		} assignment;
		ExprList *print;
	} statement;
	size_t id;
} Statement;

typedef struct {
	Statement *statements;
	size_t length;
} AST;

extern AST empty_ast();
extern void push_statement(AST *ast, Statement statement);
extern void free_statement(Statement statement);
extern void free_ast(AST ast);

typedef struct {
	char *message;
	Token *token;
} ParseError;

typedef struct {
	ParseError *errors;
	size_t length;
} ParseErrorList;

extern ParseErrorList empty_parse_error_list();
extern void push_parse_error(ParseErrorList *errors, ParseError error);
extern void free_parse_error_list(ParseErrorList errors);

typedef struct {
	uint8_t left;
	uint8_t right;
} BindingPower;

typedef struct {
	bool success;
	union {
		Expr expr;
		ParseError error;
	} result;
} ParseExprResult;

typedef struct {
	bool success;
	union {
		ExprList *exprs;
		ParseError error;
	} result;
} ParseExprListResult;

typedef struct {
	AST ast;
	ParseErrorList errors;
} ParserResult;

// expr parsing helpers
extern bool token_ends_expr(Token *token);
extern bool token_ends_expr_list(Token *token);
extern BindingPower get_op_binding_power(Token token);
extern inline ParseExprResult parse_expr_error(char *string, Token *token);
extern inline ParseExprResult parse_expr_success(Expr expr);

// main functions for expr parsing
extern ParseExprResult parse_expr(TokenLinkedList *tokens, bool allow_string);
extern ParseExprResult parse_math_expr(
	TokenLinkedList *tokens,
	uint8_t min_binding_power
);
extern ParseExprListResult parse_expr_list(
	TokenLinkedList *tokens,
	bool allow_string,
	bool store_delimiters
);

extern ParserResult parse(TokenLinkedList tokens);

#endif // INCLUDE_AST_H
