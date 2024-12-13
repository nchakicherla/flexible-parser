#include "file.h"

char *tryReadFile(char *name, MemPool *pool) {
	FILE *file = NULL;
	char *output = NULL;
	long size;

	file = fopen(name, "r");
	if (!file)
		return NULL;

	// get length of file as size, then rewind pointer
	fseek(file, 0, SEEK_END);
	size = ftell(file);
	fseek(file, 0, SEEK_SET);
	
	if(pool) {
		output = palloc(pool, size + 1);
	} else {
		fclose(file);
		return NULL;
	}
	fread(output, size, 1, file);
	output[size] = '\0';

	fclose(file);
	return output;
}

int writeFileChars(char *name, char *source) {
	FILE *file = NULL;

	file = fopen(name, "w");
	if(!file) return 1;

	fprintf(file, "%s", source);
	fclose(file);
	return 0;
}

FILE *tryFileOpen(char *name, char *mode) {
	FILE *fp = NULL;
	if(!(fp = fopen(name, mode))) {
		return NULL;
	}
	return fp;
}

