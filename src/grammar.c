#include "common.h"
#include "grammar.h"
#include "syntax_types.h"
#include "syntax_labels.h"
#include "scanner.h"
#include "token_labels.h"
#include "mempool.h"
#include "file.h"

#include <stdio.h>

static SYNTAX_TYPE getSNodeTypeFromLiteral(char *str) {
	for (size_t i = 0; i < (sizeof(syntax_labels) / sizeof(char *)); i++) {
		if(0 == strcmp(str, syntax_labels[i])) {
			return i;
		}
	}
	return STX_ERROR;
}

static TOKEN_TYPE tokenTypeValFromNChars(char* str, size_t n) {
	for (size_t i = 0; i < (sizeof(__tokenNameLiterals) / sizeof(char *)); i++) {
		if(0 == strncmp(__tokenNameLiterals[i], str, n)) return i;
	}
	return TK_ERROR;
}

static void __printNTabs(unsigned int n) {
	for(unsigned int i = 0; i < n; i++) {
		putchar('\t');
	}
	return;
}

static void __fPrintNTabs(unsigned int n, FILE *file) {
	for(unsigned int i = 0; i < n; i++) {
		fputc('\t', file);
	}
	return;
}

static char *syntaxTypeLiteralLookup(SYNTAX_TYPE type) {
	return syntax_labels[type];
}

void initRuleNode(RuleNode *node) {
	node->rule_head = NULL;
	node->n_children = 0;
	node->children = NULL;
	node->parent = NULL;
}

size_t getSemicolonOffsetFromRuleStart(Token *tokens) {
	size_t ret = 0;
	while(tokens[ret].type != TK_SEMICOLON && tokens[ret].type != TK_EOF) {
		ret++;
	}
	return ret;
}

RULE_NODE_TYPE getPrevalentType(Token *tokens, size_t n) {
	if(n == 1) {
		switch(tokens[0].start[0]) {
			case 'S': {
				return RULE_STX;
			}
			case 'T': {
				return RULE_TK;
			}
		}
	}
	return RULE_GRM;
}

GRAMMAR_TYPE getPrevalentGrammarType(Token *tokens, size_t n) {
	size_t inGroup = 0;
	bool commaFound = false;
	bool pipeFound = false;

	for(size_t i = 0; i < n; i++) {
		switch(tokens[i].type) {
			case TK_LPAREN:
			case TK_LBRACE:
			case TK_LSQUARE:
				inGroup++;
				continue;
			case TK_RPAREN:
			case TK_RBRACE:
			case TK_RSQUARE:
				inGroup--;
				continue;
			default:
				break;
		}
		if(0 == inGroup && tokens[i].type == TK_COMMA) {
			commaFound = true;
		}
		if(0 == inGroup && tokens[i].type == TK_PIPE) {
			pipeFound = true;
		}
	}
	if(commaFound) return GRM_AND;
	if(pipeFound) return GRM_OR;

	if(tokens[0].type == TK_LPAREN) return GRM_GROUP;
	if(tokens[0].type == TK_LSQUARE) return GRM_IFONE;
	if(tokens[0].type == TK_LBRACE) return GRM_IFMANY;
	return GRM_AND; // and by default
}

size_t getRuleStartIndex(SYNTAX_TYPE type, Token *tokens, size_t n) {
	char *typeLiteral = syntaxTypeLiteralLookup(type);
	size_t len = strlen(typeLiteral);
	for(size_t i = 0; i < n; i++) {
		if(tokens[i].len == len) {
			if(0 == strncmp(typeLiteral, tokens[i].start, len) && tokens[i + 1].type == TK_EQUAL) {
				return i + 2;
			}
		}
	}
	return 1; // returns 0 on error because all rule starts should be at least 2 (STX_XYZ = ...)
}

size_t countChildren(GRAMMAR_TYPE type, Token *tokens, size_t n) { // should only be called on GRM_AND, GRM_OR
	size_t n_children = 1;
	size_t inGroup = 0;
	TOKEN_TYPE delim;
	switch(type) {
		case GRM_AND:
			delim = TK_COMMA;
			break;
		case GRM_OR:
			delim = TK_PIPE;
			break;
		case GRM_GROUP:
		case GRM_IFONE:
		case GRM_IFMANY:
		default:
			break;
	}

	for(size_t i = 0; i < n; i++) {
		switch(tokens[i].type) {
			case TK_LPAREN:
			case TK_LBRACE:
			case TK_LSQUARE:
				inGroup++;
				continue;
			case TK_RPAREN:
			case TK_RBRACE:
			case TK_RSQUARE:
				inGroup--;
				continue;
			default:
				break;
		}
		if(0 == inGroup && tokens[i].type == delim) {
			n_children++;
		}
	}

	return n_children;
}

size_t getDelimOffset(GRAMMAR_TYPE type, Token *tokens) {
	size_t inGroup = 0;
	TOKEN_TYPE delim;
	size_t ret = 0;
	switch(type) {
		case GRM_AND:
			delim = TK_COMMA;
			break;
		case GRM_OR:
			delim = TK_PIPE;
			break;
		case GRM_GROUP:
		case GRM_IFONE:
		case GRM_IFMANY:
		default:
			break;
	}
	while(true) {
		switch(tokens[ret].type) {
			case TK_LPAREN:
			case TK_LBRACE:
			case TK_LSQUARE:
				inGroup++;
				ret++;
				continue;
			case TK_RPAREN:
			case TK_RBRACE:
			case TK_RSQUARE:
				inGroup--;
				ret++;
				continue;
			default:
				break;
		}
		if(0 == inGroup && tokens[ret].type == delim) {
			return ret;
		}
		ret++;
	}
	return 0;
}

int fillGrammarNode(RuleNode *node, Token *tokens, size_t n, MemPool *pool) {
	initRuleNode(node);
	RULE_NODE_TYPE nodeType = getPrevalentType(tokens, n);
	node->node_type = nodeType;

	switch(nodeType) {
		case RULE_GRM: {
			GRAMMAR_TYPE grammarType = getPrevalentGrammarType(tokens, n);
			switch(grammarType) {
				case GRM_AND:
				case GRM_OR: {
					node->nested_type.g = grammarType;
					node->n_children = countChildren(grammarType, tokens, n);
					node->children = palloc(pool, node->n_children * sizeof(RuleNode));

					Token *start = tokens;
					size_t delimOffset = 0;
					for(size_t i = 0; i < node->n_children - 1; i++) {
						delimOffset = getDelimOffset(grammarType, start);
						fillGrammarNode(&(node->children[i]), start, delimOffset, pool);
						node->children[i].parent = node;
						start = start + delimOffset + 1;
					}
					fillGrammarNode(&(node->children[node->n_children - 1]), start, n - (size_t)(start - tokens), pool);
					break;
				}
				case GRM_GROUP:
				case GRM_IFONE:
				case GRM_IFMANY: {
					node->nested_type.g = grammarType;
					node->n_children = 1;
					node->children = palloc(pool, sizeof(RuleNode));
					node->children->parent = node;
					fillGrammarNode(node->children, tokens + 1, n - 2, pool);
					break;
				}
				default: 
					break;
			}
			break;
		}
		case RULE_STX: {
			char *tmp_str = palloc(pool, tokens[0].len + 1);
			for(size_t i = 0; i < tokens[0].len; i++) {
				tmp_str[i] = tokens[0].start[i];
			}
			tmp_str[tokens[0].len] = '\0';
			node->nested_type.s = getSNodeTypeFromLiteral(tmp_str);
			break;
		}
		case RULE_TK: {
			node->nested_type.t = tokenTypeValFromNChars(tokens[0].start, tokens[0].len);
			break;
		}
	}
	return 0;
}

int populateCyclicalReferences(RuleNode *node, GrammarRuleArray *rule_array) {
	switch(node->node_type) {
		case RULE_GRM: {
			switch(node->nested_type.g) {
				case GRM_AND:
				case GRM_OR:
				case GRM_GROUP:
				case GRM_IFONE:
				case GRM_IFMANY:
					for(size_t i = 0; i < node->n_children; i++) {
						populateCyclicalReferences(&(node->children[i]), rule_array);
					}
					break;
				default:
					break;
			}
		}
		case RULE_STX: {
			node->rule_head = rule_array->rules[node->nested_type.s].head;
			break;
		}
		case RULE_TK:
		default:
			break;
	}
	return 0;
}

int tryInitGrammarRuleArray(GrammarRuleArray *rule_array, char *fileName, MemPool *pool) {

	char *source = tryReadFile(fileName, pool);
	if(!source) {
		return 1;
	}
	initScanner(source);

	size_t n_tokens = 0;
	while(true) {
		Token token = scanToken();
		n_tokens++;
		if (token.type == TK_EOF) break;
	}

	Token *tokens = palloc(pool, (n_tokens * sizeof(Token)));

	initScanner(source);

	for(size_t i = 0; i < n_tokens; i++) {
		tokens[i] = scanToken();
	}

	rule_array->n_rules = (size_t) STX_ERROR;
	rule_array->rules = palloc(pool, rule_array->n_rules * sizeof(GrammarRule));

	for(size_t i = 0; i < rule_array->n_rules; i++) {
		rule_array->rules[i].stype = i;
		rule_array->rules[i].head = palloc(pool, sizeof(RuleNode));

		size_t ruleStart = getRuleStartIndex((SYNTAX_TYPE)i, tokens, n_tokens);
		if(ruleStart == 1) { // ruleStart should be at least 2 since it's always preceded by "STX_XYZ ="
			printf("valid rule start for i = %zu not found\n", i);
			return 2;
		}

		size_t semiOffset = getSemicolonOffsetFromRuleStart(&tokens[ruleStart]); // get rule length in Tokens
		if(tokens[ruleStart].line != tokens[ruleStart + semiOffset].line) { // cursory rule validation, rules must be one line and terminated with semicolon
			printf("rule start and semicolon not on same line...\n");
			return 3;
		}

		// check here if rule is formed correctly, return if not
		// 

		fillGrammarNode(rule_array->rules[i].head, &tokens[ruleStart], semiOffset, pool);
		rule_array->rules[i].head->parent = NULL;
	}
	for(size_t i = 0; i < rule_array->n_rules; i++) {
		populateCyclicalReferences(rule_array->rules[i].head, rule_array);
	}

	FILE *rulesLog = fopen("./debug/grammar_tree.log", "w");
	if(rulesLog) {
		fPrintGrammarRuleArray(rule_array, rulesLog);
		fclose(rulesLog);
	}
	return 0;
}

void printGrammarNode(RuleNode *node, unsigned int indent) {
	switch(node->node_type) {
		case RULE_GRM: {
			__printNTabs(indent);
			char *label = NULL;
			switch(node->nested_type.g) {
				case GRM_AND:
					label = "AND"; break;
				case GRM_OR:
					label = "OR"; break;
				case GRM_GROUP:
					label = "GROUP"; break;
				case GRM_IFONE:
					label = "IFONE"; break;
				case GRM_IFMANY:
					label = "IFMANY"; break;
			}
			printf("%s\n", label);
			for(size_t i = 0; i < node->n_children; i++) {
				printGrammarNode(&(node->children[i]), indent + 1);
			}
			break;
		}
		case RULE_STX: {
			__printNTabs(indent);
			printf("SYNTAX --- %s (%p)\n", 
				syntaxTypeLiteralLookup(node->nested_type.s),
				(void *)node->rule_head);
			break;
		}
		case RULE_TK: {
			__printNTabs(indent);
			printf("TOKEN --- %s\n", tokenLabelLookup(node->nested_type.t));
			break;
		}
	}
}

void printGrammarRule(GrammarRule *rule) {
	printf("RULE: %s\n", syntaxTypeLiteralLookup(rule->stype));
	printf("-\n");
	printGrammarNode(rule->head, 1);
	printf("\n----\n----\n\n");
}

void printGrammarRuleByIndex(GrammarRuleArray *array, int i) {
	printGrammarRule(&(array->rules[i]));
}

void printGrammarRuleArray(GrammarRuleArray *array) {
	for(size_t i = 0; i < array->n_rules; i++) {
		printGrammarRule(&(array->rules[i]));
	}
}

void fPrintGrammarNode(RuleNode *node, unsigned int indent, FILE *file) {
	switch(node->node_type) {
		case RULE_GRM: {
			__fPrintNTabs(indent, file);
			char *label = NULL;
			switch(node->nested_type.g) {
				case GRM_AND:
					label = "AND"; break;
				case GRM_OR:
					label = "OR"; break;
				case GRM_GROUP:
					label = "GROUP"; break;
				case GRM_IFONE:
					label = "IFONE"; break;
				case GRM_IFMANY:
					label = "IFMANY"; break;
			}
			fprintf(file, "%s\n", label);
			for(size_t i = 0; i < node->n_children; i++) {
				fPrintGrammarNode(&(node->children[i]), indent + 1, file);
			}
			break;
		}
		case RULE_STX: {
			__fPrintNTabs(indent, file);
			fprintf(file, "SYNTAX --- %s\n", syntaxTypeLiteralLookup(node->nested_type.s));
			break;
		}
		case RULE_TK: {
			__fPrintNTabs(indent, file);
			fprintf(file, "TOKEN --- %s\n", tokenLabelLookup(node->nested_type.t));
			break;
		}
	}
}

void fPrintGrammarRule(GrammarRule *rule, FILE *file) {
	fprintf(file, "RULE: %s\n", syntaxTypeLiteralLookup(rule->stype));
	fprintf(file, "-\n");
	fPrintGrammarNode(rule->head, 1, file);
	fprintf(file, "\n----\n----\n\n");

}

void fPrintGrammarRuleByIndex(GrammarRuleArray *array, int i, FILE *file) {
	fPrintGrammarRule(&(array->rules[i]), file);
}

void fPrintGrammarRuleArray(GrammarRuleArray *array, FILE *file) {
	for(size_t i = 0; i < array->n_rules; i++) {
		fPrintGrammarRule(&(array->rules[i]), file);
	}
}
