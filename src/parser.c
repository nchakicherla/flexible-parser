#include "parser.h"

void initParser(Parser *parser) {
	//parser->source = NULL;
	parser->n_tokens = 0;
	parser->tokens = NULL;
	parser->head = NULL;
	initMemPool(&parser->p);
	parser->is_initialized = true;
}

void termParser(Parser *parser) {
	termMemPool(&parser->p);
}

int trySetGrammar(Parser *parser, char *grammar_file) {

	if(!parser->is_initialized) {
		return 1;
	}

	MemPool scratch;
	initMemPool(&scratch); // temp arena for grammar_file contents

	char *file_contents = tryReadFile(grammar_file, &scratch);
	if(!file_contents) {
		return 2;
	}

	int ret = tryInitGrammarRuleArray(&parser->grammar, grammar_file, &parser->p); // rule array itself is in parser's arena
	if(ret != 0) {
		termMemPool(&scratch);
		return 3;
	}

	termMemPool(&scratch);
	return 0;
}
/*
int tryReadSource(Parser *parser, char *source) {
	parser->source = tryReadFile(source, &parser->p);
	if(!parser->source) {
		return 1;
	}
	return 0;
}
*/
int tryScanTokens(Parser *parser, char *source) {

	if(!parser->is_initialized) {
		return 1;
	}
	if(!source) {
		return 2;
	}

	bool scanner_error = false;
	size_t n_tokens = countTokens(source, &scanner_error);
	if(!scanner_error) {
		printf("n_tokens: %zu\n", n_tokens);
	} else {
		printf("had scanner error, returning with error code...\n");
		return 3;
	}

	parser->n_tokens = n_tokens;
	parser->tokens = palloc(&parser->p, (n_tokens * sizeof(Token)));
	initScanner(source);

	for(size_t i = 0; i < n_tokens; i++) {
		parser->tokens[i] = scanToken();
	}

	return 0;
}
