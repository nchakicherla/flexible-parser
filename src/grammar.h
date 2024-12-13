#ifndef GRAMMAR_H
#define GRAMMAR_H

#include "scanner.h"
#include "token_types.h"
#include "syntax_types.h"
#include "mempool.h"

#include <stdio.h>

typedef enum {
	GRM_AND,
	GRM_OR,
	GRM_GROUP,
	GRM_IFONE,
	GRM_IFMANY,
} GRAMMAR_TYPE;

typedef enum {
	RULE_GRM,
	RULE_STX,
	RULE_TK,
} RULE_NODE_TYPE;

union u_TargetType {
	GRAMMAR_TYPE g;
	SYNTAX_TYPE s;
	TOKEN_TYPE t;
};

typedef struct s_RuleNode {

	RULE_NODE_TYPE node_type;

	union u_TargetType nested_type;

	struct s_RuleNode *rule_head;

	size_t n_children;
	struct s_RuleNode *children;
	struct s_RuleNode *parent;
} RuleNode;

typedef struct s_GrammarRule {
	SYNTAX_TYPE stype;
	RuleNode *head;
} GrammarRule;

typedef struct s_GrammarRuleArray {
	size_t n_rules;
	GrammarRule *rules;
} GrammarRuleArray;

void initRuleNode(RuleNode *node);

size_t getSemicolonOffsetFromRuleStart(Token *tokens);
RULE_NODE_TYPE getPrevalentType(Token *tokens, size_t n);
GRAMMAR_TYPE getPrevalentGrammarType(Token *tokens, size_t n);
size_t getRuleStartIndex(SYNTAX_TYPE type, Token *tokens, size_t n);
size_t countChildren(GRAMMAR_TYPE type, Token *tokens, size_t n);
size_t getDelimOffset(GRAMMAR_TYPE type, Token *tokens);

int fillGrammarNode(RuleNode *node, Token *tokens, size_t n, MemPool *pool);
int populateCyclicalReferences(RuleNode *node, GrammarRuleArray *array);
//int populateCyclicalReferences(GrammarRuleArray *rule_array);
int tryInitGrammarRuleArray(GrammarRuleArray *rule_array, char *fileName, MemPool *pool);

void printGrammarNode(RuleNode *node, unsigned int indent);
void printGrammarRule(GrammarRule *rule);
void printGrammarRuleByIndex(GrammarRuleArray *array, int i);
void printGrammarRuleArray(GrammarRuleArray *array);
void fPrintGrammarNode(RuleNode *node, unsigned int indent, FILE *file);
void fPrintGrammarRule(GrammarRule *rule, FILE *file);
void fPrintGrammarRuleByIndex(GrammarRuleArray *array, int i, FILE *file);
void fPrintGrammarRuleArray(GrammarRuleArray *array, FILE *file);

#endif // GRAMMAR_H
