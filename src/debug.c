#include <stdio.h>

#include "debug.h"
#include "parser.h"

void print_token(Token token) {
	printf("%s ", stringify_token_type(token.type));
	if (token.string_literal != NULL)
		printf("\"%s\" ", token.string_literal);
	else if (token.char_literal != '\0')
		printf("'%c' ", token.char_literal);
	printf("at %zu:%zu", token.line, token.column);
}

void print_token_buffer_range(TokenBuffer buffer, size_t start, size_t end) {
	for (size_t i = start; i < end; i++) {
		printf("  ");
		print_token(buffer.tokens[i]);
		if (i < end - 1) printf(",");
		printf("\n");
	}
}

void print_token_buffer(TokenBuffer buffer) {
	if (buffer.length == 0) {
		printf("[]\n");
		return;
	}

	printf("[\n  cap = %zu,\n  len = %zu,\n  next = %zu,\n  peeked = %d\n  ----------\n", buffer.capacity, buffer.length, buffer.next_index, buffer.peeked);

	if (buffer.next_index == 0) print_token_buffer_range(buffer, 0, buffer.length);
	else {
		print_token_buffer_range(buffer, buffer.next_index, buffer.length);
		print_token_buffer_range(buffer, 0, buffer.next_index);
	}

	printf("]\n");
}

void print_expr(Expr expr) {
	switch (expr.type) {
		case EXPR_NUMBER: printf("%s", expr.expr.number_literal); break;
		case EXPR_STRING: printf("\"%s\"", expr.expr.string_literal); break;
		case EXPR_VAR: printf("%s", expr.expr.variable); break;
		case EXPR_CALL:
			if (expr.expr.call.name_string != NULL)
				printf("(%s", expr.expr.call.name_string);
			else
				printf("(%c", expr.expr.call.name_char);
			for (size_t i = 0; i < expr.expr.call.args->length; i++) {
				printf(" ");
				print_expr(expr.expr.call.args->exprs[i]);
			}
			printf(")");
			break;
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
		if (i < exprs->length - 1)
			printf(exprs->store_delimiters ? (char[]){exprs->delimiters.buffer[i], ' ', '\0'} : " ");
	}

	printf("]");
}
