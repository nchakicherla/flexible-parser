#include "ast_node.h"

void initSyntaxNode(SyntaxNode *node) {
	node->type = STX_ERROR;
	node->is_token = false;
	node->n_children = 0;
	node->children = NULL;
	node->parent = NULL;
	node->had_error = false;
	node->msg = NULL;
}
