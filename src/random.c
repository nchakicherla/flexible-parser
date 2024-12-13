#include "random.h"

int64_t randI64(int64_t range) {
	uint64_t high = ((uint64_t)rand() << 32);
	uint64_t low = ((uint64_t)rand());
	return (high | low) % range;
}

int fillI64ArrRand(int64_t *arr, size_t len, int64_t range) {

	if(arr == NULL) return 1;
	
	srand(time(NULL));

	for(size_t i = 0; i < len; i++) {
		arr[i] = randI64(range);
	}
	return 0;
}
