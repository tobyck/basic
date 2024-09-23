#ifndef INCLUDE_DEBUG_H
#define INCLUDE_DEBUG_H

#include "lexer.h"

extern void print_token(Token token);
extern void _print_token_buffer_from(TokenBuffer buffer, size_t start, size_t end);
extern void print_token_buffer(TokenBuffer buffer);

#endif  // INCLUDE_DEBUG_H
