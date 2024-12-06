#ifndef PARSER_H
#define PARSER_H

#include "mempool.h"
#include "grammar.h"
#include "scanner.h"
#include "ast.h"
#include "syntax_node.h"

typedef struct s_Parser {
	MemPool p;
	GrammarRuleArray grammar;
	char *source;
	Token *tokens;
	TokenStream tk_stream;
	SyntaxNode *head;
} Parser;

void initParser(Parser *parser);

#endif // PARSER_H