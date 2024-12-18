#ifndef AST_NODE_H
#define AST_NODE_H

#include "common.h"
#include "scanner.h"
#include "syntax_types.h"

typedef struct s_SyntaxNode {
	SYNTAX_TYPE type;

	bool is_token;
	Token token;

	size_t n_children;
	struct s_SyntaxNode **children;
	struct s_SyntaxNode *parent;

	bool had_error;
	char *msg;
} SyntaxNode;

void initSyntaxNode(SyntaxNode *node);

#endif // AST_NODE_H
