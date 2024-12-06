#ifndef FILE_H
#define FILE_H

#include <stdio.h>
#include <stdlib.h>

#include "mempool.h"

char *pReadFile(char *name, MemPool *pool);

int writeFileChars(char *name, char *source);

FILE *checkFileOpen(char *name, char *mode);

#endif // FILE_H
