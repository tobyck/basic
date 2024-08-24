#ifndef INCLUDE_DEBUG_H
#define INCLUDE_DEBUG_H

#include "lexer.h"
#include "parser.h"

extern void print_token(Token *token);
extern void print_token_linked_list(TokenLinkedList tokens);

extern void print_lexer_errors(LexerErrorList errors);

extern void print_expr(Expr expr);
extern void print_expr_list(ExprList *exprs);

extern void print_ast(AST ast);

extern void print_parse_errors(ParseErrorList errors);

#endif // INCLUDE_DEBUG_H
