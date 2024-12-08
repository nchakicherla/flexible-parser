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

	Parser parser = { 0 };
	initParser(&parser);

	char *source = tryReadFile(test_script, &scratch);
	if(!source) {
		goto error;
	}
	if(0 != trySetGrammar(&parser, grammar_file)) {
		goto error;
	}
	if(0 != tryScanTokens(&parser, source)) {
		goto error;
	}
	termMemPool(&scratch);
	termParser(&parser);
	return 0;

error:
	printf("error!\n");
	termMemPool(&scratch);
	termParser(&parser);
	return 1;
}
