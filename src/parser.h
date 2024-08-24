#ifndef INCLUDE_AST_H
#define INCLUDE_AST_H

#include <stdint.h>

#include "lexer.h"

struct ExprList;

typedef struct {
	enum {
		EXPR_INT,
		EXPR_VAR,
		EXPR_FUNC
	} type;
	union {
		char *int_literal;
		char variable;
		struct {
			char *name;
			struct ExprList *args;
		} func;
	} expr;
} Expr;

typedef struct ExprList {
	Expr *exprs;
	size_t length;
} ExprList;

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

typedef struct {
	char *message;
	Token *token;
} ParseError;

typedef struct {
	ParseError *errors;
	size_t length;
} ParseErrorList;

typedef struct {
	AST ast;
	ParseErrorList errors;
} ParserResult;

typedef struct {
	uint8_t arity;
	uint8_t left_binding_power;
	uint8_t right_binding_power;
} FuncInfo;

typedef struct {
	bool success;
	union {
		Expr expr;
		ParseError error;
	} result;
} ParseExprResult;

extern ExprList *empty_expr_list();
extern void push_expr(ExprList *exprs, Expr expr);

extern AST empty_ast();
extern void push_statement(AST *ast, Statement statement);
extern void free_expr(Expr expr);
extern void free_expr_list(ExprList *exprs);
extern void free_statement(Statement statement);
extern void free_ast(AST ast);

extern ParseErrorList empty_parse_error_list();
extern void push_parse_error(ParseErrorList *errors, ParseError error);
extern void free_parse_error_list(ParseErrorList errors);

extern bool token_ends_expr(Token *token);
extern FuncInfo get_func_info(char *func_name);
extern ParseExprResult parse_expr(
	TokenLinkedList *tokens,
	uint8_t min_binding_power
);
extern ParserResult parse(TokenLinkedList tokens);

#endif // INCLUDE_AST_H
