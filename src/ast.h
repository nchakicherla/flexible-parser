#ifndef AST_H
#define AST_H

#include <string.h>
#include "common.h"

#include "mempool.h"
#include "scanner.h"
#include "file.h"
#include "ast_node.h"
#include "syntax_types.h"
#include "grammar.h"

typedef struct s_TokenStream {
	Token *tk;
	size_t pos;
	size_t n;
} TokenStream;

void defineParentPtrs(SyntaxNode *head);
void addChild(SyntaxNode *parent, SyntaxNode *child, MemPool *pool);
SyntaxNode *wrapNode(SyntaxNode *child, SYNTAX_TYPE stype, MemPool *pool);
SyntaxNode *parseAnd(RuleNode *rnode, TokenStream *stream, MemPool *pool);
SyntaxNode *parseOr(RuleNode *rnode, TokenStream *stream, MemPool *pool);
SyntaxNode *parseIfOne(RuleNode *rnode, TokenStream *stream, MemPool *pool);
SyntaxNode *parseIfMany(RuleNode *rnode, TokenStream *stream, MemPool *pool);
SyntaxNode *parseSyntax(RuleNode *rnode, TokenStream *stream, MemPool *pool);
SyntaxNode *parseToken(RuleNode *rnode, TokenStream *stream, MemPool *pool);
SyntaxNode *parseGrammar(RuleNode* rnode, TokenStream *stream, MemPool *pool);
void printTokenStream(TokenStream *stream);
void printSyntaxNode(SyntaxNode *node, unsigned int indent);

#endif // AST_H
