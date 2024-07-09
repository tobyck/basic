#ifndef INCLUDE_PARSER_H
#define INCLUDE_PARSER_H

#include <stdlib.h>

typedef enum {
	AST_ASSIGNMENT,
	AST_EXPRESSION,
	AST_STRING,
	AST_PRINT
} ASTNodeType;

typedef enum {
	MATH_OP_ADD,
	MATH_OP_SUB,
	MATH_OP_MUL,
	MATH_OP_DIV,
	MATH_OP_MOD,
} MathOp;

typedef struct {
	struct ASTExpression *exprs;
	size_t length;
} ASTExpressionList;

typedef struct ASTNode {
	ASTNodeType type;
	union {
		struct ASTExpression {
			MathOp op;
			struct ASTExpression *lhs;
			struct ASTExpression *rhs;
		} expr;
		struct ASTAssignment {
			char variable;
			struct ASTExpression *expr;
		} assingment;
		char *string;
		ASTExpressionList print;
	} node;
} ASTNode;

typedef struct {
	ASTNode nodes;
	size_t length;
} AST;

extern AST new_ast();

#endif // INCLUDE_PARSER_H
