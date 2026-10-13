#include <stdio.h>

#include "common.h"

#include "mempool.h"
#include "file.h"

#include "parser.h"

#define INPUT_BUFFER_SIZE 512

int main(void) {

	char *grammar_file = "./resources/grammar.txt";
	char *source_file = "./resources/script.tl";

	MemPool scratch;
	initMemPool(&scratch); // scratch arena for source file contents

	Parser parser = {0}; // initialized to zero for is_initialized flag
	initParser(&parser);

	char *source = tryReadFile(source_file, &scratch);
	if(!source) {
		printf("couldn't read file...\n");
		goto error;
	}

	if(0 != trySetParserGrammar(&parser, grammar_file)) {
		printf("couldn't set grammar...\n");
		goto error;
	}

	if(0 != tryScanParserTokens(&parser, source)) {
		printf("couldn't tokenize...\n");
		goto error;
	}

	printf("successfully tokenized source...\n");

	// try parsing based on a rule in GrammarRuleArray in Parser, or brute-force and check all rules
	// parsing fails by returning NULL from parseGrammar in ast.c

	termMemPool(&scratch);
	termParser(&parser);
	return 0;

error:
	printf("error!\n");
	termMemPool(&scratch);
	termParser(&parser);
	return 1;
}
