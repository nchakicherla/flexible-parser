#include "parser.h"

void initParser(Parser *parser) {
	//parser->source = NULL;
	parser->tokens = NULL;
	parser->head = NULL;
	initMemPool(&parser->p);
}

void termParser(Parser *parser) {
	termMemPool(&parser->p);
}

int trySetGrammar(Parser *parser, char *grammar_file) {
	MemPool scratch;
	initMemPool(&scratch);

	char *file_contents = pReadFile(grammar_file, &scratch);
	if(!file_contents) {
		return 1;
	}

	int ret = initGrammarRuleArray(&parser->grammar, grammar_file, &parser->p);
	if(ret != 0) {
		return 2;
	}

	termMemPool(&scratch);
	return 0;
}
/*
int tryReadSource(Parser *parser, char *source) {
	parser->source = pReadFile(source, &parser->p);
	if(!parser->source) {
		return 1;
	}
	return 0;
}
*/
int tryScanTokens(Parser *parser, char *source) {

	bool scanner_error = false;
	size_t n_tokens = countTokens(source, &scanner_error);
	if(!scanner_error) {
		printf("n_tokens: %zu\n", n_tokens);
	} else {
		printf("had scanner error\n");
	}

	parser->tokens = palloc(&parser->p, (n_tokens * sizeof(Token)));
	initScanner(source);

	for(size_t i = 0; i < n_tokens; i++) {
		parser->tokens[i] = scanToken();
	}

	return 0;
}
