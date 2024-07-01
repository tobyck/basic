#ifndef INCLUDE_PARSER_H
#define INCLUDE_PARSER_H

#include "lexer.h"

typedef struct ASTNode ASTNode;

struct ASTNode {
	enum {
		ASSIGNMENT
	} type;
	union {
		struct Assignment {
			char *var; // variable being assigned to
			AST expr; // expression to evaluate and assign
		} assignment;
	} node;
};

// top level AST
typedef struct {
	ASTNode *nodes;
	size_t length;
} AST;

typedef struct {
	AST ast;
	char *error;
} ParserResult;

extern ParserResult parse(TokenList tokens);

#endif  // INCLUDE_PARSER_H
