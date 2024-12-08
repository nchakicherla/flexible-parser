#include <stdio.h>

#include "common.h"

//#include "scanner.h"
#include "file.h"
#include "table.h"
#include "mempool.h"
#include "vm.h"
#include "ast.h"

#include "random.h"
#include "color.h"

#include "parser.h"

#include <time.h>

#define INPUT_BUFFER_SIZE 512

int main(void) {

	char *grammar_file = "./resources/grammar.txt";
	char *test_script = "./resources/script.tl";

	MemPool scratch;
	initMemPool(&scratch); // scratch arena for source file contents

	Parser parser = {0};
	initParser(&parser);

	char *source = tryReadFile(test_script, &scratch);
	if(!source) {
		printf("couldn't read file...\n");
		goto error;
	}
	if(0 != trySetGrammar(&parser, grammar_file)) {
		printf("couldn't set grammar...\n");
		goto error;
	}
	if(0 != tryScanTokens(&parser, source)) {
		printf("couldn't tokenize...\n");
		goto error;
	}

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
