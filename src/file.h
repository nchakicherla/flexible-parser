#ifndef FILE_H
#define FILE_H

#include <stdio.h>
#include <stdlib.h>

#include "mempool.h"

char *tryReadFile(char *name, MemPool *pool);
int writeFileChars(char *name, char *source);
FILE *tryFileOpen(char *name, char *mode);

#endif // FILE_H
