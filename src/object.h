
#ifndef OBJECT_H
#define OBJECT_H

#include "common.h"

typedef enum {
	NIL_TYPE,
	STR_TYPE,
	I64_TYPE,
	DBL_TYPE,
	BLN_TYPE,
	PTR_TYPE,
} OBJ_TYPE;

union Value {
	char *str;
	int64_t i64;
	double dbl;
	bool bln;
	void *ptr;
};

typedef struct s_Object {
	OBJ_TYPE type;
	union Value val;
	int precision;
} Object;

#endif // OBJECT_H
