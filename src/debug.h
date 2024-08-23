#ifndef INCLUDE_DEBUG_H
#define INCLUDE_DEBUG_H

#include "lexer.h"
#include "parser.h"

extern char *stringify_token_type(TokenType token_type);
extern void print_token(Token token);
extern void print_token_linked_list(Token *tokens);

extern void print_lexer_errors(LexerErrorList errors);

extern void print_expr(Expr expr);
extern void print_expr_list(ExprList *exprs);
extern void print_ast(AST ast);

#endif // INCLUDE_DEBUG_H
