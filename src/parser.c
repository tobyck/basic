#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include "parser.h"
#include "debug.h"

AST empty_ast() {
	Statement *statements = malloc(0);

	if (statements == NULL) {
		printf("Error: could not allocate memory for new AST\n");
		exit(EXIT_FAILURE);
	}

	return (AST){ statements, 0 };
}

void push_statement(AST *ast, Statement statement) {
	ast->statements = realloc(ast->statements, sizeof(Statement) * (ast->length + 1));

	if (ast->statements == NULL) {
		printf("Error: Could not allocate more memory for another statement in the AST\n");
		exit(EXIT_FAILURE);
	}

	ast->statements[ast->length++] = statement;
}

void free_expr(Expr expr) {
	switch (expr.type) {
		case EXPR_INT: free(expr.expr.int_literal); break;
		case EXPR_FUNC:
			free(expr.expr.func.name);
			free_expr_list(expr.expr.func.args);
			break;
		default: break;
	}
}

void free_expr_list(ExprList *exprs) {
	for (size_t i = 0; i < exprs->length; i++)
		free_expr(exprs->exprs[i]);

	free(exprs->exprs);
	free(exprs);
}

void free_statement(Statement statement) {
	switch (statement.type) {
		case STATEMENT_ASSIGNMENT: free_expr(statement.statement.assignment.expr); break;
		case STATEMENT_PRINT: free_expr_list(statement.statement.print); break;
	}
}

void free_ast(AST ast) {
	for (size_t i = 0; i < ast.length; i++)
		free_statement(ast.statements[i]);

	free(ast.statements);
}

Expr parse_expr(Token *tokens, int min_bp) {
	return (Expr){ EXPR_INT, { .int_literal = strdup("123") } };
}

AST parse(Token *tokens) {
	AST ast = empty_ast();

	// just for testing
	Expr e = parse_expr(tokens, 0);
	push_statement(&ast, (Statement){ STATEMENT_ASSIGNMENT, { .assignment = { 'a', e } } });

	return ast;
}
