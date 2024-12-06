#include "vm.h"
#include "file.h"
#include "ast.h"
#include "syntax_tree.h"

void initVM(VM *vm, char *grammarFile) {
	initSyntaxTree(&(vm->tree));
	initMemPool(&(vm->pool));
	initGrammarRuleArray(&(vm->rule_array), grammarFile, &(vm->pool));
	initTable(&(vm->table));
	//vm->name = pNewStr(script, &(vm->pool));
	//vm->source = pReadFile(script, &(vm->pool));
	vm->grammar_name = pNewStr(grammarFile, &(vm->pool));
	return;
}

void termVM(VM *vm) {
	termSyntaxTree(&(vm->tree));
	termTable(&(vm->table));
	termMemPool(&(vm->pool));
	return;
}

void printTokens(VM *vm) {

	for(size_t i = 0; i < vm->tree.n_tokens; i++) {
		printf("LINE: %6zu TK%6zu: TYPE: %16s - \"%.*s\"\n", 
			vm->tree.tokens[i].line,
			i,
			tokenLabelLookup(vm->tree.tokens[i].type), 
			(int)vm->tree.tokens[i].len, vm->tree.tokens[i].start);
	}
	return;
}
/*
void printSource(VM *vm) {
	printf("SOURCE:\n\"\n%s\n\"\n", vm->source);
	return;
}
*/
