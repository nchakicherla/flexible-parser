#include "parser.h"

void initParser(Parser *parser) {
	parser->n_tokens = 0;
	parser->tokens = NULL;
	parser->head = NULL;
	initMemPool(&parser->p);
	parser->is_initialized = true;
	parser->grammar_set = false;
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
	parser->grammar_set = true;
	return 0;
}

int tryScanTokens(Parser *parser, char *source) {

	if(!parser->is_initialized) {
		return 1;
	}
	if(!parser->grammar_set) {
		return 2;
	}
	if(!source) {
		return 3;
	}

	bool scanner_error = false;
	size_t n_tokens = countTokens(source, &scanner_error);
	if(!scanner_error) {
		printf("n_tokens: %zu\n", n_tokens);
	} else {
		printf("encountered error token, returning with error code...\n");
		return 4;
	}

	parser->n_tokens = n_tokens;
	parser->tokens = palloc(&parser->p, (n_tokens * sizeof(Token)));
	initScanner(source);

	for(size_t i = 0; i < n_tokens; i++) {
		parser->tokens[i] = scanToken();
	}

	return 0;
}
