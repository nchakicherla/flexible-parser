#include "syntax_tree.h"
#include "mempool.h"

void initSyntaxTree(SyntaxTree *tree) {
	tree->source = NULL;

	tree->n_tokens = 0;
	tree->tokens = NULL;

	tree->head = NULL;
	initMemPool(&(tree->pool));
	return;
}

void resetSyntaxTree(SyntaxTree *tree) {
	tree->source = NULL;

	tree->n_tokens = 0;
	tree->tokens = NULL;
	
	tree->head = NULL;
	resetMemPool(&(tree->pool));
}

void termSyntaxTree(SyntaxTree *tree) {
	termMemPool(&(tree->pool));
	return;
}
