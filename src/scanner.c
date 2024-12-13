#include "scanner.h"
#include "token_types.h"
#include "token_labels.h"

#include <stdio.h>

Scanner scanner;

void initScanner(char *source) {
	scanner.start = source;
	scanner.current = source;
	scanner.line = 1;
}

bool isAtEnd(void) {
	return *scanner.current == '\0';
}

char advance(void) {
	scanner.current++;
	return scanner.current[-1];
}

char peek(void) {
	return *scanner.current;
}

char peekNext(void) {
	if (isAtEnd()) {
		return '\0';
	}
	return scanner.current[1];
}

char peekNextNext(void) {
	if (isAtEnd()) {
		return '\0';
	}
	if(scanner.current[1] == '\0') {
		return '\0';
	}
	return scanner.current[2];
}

bool match(char expect) {
	if (isAtEnd())  {
		return false;
	}
	if (*scanner.current != expect)  {
		return false;
	}
	
	scanner.current++;
	return true;
}

Token makeToken(TOKEN_TYPE type) {
	Token token;
	token.type = type;
	token.start = scanner.start;
	token.len = (size_t)(scanner.current - scanner.start);
	token.line = scanner.line;
	return token;
}

Token makeErrorToken(char *message) {
	Token token;
	token.type = TK_ERROR;
	token.start = message;
	token.len = strlen(message);
	token.line = scanner.line;
	return token;
}

void skipWhitespace(void) {
	while (true) {
		char c = peek();
		switch(c) {
			case ' ':
			case '\r':
			case '\t':
				advance();
				break;
			case '\n':
				scanner.line++;
				advance();
				break;
			case '/':
				if (peekNext() == '/') {
					while (peek() != '\n' && !isAtEnd()) {
						advance();
					}
				} 
				else if (peekNext() == '*') {
					advance();
					advance();
					while (peek() != '*' && !isAtEnd()) {
						if(peek() == '\n') {
							scanner.line++;
						}
						advance();
						if(peek() == '*') {
							if(peekNext() == '/') {
								advance();
								advance();
								break;
							}
							advance();
						}
					}
				} else {
					return;
				}
				break;
			default:
				return;
		}
	}
}

Token string(void) {
	while (peek() != '"' && !isAtEnd()) {
		if (peek() == '\n') { // strings can be multi-line
			// scanner.line++;
			return makeErrorToken("unterminated string");
		}
		advance();
	}

	if (isAtEnd()) {
		return makeErrorToken("unterminated string");
	}

	advance();
	return makeToken(TK_CHARS);
}

bool isDigit(char c) {
	return c >= '0' && c <= '9';
}

Token number(void) {
	while (isDigit(peek())) {
		advance();
	}

	if (peek() == '.' && isDigit(peekNext())) {
		advance();

		while (isDigit(peek())) {
			advance();
		}
	}

	if (peek() == 'e' && isDigit(peekNext())) {
		advance();

		while (isDigit(peek())) {
			advance();
		}
	}

	return makeToken(TK_NUM);
}

bool isAlpha(char c) {
	return (c >= 'a' && c <= 'z') ||
		(c >= 'A' && c <= 'Z') ||
		c == '_';
}

TOKEN_TYPE checkKeyword(size_t start, size_t len, char *rest, TOKEN_TYPE type) {
	if (scanner.current - scanner.start == start + len &&
		memcmp(scanner.start + start, rest, len) == 0) {
		return type;
	}

	return TK_IDENTIFIER;
}

TOKEN_TYPE identifierType(void) {

	switch (scanner.start[0]) {
		case 'a': return checkKeyword(1, 2, "nd", TK_AND);
		case 'b':
			if (scanner.current - scanner.start > 1) {
				switch (scanner.start[1]) {
					case 'o': return checkKeyword(2, 2, "ol", TK_BOOL);
					case 'r': return checkKeyword(2, 3, "eak", TK_BREAK);
				}
			}
		case 'c': {
			if (scanner.current - scanner.start > 1) {
				switch (scanner.start[1]) {
					case 'a': return checkKeyword(2, 2, "se", TK_CASE);
					case 'l': return checkKeyword(2, 3, "ass", TK_CLASS);
					case 'o': return checkKeyword(2, 3, "nst", TK_CONST);
				}
			}
		}
		case 'e':
			if (scanner.current - scanner.start > 1) {
				switch (scanner.start[1]) {
					case 'l': return checkKeyword(2, 2, "se", TK_ELSE);
					case 'x': return checkKeyword(2, 2, "it", TK_EXIT); 
				}
			}
		case 'f': {
			if (scanner.current - scanner.start > 1) {
				switch (scanner.start[1]) {
					case 'a': return checkKeyword(2, 3, "lse", TK_FALSE);
					case 'l': return checkKeyword(2, 1, "t", TK_FLT);
					case 'n': return checkKeyword(0, 2, "fn", TK_FN);;
					case 'o': return checkKeyword(2, 1, "r", TK_FOR);
				}
			}
		}
		case 'i': {
			if (scanner.current - scanner.start > 1) {
				switch (scanner.start[1]) {
					case 'f': return checkKeyword(0, 2, "if", TK_IF);
					case 'n': return checkKeyword(0, 2, "in", TK_IN);
					case '1': {
						if (scanner.current - scanner.start > 2) {
							return checkKeyword(2, 1, "6", TK_I16);
						}
					}						
					case '3': {
						if (scanner.current - scanner.start > 2) {
							return checkKeyword(2, 1, "2", TK_I32);
						}
					}
					case '6': {
						if (scanner.current - scanner.start > 2) {
							return checkKeyword(2, 1, "4", TK_I64);
						}
					}
					case '8': return checkKeyword(0, 2, "i8", TK_I8);		
				}
			}
		}
		case 'n': return checkKeyword(1, 2, "il", TK_NIL);
		case 'o': return checkKeyword(1, 1, "r", TK_OR);
		case 'r': return checkKeyword(1, 5, "eturn", TK_RETURN);
		case 's': {
			if (scanner.current - scanner.start > 1) {
				switch (scanner.start[1]) {
					case 'c': return checkKeyword(2, 3, "ope", TK_SCOPE);
					case 'w': return checkKeyword(2, 4, "itch", TK_SWITCH);
				}
			}
		}
		case 't': {
			if (scanner.current - scanner.start > 1) {
				switch (scanner.start[1]) {
					case 'h': return checkKeyword(2, 2, "is", TK_THIS);
					case 'r': return checkKeyword(2, 2, "ue", TK_TRUE);
				}
			}
		}
		case 'u': {
			if (scanner.current - scanner.start > 1) {
				switch (scanner.start[1]) {
					case '1': {
						if (scanner.current - scanner.start > 2) {
							return checkKeyword(2, 1, "6", TK_U16);
						}
					}						
					case '3': {
						if (scanner.current - scanner.start > 2) {
							return checkKeyword(2, 1, "2", TK_U32);
						}
					}
					case '6': {
						if (scanner.current - scanner.start > 2) {
							return checkKeyword(2, 1, "4", TK_U64);
						}
					}
					case '8': return checkKeyword(0, 2, "u8", TK_U8);
				}
			}			
		}
		case 'w': return checkKeyword(1, 4, "hile", TK_WHILE);
		case 'S': return checkKeyword(1, 2, "tr", TK_STR);
	}
	return TK_IDENTIFIER;
}

Token identifier(void) {
	while (isAlpha(peek()) || isDigit(peek())) {
		advance();
	}
	return makeToken(identifierType());

}

Token scanToken(void) {

	skipWhitespace();

	scanner.start = scanner.current;

	if (isAtEnd()) return makeToken(TK_EOF);

	char c = advance();

	if (isAlpha(c)) {
		return identifier();
	}
	if (isDigit(c)) {
		return number();
	}

	switch(c) {
		case '(': return makeToken(TK_LPAREN);
		case ')': return makeToken(TK_RPAREN);
		case '{': return makeToken(TK_LBRACE);
		case '}': return makeToken(TK_RBRACE);
		case '[': return makeToken(TK_LSQUARE);
		case ']': return makeToken(TK_RSQUARE);
		case ',': return makeToken(TK_COMMA);
		case '.': return makeToken(TK_DOT);
		case '-': return makeToken(TK_MINUS);
		case '+': return makeToken(TK_PLUS);
		case ';': return makeToken(TK_SEMICOLON);
		case '/': return makeToken(TK_FSLASH);
		case '*': return makeToken(TK_STAR);
		case '%': return makeToken(TK_MOD);
		case ':': return makeToken(TK_COLON);
		case '|': return makeToken(TK_PIPE);
		case '#': return makeToken(TK_HASH);

		case '!':
			return makeToken (
				match('=') ? TK_BANG_EQUAL : TK_BANG);
		case '=':
			return makeToken (
				match('=') ? TK_EQUAL_EQUAL : TK_EQUAL);
		case '<':
			return makeToken (
				match('=') ? TK_LESS_EQUAL : TK_LESS);
		case '>':
			return makeToken (
				match('=') ? TK_GREATER_EQUAL : TK_GREATER);
		case '"': 
			return string();
	}
	return makeErrorToken("unexpected character");
}

char *tokenLabelLookup(TOKEN_TYPE type) {
	return __tokenNameLiterals[type];
}

size_t countTokens(char *source, bool *had_error) {
	initScanner(source);

	size_t n_tok = 0;
	*had_error = false;

	while(true) {
		Token token = scanToken();
		n_tok++;
		if (token.type == TK_ERROR) {
			*had_error = true;
			break;
		}
		if (token.type == TK_EOF) {
			break;
		}
	}
	return n_tok;
}

void __printToken(Token *token) {
	printf("LINE: %6zu TYPE: %16s - \"%.*s\"\n", 
		token->line,
		tokenLabelLookup(token->type), 
		(int)token->len, token->start);
	return;
}
