#ifndef PARSER_H
#define PARSER_H

#include "mempool.h"
#include "grammar.h"
#include "scanner.h"
#include "ast.h"
#include "ast_node.h"

typedef struct s_Parser {
	GrammarRuleArray grammar;
	//char *source;
	size_t n_tokens;
	Token *tokens;
	TokenStream tk_stream;
	SyntaxNode *head;
	MemPool p;
	bool is_initialized;
} Parser;

void initParser(Parser *parser);
void termParser(Parser *parser);
int trySetGrammar(Parser *parser, char *grammar_file);
int tryScanTokens(Parser *parser, char *source);

#endif // PARSER_H
