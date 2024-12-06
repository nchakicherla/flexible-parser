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
	Parser parser;
	

	return 0;
}

int main3(void) {

	char *grammar_definition = "./resources/grammar.txt";
	char *script = "./resources/script.tl";

	MemPool scratch;
	initMemPool(&scratch);

	char *source = pReadFile(script, &scratch);
	printf("source: \n\n %s\n", source);

	GrammarRuleArray grammar;
	initGrammarRuleArray(&grammar, grammar_definition, &scratch);

	SyntaxTree tree; // syntax tree has its own arena
	initSyntaxTree(&tree);

	scanTokensFromSourceNoError(&tree, source);

	TokenStream stream;
	initTokenStream(&stream, &tree);

	termSyntaxTree(&tree);
	termMemPool(&scratch);
	return 0;
}

int main2(void) {

	char *grammar = "./resources/grammar.txt";

	VM vm;
	initVM(&vm, grammar);

	while(true) {
		//char *input_buffer = pzalloc(&vm.tree.pool, INPUT_BUFFER_SIZE);
		vm.tree.source = pzalloc(&vm.tree.pool, INPUT_BUFFER_SIZE);

		setColor(ANSI_GREEN);
		puts("\n");
		while(0 == strchr(vm.tree.source, '\n')) {
			fgets(vm.tree.source, INPUT_BUFFER_SIZE, stdin);
		}
		vm.tree.source[INPUT_BUFFER_SIZE - 1] = '\0';
		resetColor();

		scanTokensFromSource(&vm.tree, vm.tree.source);

		bool had_error = false;
		for(size_t i = 0; i < vm.tree.n_tokens; i++) {
			if(vm.tree.tokens[i].type == TK_ERROR) {
				setColor(ANSI_RED);
				printf("%.*s\n", (int)vm.tree.tokens[i].len, vm.tree.tokens[i].start);
				resetColor();
				had_error = true;
				break;
			}
		}

		if(had_error) {
			resetSyntaxTree(&vm.tree);
			continue;
		}

		TokenStream stream;

		for(size_t i = 0; i < vm.rule_array.n_rules; i++) {
			initTokenStream(&stream, &vm.tree);
			vm.tree.head = parseGrammar(vm.rule_array.rules[i].head, &stream, &vm.tree.pool);
			if(vm.tree.head) {
				if(stream.tk[stream.pos].type != TK_EOF) {
					//break; // ignore match if didn't consume all tokens in line
					continue;
				}
				if(vm.rule_array.rules[i].head->node_type == RULE_TK) {
					vm.tree.head = wrapNode(vm.tree.head, (SYNTAX_TYPE) i, &vm.tree.pool);
				} else {
					vm.tree.head->type = (SYNTAX_TYPE) i;
				}
				defineParentPtrs(vm.tree.head);
				printSyntaxNode(vm.tree.head, 0);
				break;
			}
		}

		if(0 == strncmp(vm.tree.source, ".exit", 5)) {
			break;
		}

		//printTokens(&vm);
		printPoolInfo(&vm.tree.pool);

		resetSyntaxTree(&vm.tree);
	}
	printPoolInfo(&vm.pool);

	termVM(&vm);
	return 0;
}
