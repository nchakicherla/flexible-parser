#ifndef OBJECT_H
#define OBJECT_H

/* object.h - the interpreter's runtime value.
 *
 * Strings are pool-allocated and never freed individually; the whole arena goes
 * at once when the run ends. */

#include "common.h"
#include "mempool.h"

typedef enum {
	NIL_TYPE,
	STR_TYPE,
	I64_TYPE,
	DBL_TYPE,
	BLN_TYPE,
	PTR_TYPE,
} OBJ_TYPE;

union Value {
	const char *str;
	int64_t i64;
	double dbl;
	bool bln;
	void *ptr;
};

typedef struct s_Object {
	OBJ_TYPE type;
	union Value val;
} Object;

Object objNil(void);
Object objI64(int64_t v);
Object objDbl(double v);
Object objBln(bool v);
Object objStr(const char *s);
Object objPtr(void *p);

bool objIsNumber(Object o);
bool objIsTruthy(Object o);

/* Numeric view of a value, for mixed int/float arithmetic. */
double objAsDouble(Object o);

const char *objTypeName(OBJ_TYPE type);
void objPrint(Object o);

/* Interns `len` bytes as a NUL-terminated pool string. */
const char *objInternChars(MemPool *pool, const char *s, size_t len);

#endif // OBJECT_H
