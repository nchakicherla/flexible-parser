#ifndef PARSER_H
#define PARSER_H

#include "mempool.h"
#include "grammar.h"
#include "scanner.h"
#include "ast.h"
#include "ast_node.h"

typedef struct s_Parser {
	GrammarRuleArray grammar;
	size_t n_tokens;
	Token *tokens;
	TokenStream tk_stream;
	SyntaxNode *head;
	MemPool p;
	bool is_initialized;
	bool is_grammar_set;
} Parser;

void initParser(Parser *parser);
void termParser(Parser *parser);
int trySetParserGrammar(Parser *parser, char *grammar_file);
int tryScanParserTokens(Parser *parser, char *source);

#endif // PARSER_H
