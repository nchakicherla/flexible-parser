#ifndef PARSER_H
#define PARSER_H

#include "mempool.h"
#include "grammar.h"
#include "scanner.h"
#include "ast.h"
#include "syntax_node.h"

typedef struct s_Parser {
	GrammarRuleArray grammar;
	//char *source;
	Token *tokens;
	TokenStream tk_stream;
	SyntaxNode *head;
	MemPool p;
} Parser;

void initParser(Parser *parser);

void termParser(Parser *parser);

int trySetGrammar(Parser *parser, char *grammar_file);

//int tryReadSource(Parser *parser, char *source);

int tryScanTokens(Parser *parser, char *source);

#endif // PARSER_H
