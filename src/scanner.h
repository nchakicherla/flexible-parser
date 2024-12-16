#ifndef SCANNER_H
#define SCANNER_H

#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

#include "token_types.h"

typedef struct s_Scanner {
	char *start;
	char *current;
	size_t line;
} Scanner;

typedef struct s_Token {
	TOKEN_TYPE type;
	char *start;
	size_t len;
	size_t line;
} Token;

void initScanner(char *source);
bool isAtEnd(void);
char advance(void);
char peek(void);
char peekNext(void);
bool match(char expect);
Token makeToken(TOKEN_TYPE type);
Token makeErrorToken(char *message);
void skipWhitespace(void);
Token string(void);
bool isDigit(char c);
Token number(void);
bool isAlpha(char c);
TOKEN_TYPE checkKeyword(size_t start, size_t len, char *rest, TOKEN_TYPE type);
TOKEN_TYPE identifierType(void);
Token identifier(void);
Token scanToken(void);
char *tokenLabelLookup(TOKEN_TYPE type);
size_t countTokens(char *source, bool *had_error);
void __printToken(Token *token);

#endif // SCANNER_H
