#include <stdio.h>
#include <string.h>

#include "common.h"
#include "file.h"
#include "interp.h"
#include "mempool.h"
#include "parser.h"

static const char *DEFAULT_GRAMMAR = "./resources/grammar.txt";
static const char *DEFAULT_SOURCE  = "./resources/script.tl";
static const char *GRAMMAR_LOG     = "./debug/grammar_tree.log";

static void usage(void) {
	printf("usage: main.run [options]\n");
	printf("  -g <file>   grammar file      (default %s)\n", DEFAULT_GRAMMAR);
	printf("  -s <file>   source file       (default %s)\n", DEFAULT_SOURCE);
	printf("  --ast       print the syntax tree\n");
	printf("  --tokens    print the token stream\n");
	printf("  --dump-grammar <file>  write the compiled rule trees to a file\n");
	printf("  --parse-only           skip execution\n");
}

int main(int argc, char **argv) {
	const char *grammar_file = DEFAULT_GRAMMAR;
	const char *source_file = DEFAULT_SOURCE;
	const char *grammar_log = GRAMMAR_LOG;
	bool show_ast = false;
	bool show_tokens = false;
	bool parse_only = false;

	MemPool scratch;
	Parser parser;
	ParseStatus status;
	char *source;
	int exit_code = 0;

	for (int i = 1; i < argc; i++) {
		if (0 == strcmp(argv[i], "-g") && i + 1 < argc) {
			grammar_file = argv[++i];
		} else if (0 == strcmp(argv[i], "-s") && i + 1 < argc) {
			source_file = argv[++i];
		} else if (0 == strcmp(argv[i], "--dump-grammar") && i + 1 < argc) {
			grammar_log = argv[++i];
		} else if (0 == strcmp(argv[i], "--ast")) {
			show_ast = true;
		} else if (0 == strcmp(argv[i], "--tokens")) {
			show_tokens = true;
		} else if (0 == strcmp(argv[i], "--parse-only")) {
			parse_only = true;
		} else if (0 == strcmp(argv[i], "-h") || 0 == strcmp(argv[i], "--help")) {
			usage();
			return 0;
		} else {
			fprintf(stderr, "unrecognized argument: %s\n", argv[i]);
			usage();
			return 1;
		}
	}

	initMemPool(&scratch);
	initParser(&parser);

	source = tryReadFile(source_file, &scratch);
	if (!source) {
		fprintf(stderr, "could not read source file '%s'\n", source_file);
		exit_code = 1;
		goto done;
	}

	status = parserSetGrammar(&parser, grammar_file);
	if (status != PARSE_OK) {
		fprintf(stderr, "%s\n", parseStatusMessage(status));
		exit_code = 1;
		goto done;
	}

	if (grammar_log) {
		FILE *log = fopen(grammar_log, "w");
		if (log) {
			fPrintGrammar(&parser.grammar, log);
			fclose(log);
		}
	}

	status = parserParseSource(&parser, source);
	if (status != PARSE_OK) {
		fprintf(stderr, "%s\n", parseStatusMessage(status));
		exit_code = 1;
		goto done;
	}

	if (show_tokens) {
		for (size_t i = 0; i < parser.n_tokens; i++) {
			printToken(&parser.reg, &parser.tokens[i]);
		}
	}

	if (show_ast) {
		printSyntaxNode(&parser.reg, parser.ast, 0);
	}

	if (parse_only) {
		printf("parsed %s against %s: ok (%zu tokens)\n",
		       source_file, grammar_file, parser.n_tokens);
		goto done;
	}

	{
		int program_exit = 0;
		if (0 != runProgram(parser.ast, &parser.reg, &parser.pool, &program_exit)) {
			exit_code = 1;
			goto done;
		}
		exit_code = program_exit;
	}

done:
	termMemPool(&scratch);
	termParser(&parser);
	return exit_code;
}
