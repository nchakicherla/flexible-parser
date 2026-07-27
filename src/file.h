#ifndef FILE_H
#define FILE_H

#include <stdio.h>
#include <stdlib.h>

#include "mempool.h"

char *tryReadFile(const char *name, MemPool *pool);
int tryWriteChars(const char *name, const char *source);
FILE *tryFileOpen(const char *name, const char *mode);

#endif // FILE_H
