#include <stdlib.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>

#include "parser.h"
#include "lexer.h"
#include "utils.h"
#include "debug.h"

void free_expr(Expr expr) {
	switch (expr.type) {
		case EXPR_NUMBER: free(expr.expr.number_literal); break;
		case EXPR_STRING: free(expr.expr.string_literal); break;
		case EXPR_VAR: free(expr.expr.variable); break;
		case EXPR_CALL:
			if (expr.expr.call.name_string != NULL)
				free(expr.expr.call.name_string);
			free_expr_list(expr.expr.call.args);
	}
}

ExprList *new_expr_list_from(size_t length, ...) {
	va_list args;
	va_start(args, length);

	ExprList *exprs = malloc(sizeof(ExprList));
	ensure_alloc(exprs);

	exprs->exprs = malloc(sizeof(Expr) * length);
	ensure_alloc(exprs->exprs);

	for (int i = 0; i < length; i++)
		exprs->exprs[i] = va_arg(args, Expr);

	exprs->length = length;
	exprs->store_delimiters = false;

	return exprs;
}

ExprList *empty_expr_list(bool store_delimiters) {
	ExprList *exprs = malloc(sizeof(ExprList));
	ensure_alloc(exprs);

	exprs->exprs = malloc(0);
	ensure_alloc(exprs->exprs);

	exprs->length = 0;
	exprs->store_delimiters = store_delimiters;

	if (store_delimiters)
		exprs->delimiters = empty_buffered_string(4);

	return exprs;
}

void push_expr(ExprList *exprs, Expr expr) {
	exprs->exprs = realloc(exprs->exprs, sizeof(Expr) * (exprs->length + 1));
	ensure_alloc(exprs->exprs);
	exprs->exprs[exprs->length++] = expr;
}

void free_expr_list(ExprList *exprs) {
	for (size_t i = 0; i < exprs->length; i++)
		free_expr(exprs->exprs[i]);

	free(exprs->exprs);
	if (exprs->store_delimiters) free(exprs->delimiters.buffer);
	free(exprs);
}

AST new_ast(void) {
	Statement *statements = malloc(0);
	ensure_alloc(statements);
	return (AST){ statements, .length = 0 };
}

ParseExprResult expected_expression_error(Lexer *lexer) {
	Token *previous_token = get_most_recent_token(lexer);
	size_t line, column;

	if (previous_token == NULL) {
		// if there are no previous tokens then error at the very beginning
		line = 1;
		column = 1;
	} else {
		// otherwise error at the column after the previous token
		line = previous_token->line;
		column = previous_token->column + 1;
	}

	return (ParseExprResult){ false, { .error = {
		strdup("Expected expression"), line, column, -1
	} } };
}

ParseExprResult parse_expr(Lexer *lexer, bool allow_string) {
	TokenResult first_token_result = peek_token(lexer);

	if (!first_token_result.success) return expected_expression_error(lexer);

	// if the expression is allowed to be a string then try to parse it as that
	if (allow_string) {
		Token first_token = first_token_result.result.token;
		if (first_token.type == TOKEN_STRING) {
			next_token(lexer);
			return (ParseExprResult){ true, { .expr = {
				EXPR_STRING, { .string_literal = first_token.string_literal }
			} } };
		}
	}

	// default to a mathematical expression
	return parse_math_expr(lexer, 0);
}

BindingPower get_binding_power(Token token) {
	if (token.type == TOKEN_UNARY_OP) {
		if (token.char_literal == '-') return (BindingPower){ -1, 5 };
	} else if (token.type == TOKEN_BINARY_OP) {
		if (strchr("+-", token.char_literal)) return (BindingPower){ 1, 2 };
		if (strchr("*/", token.char_literal)) return (BindingPower){ 3, 4 };
		if (token.char_literal == '^') return (BindingPower){ 7, 6 };
	}

	printf("Error: attempted to find binding power of unknown operator\n");
	exit(EXIT_FAILURE);
}

bool token_ends_expr(TokenResult token_result) {
	if (!token_result.success) return true;

	switch (token_result.result.token.type) {
		case TOKEN_NUMBER:
		case TOKEN_STRING:
		case TOKEN_BINARY_OP:
		case TOKEN_UNARY_OP:
		case TOKEN_OPEN_PAREN:
		case TOKEN_NAME: return false;
		default: return true;
	}
}

bool token_ends_expr_list(TokenResult token_result) {
	if (!token_result.success) return true;
	Token token = token_result.result.token;
	if (token.type == TOKEN_COMMA || token.type == TOKEN_SEMICOLON) return false;
	return token_ends_expr(token_result);
}

ParseExprResult parse_math_expr(Lexer *lexer, uint8_t min_binding_power) {
	TokenResult token_result = next_token(lexer);
	if (!token_result.success) return expected_expression_error(lexer);

	Token token = token_result.result.token;
	Expr lhs;

	switch (token.type) {
		case TOKEN_NUMBER:
			lhs = (Expr){ EXPR_NUMBER, { .number_literal = token.string_literal } };
			break;
		case TOKEN_STRING:
			free_token_literal(token);
			return (ParseExprResult){ false, { .error = {
				strdup("Math cannot be done with strings"), token.line, token.column, -1
			} } };
		case TOKEN_UNARY_OP: {
			BindingPower binding_power = get_binding_power(token);
			ParseExprResult arg_result = parse_math_expr(lexer, binding_power.right);

			if (arg_result.success) {
				lhs = (Expr){ EXPR_CALL, { .call = {
					.name_char = token.char_literal,
					.args = new_expr_list_from(1, arg_result.result.expr)
				} } };
				break;
			} else return arg_result;
		}
		case TOKEN_OPEN_PAREN: {
			ParseExprResult expr_result = parse_math_expr(lexer, 0);

			if (expr_result.success) {
				// if we parsed that successfully, then ensure it's closed correctly
				TokenResult closing_paren = next_token(lexer);

				if (!closing_paren.success || closing_paren.result.token.type != TOKEN_CLOSE_PAREN) {
					free_expr(expr_result.result.expr);
					Token *previous_token = get_most_recent_token(lexer);
					return (ParseExprResult){ false, { .error = {
						strdup("Expected closing parenthesis"),
						previous_token->line, token.column, previous_token->column
					} } };
				}

				lhs = expr_result.result.expr;
				break;
			} else return expr_result;
		}
		case TOKEN_NAME: {
			TokenResult open_paren = peek_token(lexer);

			if (open_paren.success && open_paren.result.token.type == TOKEN_OPEN_PAREN) {
				next_token(lexer); // consume open paren

				// the falses here disable storing delimiters and allowing string expressions
				ParseExprListResult args_result = parse_expr_list(lexer, false, false);

				if (args_result.success) {
					TokenResult closing_paren = next_token(lexer);

					if (!closing_paren.success || closing_paren.result.token.type != TOKEN_CLOSE_PAREN) {
						free_expr_list(args_result.result.exprs);
						Token *previous_token = get_most_recent_token(lexer);
						return (ParseExprResult){ false, { .error = {
							strdup("Expected closing parenthesis"),
							previous_token->line, token.column, previous_token->column
						} } };
					}

					lhs = (Expr){ EXPR_CALL, { .call = {
						.name_string = token.string_literal, .args = args_result.result.exprs
					} } };
				} else return (ParseExprResult){ false, { .error = args_result.result.error } };
			} else lhs = (Expr){ EXPR_VAR, { .variable = token.string_literal } };

			break;
		}
		default: {
			char *error_msg = strdup("Unexpected token: ");
			append_str(&error_msg, stringify_token_type(token.type));
			return (ParseExprResult){ false, { .error = { error_msg, token.line, token.column, -1 } } };
		}
	}

	// continually try to parse more operators
	while (true) {
		TokenResult token_result = peek_token(lexer);
		if (!token_result.success || token_ends_expr(token_result)) break;
		
		Token op = token_result.result.token;

		if (op.type != TOKEN_BINARY_OP) {
			next_token(lexer); // consume the operator so it's out of the way for whatever we parse next
			free_expr(lhs);
			char *error_msg = strdup("Expected BINARY_OP, received ");
			append_str(&error_msg, stringify_token_type(op.type));
			return (ParseExprResult){ false, { .error = { error_msg, op.line, op.column, -1 } } };
		}

		BindingPower binding_power = get_binding_power(op);

		if (binding_power.left < min_binding_power)
			break;

		// now that we know we're actually going to parse this operator (because of
		// the check above) we can consume the operator and parse the rhs
		next_token(lexer);
		
		// make sure there are more tokens before we recurse
		TokenResult next_token_result = peek_token(lexer);
		if (!next_token_result.success || token_ends_expr(next_token_result)) {
			free_expr(lhs);
			return expected_expression_error(lexer);
		}

		// find out what the right hand side of the current operator is
		ParseExprResult rhs_result = parse_math_expr(lexer, binding_power.right);
		Expr rhs;

		if (rhs_result.success) 
			rhs = rhs_result.result.expr;
		else {
			free_expr(lhs);
			return rhs_result;
		}

		// update the left hand side to be the expression we've just parsed
		lhs = (Expr){ EXPR_CALL, { .call = {
			.name_char = op.char_literal,
			.args = new_expr_list_from(2, lhs, rhs)
		} } };
	}

	// the expression will bulid up in lhs; return that at the end
	return (ParseExprResult){ true, { .expr = lhs } };
}

ParseExprListResult parse_expr_list(Lexer *lexer, bool allow_string, bool store_delimiters) {
	ExprList *exprs = empty_expr_list(store_delimiters);

	while (true) {
		ParseExprResult expr_result = parse_expr(lexer, allow_string);

		if (expr_result.success) push_expr(exprs, expr_result.result.expr);
		else {
			free_expr_list(exprs);
			return (ParseExprListResult){ false, { .error = expr_result.result.error } };
		}

		if (token_ends_expr_list(peek_token(lexer))) break;

		TokenResult delimiter_result = next_token(lexer);
		if (store_delimiters && delimiter_result.success) {
			char delimiter_char = delimiter_result.result.token.char_literal;
			buffered_string_append_char(&exprs->delimiters, delimiter_char);
		}
	}

	return (ParseExprListResult){ true, { .exprs = exprs } };
}

ParserResult parse(char *code) {
	Lexer *lexer = new_lexer(code, 3);
	AST ast = new_ast();

	ParseExprListResult e = parse_expr_list(lexer, true, true);
	if (e.success) {
		print_expr_list(e.result.exprs);
		printf("\n");
		free_expr_list(e.result.exprs);
	} else {
		printf("%s\n", e.result.error.message);
		free(e.result.error.message);
	}

	free_lexer(lexer);

	return (ParserResult){
		true,
		{ .ast = ast }
	};
}
