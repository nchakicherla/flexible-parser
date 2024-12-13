#ifndef RANDOM_H
#define RANDOM_H

#include "common.h"
#include <time.h>
#include <stdlib.h>

int64_t randI64(int64_t range);
int fillI64ArrRand(int64_t *arr, size_t len, int64_t range);

#endif // RANDOM_H
