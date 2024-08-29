#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <inttypes.h>
#include <stdarg.h>

#include "parser.h"
#include "utils.h"
#include "debug.h"

void free_expr(Expr expr) {
	switch (expr.type) {
		case EXPR_NUMBER: free(expr.expr.number_literal); break;
		case EXPR_STRING: free(expr.expr.string_literal); break;
		case EXPR_VAR: free(expr.expr.variable); break;
		case EXPR_CALL:
			free(expr.expr.call.name);
			free_expr_list(expr.expr.call.args);
			break;
		default: break;
	}
}

ExprList *empty_expr_list(bool store_delimiters) {
	ExprList *exprs = malloc(sizeof(ExprList));

	exprs->exprs = malloc(0);

	ensure_alloc(exprs);
	ensure_alloc(exprs->exprs);

	exprs->length = 0;
	exprs->stored_delimiters = store_delimiters;

	if (store_delimiters)
		exprs->delimiters = empty_buffered_string(4);

	return exprs;
}

// creates an expression list of known values without
// doing lots of separate memory allocations
ExprList *new_expr_list_from(size_t count, ...) {
	va_list args;
	va_start(args, count);

	ExprList *exprs = malloc(sizeof(ExprList));
	exprs->exprs = malloc(sizeof(Expr) * count);

	for (int i = 0; i < count; i++)
		exprs->exprs[i] = va_arg(args, Expr);

	exprs->length = count;
	exprs->stored_delimiters = false;

	return exprs;
}

void push_expr(ExprList *exprs, Expr expr) {
	exprs->exprs = realloc(exprs->exprs, sizeof(Statement) * (exprs->length + 1));
	ensure_alloc(exprs->exprs);
	exprs->exprs[exprs->length++] = expr;
}

void free_expr_list(ExprList *exprs) {
	for (size_t i = 0; i < exprs->length; i++)
		free_expr(exprs->exprs[i]);

	free(exprs->exprs);

	if (exprs->stored_delimiters)
		free(exprs->delimiters.buffer);

	free(exprs);
}

AST empty_ast() {
	Statement *statements = malloc(0);
	ensure_alloc(statements);
	return (AST){ statements, 0 };
}

void push_statement(AST *ast, Statement statement) {
	ast->statements = realloc(ast->statements, sizeof(Statement) * (ast->length + 1));
	ensure_alloc(ast->statements);
	ast->statements[ast->length++] = statement;
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

ParseErrorList empty_parse_error_list() {
	ParseError *errors = malloc(0);
	ensure_alloc(errors);
	return (ParseErrorList){ errors, 0 };
}

void push_parse_error(ParseErrorList *errors, ParseError error) {
	errors->errors = realloc(errors->errors, sizeof(ParseError) * (errors->length + 1));
	ensure_alloc(errors->errors);
	errors->errors[errors->length++] = error;
}

void free_parse_error_list(ParseErrorList errors) {
	for (size_t i = 0; i < errors.length; i++) {
		free(errors.errors[i].message);
		free_token(errors.errors[i].token);
	}

	free(errors.errors);
}

bool token_ends_expr(Token *token) {
	if (token == NULL) return true;

	switch (token->type) {
		case TOKEN_NUMBER:
		case TOKEN_BINARY_OP:
		case TOKEN_UNARY_OP:
		case TOKEN_OPEN_PAREN:
		case TOKEN_NAME: return false;
		default: return true;
	}
}

bool token_ends_expr_list(Token *token) {
	if (token == NULL) return true;
	if (token->type == TOKEN_COMMA || token->type == TOKEN_SEMICOLON) return false;
	return token_ends_expr(toke n);
}

BindingPower get_op_binding_power(Token token) {
	if (token.type == TOKEN_UNARY_OP) {
		if (strcmp("-", token.literal) == 0) return (BindingPower){ -1, 5 };
	} else if (token.type == TOKEN_BINARY_OP && strlen(token.literal) == 1) {
		if (strchr("+-", token.literal[0])) return (BindingPower){ 1, 2 };
		if (strchr("*/", token.literal[0])) return (BindingPower){ 3, 4 };
		if (token.literal[0] == '^') return (BindingPower){ 7, 6 };
	}

	// in theory this will never happen
	return (BindingPower){};
}

inline ParseExprResult parse_expr_error(char *string, Token *token) {
	return (ParseExprResult){ false, { .error = { strdup(string), token } } };
}

inline ParseExprResult parse_expr_success(Expr expr) {
	return (ParseExprResult){ true, { .expr = expr } };
}

// given list of tokens which is assumed to be non-empty, this attempts to parse
// any kind of expression, consumes tokens as it goes, then returns either an
// Expr or ParseError wrapped in a struct.
ParseExprResult parse_expr(TokenLinkedList *tokens, bool allow_string) {
	if (allow_string && tokens->head->type == TOKEN_STRING) {
		Token *token = pop_token(tokens);

		if (!token_ends_expr(token->next)) {
			free_token(token);
			return parse_expr_error(
				"Invalid token following string",
				pop_token(tokens)
			);
		}

		char *string = token->literal;
		// only free the token struct and not the literal,
		// because that's being used in the expression
		free(token);

		return parse_expr_success((Expr){
			EXPR_STRING,
			{ .string_literal = string }
		});
	}

	return parse_math_expr(tokens, 0);
}

// same as the function above, except only for mathematical expressions
// (so this does most of the heavy lifting for expression parsing)
ParseExprResult parse_math_expr(TokenLinkedList *tokens, uint8_t min_binding_power) {
	Token *token = pop_token(tokens);
	Expr lhs;

	switch (token->type) {
		case TOKEN_NUMBER:
			lhs = (Expr){ EXPR_NUMBER, { .number_literal = token->literal } };
			free(token);
			break;
		case TOKEN_UNARY_OP: {
			// parse expr for arg of unary op
			BindingPower binding_power = get_op_binding_power(*token);
			ParseExprResult arg_result = parse_math_expr(tokens, binding_power.right);

			if (arg_result.success) {
				lhs = (Expr){ EXPR_CALL, { .call = {
					token->literal,
					new_expr_list_from(1, arg_result.result.expr)
				} } };
				free(token); // only free struct not the literal inside
				break;
			} else {
				free_token(token);
				return arg_result; // return the error caused by trying to parse the arg
			}
		}
		case TOKEN_OPEN_PAREN: {
			// parse the expression inside parentheses
			ParseExprResult result = parse_math_expr(tokens, 0);

			if (result.success) {
				// if we parsed that successfully, then ensure it's closed correctly
				Token *closing_paren = pop_token(tokens);
				if (closing_paren->type != TOKEN_CLOSE_PAREN) {
					free_token(token);
					free_expr(result.result.expr);
					return parse_expr_error(strdup("Expected closing parenthesis"), closing_paren);
				}
				free_token(closing_paren);
				free_token(token);
				lhs = result.result.expr;
				break;
			} else {
				free_token(token);
				return result;
			}
		}
		case TOKEN_NAME:
			if (token->next->type == TOKEN_OPEN_PAREN) {
				free_token(pop_token(tokens)); // consume open paren

				ParseExprListResult args_result = parse_expr_list(tokens, false, false);

				if (args_result.success) {
					// again, ensure proper closing of parenthesis
					Token *closing_paren = pop_token(tokens);
					if (closing_paren->type != TOKEN_CLOSE_PAREN) {
						free_token(token);
						free_expr_list(args_result.result.exprs);
						return parse_expr_error("Expected closing parenthesis", closing_paren);
					}
					free_token(closing_paren);

					lhs = (Expr){ EXPR_CALL, { .call = {
						token->literal, args_result.result.exprs
					} } };
				} else {
					free_token(token);
					return (ParseExprResult){ false, { .error = args_result.result.error } };
				}
			} else
				lhs = (Expr){ EXPR_VAR, { .variable = token->literal } };

			free(token);

			break;
		case TOKEN_STRING:
			return parse_expr_error("Math cannot be done with strings", token);
		default: {
			char *error_msg = strdup("Unexpected ");
			append_str(&error_msg, stringify_token_type(token->type));
			append_str(&error_msg, " token");
			return (ParseExprResult){ false, { .error = { error_msg, token } } };
		}
	}

	// continually try to parse more operators
	while (true) {
		if (token_ends_expr(tokens->head)) break;

		Token *op = tokens->head;

		// error if op is invalid
		if (op->type != TOKEN_BINARY_OP) {
			pop_token(tokens);
			free_expr(lhs);
			char *error_msg = strdup("Expected BINARY_OP, received ");
			append_str(&error_msg, stringify_token_type(op->type));
			return (ParseExprResult){ false, { .error = { error_msg, op } } };
		}

		BindingPower binding_power = get_op_binding_power(*op);

		if (binding_power.left < min_binding_power)
			break;

		pop_token(tokens);

		// make sure there are more tokens before we recurse
		if (token_ends_expr(tokens->head)) {
			free_expr(lhs);
			return parse_expr_error("Incomplete expression", op);
		}

		// find out what the right hand side of the current operator will be
		ParseExprResult rhs_result = parse_math_expr(tokens, binding_power.right);
		Expr rhs;

		if (rhs_result.success) rhs = rhs_result.result.expr;
		else {
			// return error early if parsing rhs failed
			free_expr(lhs);
			free_token(op);
			return rhs_result;
		}

		// update the left hand side to be the expression we've just parsed
		lhs = (Expr){ EXPR_CALL, { .call = {
			op->literal,
			new_expr_list_from(2, lhs, rhs)
		} } };

		free(op);
	}

	// the expression will bulid up in lhs; return that at the end
	return parse_expr_success(lhs);
}

ParseExprListResult parse_expr_list(TokenLinkedList *tokens, bool allow_string, bool store_delimiters) {
	ExprList *exprs = empty_expr_list(store_delimiters);

	while (true) {
		ParseExprResult expr_result = parse_expr(tokens, allow_string);

		if (expr_result.success) push_expr(exprs, expr_result.result.expr);
		else {
			free_expr_list(exprs);
			return (ParseExprListResult){ false, { .error = expr_result.result.error } };
		}

		if (token_ends_expr_list(tokens->head)) break;

		Token *delimiter = pop_token(tokens);
		if (store_delimiters) {
			char delimiter_char = delimiter->literal[0];
			buffered_string_append_char(&exprs->delimiters, delimiter_char);
		}
		free_token(delimiter);
	}

	return (ParseExprListResult){ true, { .exprs = exprs } };
}

ParserResult parse(TokenLinkedList tokens) {
	AST ast = empty_ast();
	ParseErrorList errors = empty_parse_error_list();

	if (tokens.head != NULL) {
		/* ParseExprResult r = parse_expr(&tokens, true);
		if (r.success) {
			print_expr(r.result.expr);
			printf("\n");
			free_expr(r.result.expr);
		} else {
			push_parse_error(&errors, r.result.error);
		}
		free_token_linked_list(&tokens); */
		ParseExprListResult r = parse_expr_list(&tokens, true, true);
		if (r.success) {
			print_expr_list(r.result.exprs);
			printf("\n");
			free_expr_list(r.result.exprs);
		} else {
			push_parse_error(&errors, r.result.error);
		}
		free_token_linked_list(&tokens);
	}

	return (ParserResult){ ast, errors };
}
