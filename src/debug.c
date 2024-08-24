#include <stdio.h>

#include "debug.h"
#include "lexer.h"

void print_token(Token *token) {
	printf("%s ", stringify_token_type(token->type));
	if (token->literal != NULL)
		printf("\"%s\" ", token->literal);
	printf("at %zu:%zu", token->line, token->column);
}

void print_token_linked_list(TokenLinkedList tokens) {
	if (tokens.head == NULL) {
		printf("[]\n");
		return;
	}

	printf("[\n");

	Token *current = tokens.head;
	while (current != NULL) {
		printf("  ");
		print_token(current);
		if (current->next != NULL)
			printf(",\n");
		current = current->next;
	}

	printf("\n]\n");
}

void print_lexer_errors(LexerErrorList errors) {
	if (errors.length == 0) {
		printf("[]\n");
		return;
	}

	printf("[\n");

	for (size_t i = 0; i < errors.length; i++) {
		LexerError err = errors.errors[i];
		printf(
			"  %s (erroneous token starts at: %zu:%zu, error at %zu:%zu)",
			err.message,
			err.line,
			err.start_column,
			err.line,
			err.error_column
		);
		if (i < errors.length - 1) printf(",\n");
	}
	
	printf("\n]\n");
}

void print_expr(Expr expr) {
	switch (expr.type) {
		case EXPR_INT: printf("%s", expr.expr.int_literal); break;
		case EXPR_VAR: printf("%c", expr.expr.variable); break;
		case EXPR_FUNC:
			printf("%s", expr.expr.func.name);
			print_expr_list(expr.expr.func.args);
	}
}

void print_expr_list(ExprList *exprs) {
	if (exprs->length == 0) {
		printf("[]");
		return;
	}

	printf("[");

	for (size_t i = 0; i < exprs->length; i++) {
		print_expr(exprs->exprs[i]);
		if (i < exprs->length - 1) printf(", ");
	}

	printf("]");
}

void print_ast(AST ast) {
	if (ast.length == 0) {
		printf("[]\n");
		return;
	}

	printf("[\n");

	for (size_t i = 0; i < ast.length; i++) {
		printf("  ");
		Statement s = ast.statements[i];
		switch (s.type) {
			case STATEMENT_ASSIGNMENT:
				printf("ASSIGNMENT %c = ", s.statement.assignment.variable);
				print_expr(s.statement.assignment.expr);
				break;
			case STATEMENT_PRINT:
				printf("PRINT ");
				print_expr_list(s.statement.print);
		}
	}

	printf("\n]\n");
}

void print_parse_errors(ParseErrorList errors) {
	if (errors.length == 0) {
		printf("[]\n");
		return;
	}

	printf("[\n");

	for (size_t i = 0; i < errors.length; i++) {
		printf("  '%s' at (", errors.errors[i].message);
		print_token(errors.errors[i].token);
		printf(")");
		if (i < errors.length - 1) printf(",\n");
	}

	printf("\n]\n");
}
