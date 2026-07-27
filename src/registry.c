#include "registry.h"

#include <string.h>

/* Growth is arena-backed: pGrowAlloc copies into a fresh block and the old one
 * is reclaimed when the pool is torn down. Both tables are tiny and are filled
 * once at load time, so the wasted space is irrelevant. */
static void *growArray(MemPool *pool, void *ptr, size_t *cap, size_t used,
                       size_t elem_size) {
	size_t new_cap;
	if (used < *cap) {
		return ptr;
	}
	new_cap = (*cap == 0) ? 16 : *cap * 2;
	ptr = pGrowAlloc(ptr, *cap * elem_size, new_cap * elem_size, pool);
	*cap = new_cap;
	return ptr;
}

static const char *internChars(MemPool *pool, const char *s, size_t len) {
	char *out = palloc(pool, len + 1);
	memcpy(out, s, len);
	out[len] = '\0';
	return out;
}

static size_t nameTablePush(NameTable *tbl, MemPool *pool, const char *name) {
	tbl->names = growArray(pool, tbl->names, &tbl->cap, tbl->n, sizeof(const char *));
	tbl->names[tbl->n] = name;
	return tbl->n++;
}

static size_t nameTableFind(NameTable *tbl, const char *name, size_t len) {
	for (size_t i = 0; i < tbl->n; i++) {
		if (strlen(tbl->names[i]) == len && 0 == memcmp(tbl->names[i], name, len)) {
			return i;
		}
	}
	return tbl->n; /* == n means "not found" */
}

/* Keywords live in one array sorted by first byte, with a 256-entry bucket
 * index over it. Lookup is one array index plus a memcmp against the handful of
 * keywords sharing that first letter. Rebuilt on every add, which happens at
 * most a few dozen times at load. */
static void reindexKeywords(Registry *reg) {
	for (size_t i = 1; i < reg->n_keywords; i++) {
		KeywordEntry key = reg->keywords[i];
		size_t j = i;
		while (j > 0 &&
		       (unsigned char)reg->keywords[j - 1].word[0] > (unsigned char)key.word[0]) {
			reg->keywords[j] = reg->keywords[j - 1];
			j--;
		}
		reg->keywords[j] = key;
	}

	memset(reg->kw_start, 0, sizeof(reg->kw_start));
	memset(reg->kw_count, 0, sizeof(reg->kw_count));

	for (size_t i = 0; i < reg->n_keywords; i++) {
		unsigned char c = (unsigned char)reg->keywords[i].word[0];
		if (reg->kw_count[c] == 0) {
			reg->kw_start[c] = i;
		}
		reg->kw_count[c]++;
	}
}

static TOKEN_TYPE definePunct(Registry *reg, const char *name, const char *lex,
                              size_t llen) {
	TOKEN_TYPE type = (TOKEN_TYPE)nameTablePush(&reg->tokens, reg->pool, name);

	if (llen == 1) {
		reg->punct1[(unsigned char)lex[0]] = type;
	} else {
		reg->punct2 = growArray(reg->pool, reg->punct2, &reg->cap_punct2,
		                        reg->n_punct2, sizeof(PunctEntry));
		reg->punct2[reg->n_punct2].c0 = lex[0];
		reg->punct2[reg->n_punct2].c1 = lex[1];
		reg->punct2[reg->n_punct2].type = type;
		reg->n_punct2++;
	}
	return type;
}

static TOKEN_TYPE defineKeyword(Registry *reg, const char *name, const char *lex,
                                size_t llen) {
	TOKEN_TYPE type = (TOKEN_TYPE)nameTablePush(&reg->tokens, reg->pool, name);

	reg->keywords = growArray(reg->pool, reg->keywords, &reg->cap_keywords,
	                          reg->n_keywords, sizeof(KeywordEntry));
	reg->keywords[reg->n_keywords].word = lex;
	reg->keywords[reg->n_keywords].len = llen;
	reg->keywords[reg->n_keywords].type = type;
	reg->n_keywords++;

	reindexKeywords(reg);
	return type;
}

void initRegistry(Registry *reg, MemPool *pool) {
	memset(reg, 0, sizeof(*reg));
	reg->pool = pool;

	for (size_t i = 0; i < 256; i++) {
		reg->punct1[i] = TK__NONE;
	}

	/* Seed both vocabularies from the .def files. Because the tables are filled
	 * in the same order the enums are declared, index == enum value by
	 * construction. */
#define TK_PUNCT(name, lexeme)   definePunct(reg, #name, lexeme, sizeof(lexeme) - 1);
#define TK_KEYWORD(name, lexeme) defineKeyword(reg, #name, lexeme, sizeof(lexeme) - 1);
#define TK_SPECIAL(name)         nameTablePush(&reg->tokens, reg->pool, #name);
#include "token.def"
#undef TK_PUNCT
#undef TK_KEYWORD
#undef TK_SPECIAL

#define STX(name) nameTablePush(&reg->syntax, reg->pool, #name);
#include "syntax.def"
#undef STX
}

TOKEN_TYPE registryFindToken(Registry *reg, const char *name, size_t len) {
	size_t i = nameTableFind(&reg->tokens, name, len);
	return (i == reg->tokens.n) ? TK__NONE : (TOKEN_TYPE)i;
}

SYNTAX_TYPE registryFindSyntax(Registry *reg, const char *name, size_t len) {
	size_t i = nameTableFind(&reg->syntax, name, len);
	return (i == reg->syntax.n) ? STX__NONE : (SYNTAX_TYPE)i;
}

SYNTAX_TYPE registryInternSyntax(Registry *reg, const char *name, size_t len) {
	SYNTAX_TYPE existing = registryFindSyntax(reg, name, len);
	if (existing != STX__NONE) {
		return existing;
	}
	return (SYNTAX_TYPE)nameTablePush(&reg->syntax, reg->pool,
	                                  internChars(reg->pool, name, len));
}

const char *tokenName(Registry *reg, TOKEN_TYPE type) {
	if (type < 0 || (size_t)type >= reg->tokens.n) {
		return "<unknown token>";
	}
	return reg->tokens.names[type];
}

const char *syntaxName(Registry *reg, SYNTAX_TYPE type) {
	if (type < 0 || (size_t)type >= reg->syntax.n) {
		return "<anon>";
	}
	return reg->syntax.names[type];
}

size_t registrySyntaxCount(Registry *reg) {
	return reg->syntax.n;
}

size_t registryTokenCount(Registry *reg) {
	return reg->tokens.n;
}

TOKEN_TYPE registryAddPunct(Registry *reg, const char *name, size_t nlen,
                            const char *lex, size_t llen) {
	if (llen < 1 || llen > 2) {
		return TK__NONE;
	}
	if (registryFindToken(reg, name, nlen) != TK__NONE) {
		return TK__NONE;
	}
	return definePunct(reg, internChars(reg->pool, name, nlen),
	                   internChars(reg->pool, lex, llen), llen);
}

TOKEN_TYPE registryAddKeyword(Registry *reg, const char *name, size_t nlen,
                              const char *lex, size_t llen) {
	if (llen < 1) {
		return TK__NONE;
	}
	if (registryFindToken(reg, name, nlen) != TK__NONE) {
		return TK__NONE;
	}
	return defineKeyword(reg, internChars(reg->pool, name, nlen),
	                     internChars(reg->pool, lex, llen), llen);
}

TOKEN_TYPE registryMatchPunct(Registry *reg, char c0, char c1, size_t *out_len) {
	if (c1 != '\0') {
		for (size_t i = 0; i < reg->n_punct2; i++) {
			if (reg->punct2[i].c0 == c0 && reg->punct2[i].c1 == c1) {
				*out_len = 2;
				return reg->punct2[i].type;
			}
		}
	}
	if (reg->punct1[(unsigned char)c0] != TK__NONE) {
		*out_len = 1;
		return reg->punct1[(unsigned char)c0];
	}
	*out_len = 0;
	return TK__NONE;
}

TOKEN_TYPE registryMatchKeyword(Registry *reg, const char *word, size_t len) {
	unsigned char c = (unsigned char)word[0];
	size_t start = reg->kw_start[c];
	size_t end = start + reg->kw_count[c];

	for (size_t i = start; i < end; i++) {
		if (reg->keywords[i].len == len &&
		    0 == memcmp(reg->keywords[i].word, word, len)) {
			return reg->keywords[i].type;
		}
	}
	return TK__NONE;
}
