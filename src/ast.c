#include "ast.h"
#include "color.h"

#include <stdio.h>
#include <string.h>

static SyntaxNode *newAnonNode(MemPool *pool) {
	SyntaxNode *node = palloc(pool, sizeof(SyntaxNode));
	initSyntaxNode(node);
	return node;
}

static void addChild(SyntaxNode *parent, SyntaxNode *child, MemPool *pool) {
	if (parent->n_children == 0) {
		parent->children = palloc(pool, sizeof(SyntaxNode *));
	} else {
		parent->children = pGrowAlloc(parent->children,
		                              parent->n_children * sizeof(SyntaxNode *),
		                              (parent->n_children + 1) * sizeof(SyntaxNode *),
		                              pool);
	}
	parent->children[parent->n_children] = child;
	parent->n_children++;
	child->parent = parent;
}

/* Anonymous nodes are grouping artifacts, not structure the caller asked for,
 * so their children are hoisted into the parent instead of nesting. Doing this
 * at every append keeps the tree flat by construction - the old code had to
 * special-case the splice in three different places. */
static void appendResult(SyntaxNode *parent, SyntaxNode *child, MemPool *pool) {
	if (child->is_anonymous && !child->is_token) {
		for (size_t i = 0; i < child->n_children; i++) {
			appendResult(parent, child->children[i], pool);
		}
		return;
	}
	addChild(parent, child, pool);
}

static SyntaxNode *wrapNode(SyntaxNode *child, SYNTAX_TYPE stype, MemPool *pool) {
	SyntaxNode *parent = newAnonNode(pool);
	parent->type = stype;
	parent->is_anonymous = false;
	addChild(parent, child, pool);
	return parent;
}

static SyntaxNode *parseTerminal(const RuleNode *rnode, TokenStream *stream, MemPool *pool) {
	SyntaxNode *node;

	if (stream->pos > stream->furthest) {
		stream->furthest = stream->pos;
	}
	if (stream->tk[stream->pos].type != rnode->as.t) {
		return NULL;
	}

	node = newAnonNode(pool);
	node->is_token = true;
	node->token = stream->tk[stream->pos];
	stream->pos++;
	return node;
}

/* A rule reference names whatever its body produced. If the body already came
 * back named - or is a bare token - renaming it would destroy that name, so it
 * gets wrapped instead. */
static SyntaxNode *parseRuleRef(const RuleNode *rnode, TokenStream *stream, MemPool *pool) {
	SyntaxNode *node = parseNode(rnode->rule_reference, stream, pool);

	if (!node) {
		return NULL;
	}
	if (node->is_token || !node->is_anonymous) {
		return wrapNode(node, rnode->as.s, pool);
	}
	node->type = rnode->as.s;
	node->is_anonymous = false;
	return node;
}

static SyntaxNode *parseSequence(const RuleNode *rnode, TokenStream *stream, MemPool *pool) {
	SyntaxNode *node = newAnonNode(pool);
	size_t start = stream->pos;

	for (size_t i = 0; i < rnode->n_children; i++) {
		SyntaxNode *child = parseNode(rnode->children[i], stream, pool);
		if (!child) {
			stream->pos = start;
			return NULL;
		}
		appendResult(node, child, pool);
	}
	return node;
}

static SyntaxNode *parseAlternation(const RuleNode *rnode, TokenStream *stream, MemPool *pool) {
	size_t start = stream->pos;

	for (size_t i = 0; i < rnode->n_children; i++) {
		SyntaxNode *child;
		stream->pos = start;
		child = parseNode(rnode->children[i], stream, pool);
		if (child) {
			return child;
		}
	}
	stream->pos = start;
	return NULL;
}

/* Optional and repeated groups never fail - they match empty. Because of that,
 * a sequence can treat any NULL child as a hard failure, which is what removes
 * the nest of "was this child optional?" checks from the old parseAnd. */
static SyntaxNode *parseOptional(const RuleNode *rnode, TokenStream *stream, MemPool *pool) {
	size_t start = stream->pos;
	SyntaxNode *child = parseNode(rnode->children[0], stream, pool);

	if (!child) {
		stream->pos = start;
		return newAnonNode(pool);
	}
	return child;
}

static SyntaxNode *parseRepetition(const RuleNode *rnode, TokenStream *stream, MemPool *pool) {
	SyntaxNode *node = newAnonNode(pool);

	while (true) {
		size_t reset = stream->pos;
		SyntaxNode *child = parseNode(rnode->children[0], stream, pool);

		if (!child) {
			stream->pos = reset;
			break;
		}
		/* A body that can match empty (say, a group of optionals) would spin
		 * here forever. Consuming nothing ends the repetition. */
		if (stream->pos == reset) {
			break;
		}
		appendResult(node, child, pool);
	}
	return node;
}

/* A rule that can reach itself without consuming a token - `A = A, b ;` and
 * friends - recurses forever. Recursive descent cannot parse left recursion at
 * all, so the only question is whether it reports that or blows the C stack.
 * The limit is far above any legitimate nesting depth. */
#define MAX_PARSE_DEPTH 1024

static SyntaxNode *parseNodeInner(const RuleNode *rnode, TokenStream *stream, MemPool *pool);

SyntaxNode *parseNode(const RuleNode *rnode, TokenStream *stream, MemPool *pool) {
	SyntaxNode *result;

	/* Once the limit is hit, abandon the whole parse rather than just this
	 * branch. Failing one branch would send alternation off to try the next
	 * one, which recurses just as deep - the search explodes and the arena
	 * grows until the process is killed. */
	if (stream->depth_exceeded) {
		return NULL;
	}
	if (stream->depth >= MAX_PARSE_DEPTH) {
		stream->depth_exceeded = true;
		return NULL;
	}
	stream->depth++;
	result = parseNodeInner(rnode, stream, pool);
	stream->depth--;
	return result;
}

static SyntaxNode *parseNodeInner(const RuleNode *rnode, TokenStream *stream, MemPool *pool) {
	switch (rnode->kind) {
		case RULE_GRM:
			switch (rnode->as.g) {
				case GRM_SEQ:    return parseSequence(rnode, stream, pool);
				case GRM_ALT:    return parseAlternation(rnode, stream, pool);
				case GRM_OPT:    return parseOptional(rnode, stream, pool);
				case GRM_REPEAT: return parseRepetition(rnode, stream, pool);
			}
			return NULL;
		case RULE_STX:
			return parseRuleRef(rnode, stream, pool);
		case RULE_TK:
			return parseTerminal(rnode, stream, pool);
	}
	return NULL;
}

SyntaxNode *parseTokenStream(const Grammar *grammar, TokenStream *stream, MemPool *pool) {
	const RuleNode *start = grammarRuleFor(grammar, grammar->start);
	SyntaxNode *root;

	if (!start) {
		fprintf(stderr, "parse error: grammar has no start rule\n");
		return NULL;
	}

	stream->pos = 0;
	stream->furthest = 0;
	stream->depth = 0;
	stream->depth_exceeded = false;
	root = parseNode(start, stream, pool);

	if (stream->depth_exceeded) {
		fprintf(stderr, "parse error: rule nesting exceeded %d levels near line %zu.\n"
		                "  This usually means a left-recursive rule (one that can reach\n"
		                "  itself without consuming a token). Recursive descent cannot\n"
		                "  parse left recursion; rewrite the rule using {} repetition.\n",
		        MAX_PARSE_DEPTH, stream->tk[stream->furthest].line);
		return NULL;
	}

	if (!root) {
		const Token *at = &stream->tk[stream->furthest];
		fprintf(stderr, "%zu: parse error: unexpected %s \"%.*s\"\n",
		        at->line, tokenName(grammar->reg, at->type),
		        (int)at->len, at->start);
		return NULL;
	}

	/* Trailing junk is a parse failure, not a success with leftovers. */
	if (stream->pos < stream->n && stream->tk[stream->pos].type != TK_EOF) {
		const Token *at = &stream->tk[stream->furthest];
		fprintf(stderr, "%zu: parse error: unexpected %s \"%.*s\"\n",
		        at->line, tokenName(grammar->reg, at->type),
		        (int)at->len, at->start);
		return NULL;
	}

	/* The start rule's own name is not applied by any reference, so apply it. */
	if (root->is_anonymous) {
		root->type = grammar->start;
		root->is_anonymous = false;
	}
	return root;
}

/* --- debug output -------------------------------------------------------- */

void fPrintSyntaxNode(Registry *reg, const SyntaxNode *node, unsigned indent, FILE *file) {
	for (unsigned i = 0; i < indent; i++) {
		fputc('\t', file);
	}

	if (node->is_token) {
		fprintf(file, "- %s \"%.*s\"\n",
		        tokenName(reg, node->token.type),
		        (int)node->token.len, node->token.start);
		return;
	}

	fprintf(file, "- /%s\n", syntaxName(reg, node->type));
	for (size_t i = 0; i < node->n_children; i++) {
		fPrintSyntaxNode(reg, node->children[i], indent + 1, file);
	}
}

void printSyntaxNode(Registry *reg, const SyntaxNode *node, unsigned indent) {
	for (unsigned i = 0; i < indent; i++) {
		putchar('\t');
	}

	if (node->is_token) {
		printf("- ");
		setColor(ANSI_GREEN);
		printf("%s ", tokenName(reg, node->token.type));
		resetColor();
		setColor(ANSI_YELLOW);
		printf("%.*s\n", (int)node->token.len, node->token.start);
		resetColor();
		return;
	}

	printf("- ");
	setColor(ANSI_CYAN);
	printf("/%s\n", syntaxName(reg, node->type));
	resetColor();

	for (size_t i = 0; i < node->n_children; i++) {
		printSyntaxNode(reg, node->children[i], indent + 1);
	}
}
