#ifndef SYNTAXTREE_H
#define SYNTAXTREE_H

#include "mempool.h"
#include "syntax_types.h"
#include "syntax_node.h"

typedef struct s_SyntaxTree {
	char *source;

	size_t n_tokens;
	Token *tokens;

	SyntaxNode *head;

	MemPool pool;
	// int err;
} SyntaxTree;

void initSyntaxTree(SyntaxTree *tree);

void resetSyntaxTree(SyntaxTree *tree);

void termSyntaxTree(SyntaxTree *tree);

#endif // SYNTAXTREE_H
