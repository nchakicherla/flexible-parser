#ifndef AST_H
#define AST_H

/* ast.h - matching a token stream against a rule tree.
 *
 * Every parse function obeys one invariant: on failure it returns NULL and
 * leaves the stream position exactly where it found it. That is what makes
 * alternation and repetition safe to backtrack through.
 */

#include "common.h"
#include "mempool.h"
#include "grammar.h"
#include "scanner.h"
#include "ast_node.h"

#include <stdio.h>

typedef struct s_TokenStream {
	Token *tk;
	size_t n;
	size_t pos;
	size_t furthest; /* high-water mark, so a failed parse can say where it gave up */
	unsigned depth;
	bool depth_exceeded; /* set once; almost always means a left-recursive rule */
} TokenStream;

/* Matches `rnode` at the current position. */
SyntaxNode *parseNode(const RuleNode *rnode, TokenStream *stream, MemPool *pool);

/* Matches the grammar's start rule and requires the whole stream to be
 * consumed. Returns NULL and reports the furthest position reached on failure. */
SyntaxNode *parseTokenStream(const Grammar *grammar, TokenStream *stream, MemPool *pool);

void printSyntaxNode(Registry *reg, const SyntaxNode *node, unsigned indent);
void fPrintSyntaxNode(Registry *reg, const SyntaxNode *node, unsigned indent, FILE *file);

#endif // AST_H
