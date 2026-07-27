#include "repl.h"

#include "ast.h"
#include "color.h"
#include "file.h"
#include "interp.h"
#include "linenoise.h"

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define HISTORY_FILE  ".flexible_parser_history"
#define HISTORY_MAX   1000

typedef struct s_Repl {
	Parser *parser;
	Interp *interp;
	const char *grammar_file;
	bool show_ast;
	bool show_tokens;
	int exit_code;
	bool quitting;
} Repl;

/* --- input buffer --------------------------------------------------------- */

typedef struct s_StrBuf {
	char *data;
	size_t len;
	size_t cap;
} StrBuf;

static void bufInit(StrBuf *b) {
	b->data = NULL;
	b->len = 0;
	b->cap = 0;
}

static void bufFree(StrBuf *b) {
	free(b->data);
	bufInit(b);
}

static void bufAppend(StrBuf *b, const char *s) {
	size_t add = strlen(s);
	if (b->len + add + 1 > b->cap) {
		size_t cap = (b->cap == 0) ? 256 : b->cap;
		while (b->len + add + 1 > cap) {
			cap *= 2;
		}
		b->data = realloc(b->data, cap);
		if (!b->data) {
			fprintf(stderr, "out of memory\n");
			exit(1);
		}
		b->cap = cap;
	}
	memcpy(b->data + b->len, s, add + 1);
	b->len += add;
}

/* --- completeness --------------------------------------------------------- */

/* Uses the real scanner rather than counting characters, so brackets inside
 * strings and comments do not confuse it. A scan failure counts as complete so
 * the error is reported now instead of trapping the user in a continuation
 * prompt they cannot escape except with Ctrl-C. */
static bool inputIsComplete(Repl *repl, const char *text) {
	MemPool scratch;
	Token *tokens;
	size_t n = 0;
	Token bad;
	long depth = 0;
	bool complete = true;

	initMemPool(&scratch);
	tokens = tokenizeAll(text, &repl->parser->reg, &scratch, &n, &bad);

	if (tokens) {
		for (size_t i = 0; i < n; i++) {
			switch (tokens[i].type) {
				case TK_LPAREN:
				case TK_LBRACE:
				case TK_LSQUARE:
					depth++;
					break;
				case TK_RPAREN:
				case TK_RBRACE:
				case TK_RSQUARE:
					depth--;
					break;
				default:
					break;
			}
		}
		complete = (depth <= 0);
	}

	termMemPool(&scratch);
	return complete;
}

/* --- parsing an entry -----------------------------------------------------
 *
 * A REPL entry is rarely a whole program, so several rules are tried and the
 * first that consumes the entire line wins. The built-in statement rules come
 * first in a deliberate order, then the grammar's start rule, then anything
 * else the grammar defines - which is what lets the REPL work against a custom
 * grammar whose rule names this file has never heard of. */
static const SYNTAX_TYPE REPL_CANDIDATES[] = {
	STX_INIT, STX_DECLARE, STX_ASSIGN,
	STX_FNDEF, STX_CLASS,
	STX_IF, STX_WHILE, STX_FOR, STX_FORRANGE, STX_SWITCH,
	STX_RETURN, STX_BREAK, STX_EXIT,
	STX_ECHO, STX_SCOPE,
	STX_EXPR,
};

/* Parses the next item at the stream's current position, or NULL if nothing
 * matches. An entry may hold several statements - a Shift+Enter block usually
 * does - so this is called in a loop rather than once per entry. */
static SyntaxNode *parseNext(Repl *repl, TokenStream *stream) {
	Parser *p = repl->parser;
	SyntaxNode *node;

	for (size_t i = 0; i < sizeof(REPL_CANDIDATES) / sizeof(SYNTAX_TYPE); i++) {
		node = parseRuleAt(&p->grammar, REPL_CANDIDATES[i], stream, &p->pool);
		if (node) {
			return node;
		}
	}

	node = parseRuleAt(&p->grammar, p->grammar.start, stream, &p->pool);
	if (node) {
		return node;
	}

	/* Last resort: any rule the grammar defines. This is what lets the repl
	 * work against a grammar whose rule names this file has never heard of. */
	for (SYNTAX_TYPE t = 0; (size_t)t < p->grammar.n_rules; t++) {
		if (!p->grammar.rules[t].head) {
			continue;
		}
		node = parseRuleAt(&p->grammar, t, stream, &p->pool);
		if (node) {
			return node;
		}
	}
	return NULL;
}

/* --- commands ------------------------------------------------------------- */

static void printHelp(void) {
	printf("  :help            this message\n");
	printf("  :quit  :q        leave the repl (also Ctrl-D)\n");
	printf("  :ast             toggle printing the syntax tree for each entry\n");
	printf("  :tokens          toggle printing the token stream for each entry\n");
	printf("  :rules           list the rules the loaded grammar defines\n");
	printf("  :load <file>     read and run a source file in this session\n");
	printf("  :reset           discard all variables and functions\n");
	printf("\n");
	printf("  Blocks continue automatically while brackets are unbalanced.\n");
	printf("  Shift+Enter forces a continuation on terminals that support it.\n");
	printf("  Ctrl-C abandons the entry being typed.\n");
}

static void listRules(Repl *repl) {
	Grammar *g = &repl->parser->grammar;
	size_t shown = 0;

	for (size_t i = 0; i < g->n_rules; i++) {
		if (!g->rules[i].head) {
			continue;
		}
		printf("%-22s%s", syntaxName(g->reg, g->rules[i].stype),
		       (g->rules[i].stype == g->start) ? "  (start)" : "");
		shown++;
		fputs((shown % 3 == 0) ? "\n" : "  ", stdout);
	}
	if (shown % 3 != 0) {
		printf("\n");
	}
}

static void runEntry(Repl *repl, const char *text);

static void loadFile(Repl *repl, const char *path) {
	char *source = tryReadFile(path, &repl->parser->pool);
	if (!source) {
		fprintf(stderr, "could not read '%s'\n", path);
		return;
	}
	runEntry(repl, source);
}

/* Returns true if the line was a command and has been handled. */
static bool handleCommand(Repl *repl, const char *line) {
	if (line[0] != ':') {
		return false;
	}

	if (0 == strcmp(line, ":help") || 0 == strcmp(line, ":h")) {
		printHelp();
	} else if (0 == strcmp(line, ":quit") || 0 == strcmp(line, ":q")) {
		repl->quitting = true;
	} else if (0 == strcmp(line, ":ast")) {
		repl->show_ast = !repl->show_ast;
		printf("ast printing %s\n", repl->show_ast ? "on" : "off");
	} else if (0 == strcmp(line, ":tokens")) {
		repl->show_tokens = !repl->show_tokens;
		printf("token printing %s\n", repl->show_tokens ? "on" : "off");
	} else if (0 == strcmp(line, ":rules")) {
		listRules(repl);
	} else if (0 == strcmp(line, ":reset")) {
		repl->interp = interpCreate(&repl->parser->reg, &repl->parser->pool);
		printf("session state cleared\n");
	} else if (0 == strncmp(line, ":load", 5)) {
		const char *arg = line + 5;
		while (*arg == ' ') {
			arg++;
		}
		if (*arg == '\0') {
			fprintf(stderr, "usage: :load <file>\n");
		} else {
			loadFile(repl, arg);
		}
	} else {
		fprintf(stderr, "unknown command '%s', try :help\n", line);
	}
	return true;
}

/* --- evaluation ----------------------------------------------------------- */

static void runEntry(Repl *repl, const char *text) {
	Parser *p = repl->parser;
	TokenStream stream;
	Token *tokens;
	size_t n_tokens = 0;
	Token bad;

	tokens = tokenizeAll(text, &p->reg, &p->pool, &n_tokens, &bad);
	if (!tokens) {
		fprintf(stderr, "scan error: %.*s\n", (int)bad.len, bad.start);
		return;
	}

	if (repl->show_tokens) {
		for (size_t i = 0; i < n_tokens; i++) {
			printToken(&p->reg, &tokens[i]);
		}
	}

	stream.tk = tokens;
	stream.n = n_tokens;
	stream.pos = 0;
	stream.furthest = 0;
	stream.depth = 0;
	stream.depth_exceeded = false;

	/* Statements are parsed and run one at a time, so an earlier declaration in
	 * the same block is in scope for a later one. */
	while (stream.pos < stream.n && stream.tk[stream.pos].type != TK_EOF) {
		SyntaxNode *node = parseNext(repl, &stream);
		int entry_exit = 0;
		ExecResult result;

		if (!node) {
			const Token *at = &stream.tk[stream.furthest];
			fprintf(stderr, "parse error: unexpected %s \"%.*s\"\n",
			        tokenName(&p->reg, at->type), (int)at->len, at->start);
			return;
		}

		if (repl->show_ast) {
			printSyntaxNode(&p->reg, node, 0);
		}

		/* Deliberately not wrapped in a scope: bindings made at the prompt have
		 * to outlive the entry that created them. */
		result = interpExecEcho(repl->interp, node, &entry_exit);
		if (result == EXEC_EXITED) {
			repl->exit_code = entry_exit;
			repl->quitting = true;
			return;
		}
		if (result == EXEC_ERROR) {
			return; /* abandon the rest of the block */
		}
	}
}

/* --- the loop ------------------------------------------------------------- */

static char *historyPath(char *buf, size_t size) {
	const char *home = getenv("HOME");
	if (!home) {
		return NULL;
	}
	if ((size_t)snprintf(buf, size, "%s/%s", home, HISTORY_FILE) >= size) {
		return NULL;
	}
	return buf;
}

static void banner(Repl *repl) {
	setColor(ANSI_CYAN);
	printf("flexible-parser repl");
	resetColor();
	printf("  grammar: %s\n", repl->grammar_file);
	printf("type :help for commands, Ctrl-D to exit\n\n");
}

int runRepl(Parser *parser, const char *grammar_file) {
	Repl repl;
	StrBuf block;
	char hist[1024];
	bool have_history;

	repl.parser = parser;
	repl.interp = interpCreate(&parser->reg, &parser->pool);
	repl.grammar_file = grammar_file;
	repl.show_ast = false;
	repl.show_tokens = false;
	repl.exit_code = 0;
	repl.quitting = false;

	have_history = (historyPath(hist, sizeof(hist)) != NULL);
	linenoiseHistorySetMaxLen(HISTORY_MAX);
	if (have_history) {
		linenoiseHistoryLoad(hist);
	}
	linenoiseEnableExtendedKeys(1);

	banner(&repl);
	bufInit(&block);

	while (!repl.quitting) {
		char *line = linenoise(block.len == 0 ? ">>> " : "... ");
		bool forced;

		if (!line) {
			if (errno == EAGAIN) { /* Ctrl-C: throw away the partial entry */
				bufFree(&block);
				bufInit(&block);
				printf("\n");
				continue;
			}
			printf("\n"); /* Ctrl-D */
			break;
		}

		forced = linenoiseLineWasContinued() != 0;

		/* A trailing backslash is the terminal-independent way to force a
		 * continuation, for terminals that cannot report Shift+Enter. */
		{
			size_t n = strlen(line);
			if (n > 0 && line[n - 1] == '\\') {
				line[n - 1] = '\0';
				forced = true;
			}
		}

		if (block.len > 0) {
			bufAppend(&block, "\n");
		}
		bufAppend(&block, line);
		linenoiseFree(line);

		if (forced || !inputIsComplete(&repl, block.data ? block.data : "")) {
			continue;
		}

		if (block.len == 0 || block.data[0] == '\0') {
			bufFree(&block);
			bufInit(&block);
			continue;
		}

		linenoiseHistoryAdd(block.data);
		if (have_history) {
			linenoiseHistorySave(hist);
		}

		if (!handleCommand(&repl, block.data)) {
			/* Bindings hold pointers into the token text, which points into
			 * this string, so the entry has to outlive the loop iteration. */
			runEntry(&repl, pNewStr(block.data, &parser->pool));
		}

		bufFree(&block);
		bufInit(&block);
	}

	bufFree(&block);
	linenoiseEnableExtendedKeys(0);
	return repl.exit_code;
}
