#ifndef INCLUDE_AST_H
#define INCLUDE_AST_H

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
} Statement;

typedef struct {
	Statement *statements;
	size_t length;
} AST;

extern AST empty_ast();
extern void push_statement(AST *ast, Statement statement);
extern void free_expr(Expr expr);
extern void free_expr_list(ExprList *exprs);
extern void free_statement(Statement statement);
extern void free_ast(AST ast);

extern Expr parse_expr(Token *tokens, int min_bp);
extern AST parse(Token *tokens);

#endif // INCLUDE_AST_H
