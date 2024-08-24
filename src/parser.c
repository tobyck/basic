#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <inttypes.h>

#include "parser.h"
#include "utils.h"
#include "debug.h"

ExprList *empty_expr_list() {
	ExprList *exprs = malloc(sizeof(ExprList));
	exprs->exprs = malloc(0);

	if (exprs == NULL || exprs->exprs == NULL) {
		printf("Error: could not allocate memory for new expression list\n");
		exit(EXIT_FAILURE);
	}

	exprs->length = 0;

	return exprs;
}

void push_expr(ExprList *exprs, Expr expr) {
	exprs->exprs = realloc(exprs->exprs, sizeof(Statement) * (exprs->length + 1));

	if (exprs->exprs == NULL) {
		printf("Error: could not allocate more memory for new expr in list\n");
		exit(EXIT_FAILURE);
	}

	exprs->exprs[exprs->length++] = expr;
}

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
		printf("Error: could not allocate more memory for another statement in the AST\n");
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

ParseErrorList empty_parse_error_list() {
	ParseError *errors = malloc(0);

	if (errors == NULL) {
		printf("Error: could not allocate memory for a new parse error list\n");
		exit(EXIT_FAILURE);
	}

	return (ParseErrorList){ errors, 0 };
}

void push_parse_error(ParseErrorList *errors, ParseError error) {
	errors->errors = realloc(errors->errors, sizeof(ParseError) * (errors->length + 1));

	if (errors->errors == NULL) {
		printf("Error: could not allocate more memory for new parse error\n");
		exit(EXIT_FAILURE);
	}

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
	if (token == NULL) return false;

	switch (token->type) {
		case TOKEN_END_STATEMENT: return true;
		default: return false;
	}
}

 FuncInfo get_func_info(char *func_name) {
	if (strlen(func_name) == 1) {
		if (strchr("+-", func_name[0])) return (FuncInfo){ 2, 1, 2 };
		if (strchr("*/", func_name[0])) return (FuncInfo){ 2, 3, 4 };
	}

	// in theory this will never happen
	return (FuncInfo){};
}

ParseExprResult parse_expr(TokenLinkedList *tokens, uint8_t min_binding_power) {
	Token *token = pop_token(tokens);
	Expr lhs;

	switch (token->type) {
		case TOKEN_INT:
			lhs = (Expr){ EXPR_INT, { .int_literal = strdup(token->literal) } }; break;
		case TOKEN_OPEN_PAREN: {
			// parse the expression inside parentheses
			ParseExprResult lhs_result = parse_expr(tokens, 0);

			if (lhs_result.success) {
				// if we parsed that successfully, then ensure it's closed correctly
				Token *closing_paren = pop_token(tokens);
				if (closing_paren->type != TOKEN_CLOSE_PAREN) {
					return (ParseExprResult){ false, { .error = {
						strdup("Expected closing parenthesis"), token
					} } };
				}
				free_token(closing_paren);
				lhs = lhs_result.result.expr;
			} else {
				free_token(token);
				return lhs_result;
			}
			break;
		}
		default: {
			char *error_msg = strdup("Unexpected ");
			append_str(&error_msg, stringify_token_type(token->type));
			append_str(&error_msg, " token");
			return (ParseExprResult){ false, { .error = { error_msg, token } } };
		}
	}

	free_token(token);

	// continually try to parse more operators
	while (true) {
		if (token_ends_expr(tokens->head)) break;

		Token *op = tokens->head;

		if (op->type == TOKEN_CLOSE_PAREN) break;

		// error if op is invalid
		else if (op->type != TOKEN_BINARY_OP && op->type != TOKEN_UNARY_OP) {
			char *error_msg = strdup("Expected operator, received ");
			append_str(&error_msg, stringify_token_type(op->type));
			return (ParseExprResult){ false, { .error = { error_msg, op } } };
		}

		FuncInfo func_info = get_func_info(op->literal);

		if (func_info.arity == 2) {
			if (func_info.left_binding_power < min_binding_power)
				break;

			pop_token(tokens);

			// make sure there are more tokens before we recurse
			if (token_ends_expr(tokens->head)) {
				free_expr(lhs);
				return (ParseExprResult){ false, { .error = { strdup("Incomplete expression"), op } } };
			}

			// find out what the right hand side of the current operator will be
			ParseExprResult rhs_result = parse_expr(tokens, func_info.right_binding_power);
			Expr rhs;

			if (rhs_result.success) rhs = rhs_result.result.expr;
			else {
				// return error early if parsing rhs failed
				free_expr(lhs);
				free_token(op);
				return rhs_result;
			}

			// update the left hand side to be the expression we've just parsed
			ExprList *args = empty_expr_list();
			push_expr(args, lhs);
			push_expr(args, rhs);
			lhs = (Expr){ EXPR_FUNC, { .func = { strdup(op->literal), args } } };

			free_token(op);
		} else break;
	}

	// the expression will bulid up in lhs; return that at the end
	return (ParseExprResult){ true, { .expr = lhs } };
}

ParserResult parse(TokenLinkedList tokens) {
	AST ast = empty_ast();
	ParseErrorList errors = empty_parse_error_list();

	// parsing the whole program as a single expression just for testing
	ParseExprResult r = parse_expr(&tokens, 0);
	if (r.success) {
		print_expr(r.result.expr);
		printf("\n");
		free_expr(r.result.expr);
	} else {
		push_parse_error(&errors, r.result.error);
	}
	free_token_linked_list(tokens);

	return (ParserResult){ ast, errors };
}
