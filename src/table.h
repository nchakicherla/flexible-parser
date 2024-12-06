#ifndef TABLE_H
#define TABLE_H

#include "common.h"
#include "mempool.h"
#include "object.h"

#include <stdlib.h>

struct s_Entry;

typedef struct s_Entry {
	char *key;
	Object obj;
	uint32_t hash;
	struct s_Entry *next;
} Entry;

typedef struct s_Table {
	size_t count;
	size_t n_buckets;
	Entry **entries;
	MemPool pool;
} Table;

uint32_t FNV_1a_hash(char* str);

int initTable(Table *table);

int termTable(Table *table);

// void free_table(Table *table);

int insertKey(Table *table, char *key, void *value, OBJ_TYPE type);

int removeKey(Table *table, char *key);

Object *getObject(Table *table, char *key);

#endif // TABLE_H
