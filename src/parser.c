#include "parser.h"

void initParser(Parser *parser) {
	parser->n_tokens = 0;
	parser->tokens = NULL;
	parser->head = NULL;
	initMemPool(&parser->p);
	parser->is_initialized = true;
	parser->is_grammar_set = false;
}

void termParser(Parser *parser) {
	termMemPool(&parser->p);
}

int trySetParserGrammar(Parser *parser, char *grammar_file) {

	if(!parser->is_initialized) {
		return 1;
	}

	int ret = tryInitGrammarRuleArray(&parser->grammar, grammar_file, &parser->p); // rule array itself is in parser's arena
	if(ret != 0) {
		return 2;
	}

	parser->is_grammar_set = true;
	return 0;
}

int tryScanParserTokens(Parser *parser, char *source) {

	if(!parser->is_initialized) {
		return 1;
	}
	/*
	if(!parser->is_grammar_set) {
		return 2;
	}
	*/
	if(!source) {
		return 3;
	}

	bool scanner_error = false;
	size_t n_tokens = countTokens(source, &scanner_error);
	if(scanner_error) {
		printf("encountered error token, returning with error code...\n");
		return 4;
	}

	parser->n_tokens = n_tokens;
	parser->tokens = palloc(&parser->p, (n_tokens * sizeof(Token)));
	initScanner(source);

	for(size_t i = 0; i < n_tokens; i++) {
		parser->tokens[i] = scanToken();
	}

	parser->tk_stream.tk = parser->tokens;
	parser->tk_stream.pos = 0;
	parser->tk_stream.n = parser->n_tokens;
	
	return 0;
}
