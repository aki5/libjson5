#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include "json.h"

// walk a json-ptr
int
jsonptr(JsonRoot *root, int off, char *ptr, int ptrlen)
{
	char *p;

	while((p = memchr(ptr, '/', ptrlen)) != NULL){
		off = jsonwalk2(root, off, ptr, p-ptr);
		if(off == -1){
			fprintf(stderr, "jsonptr: could not walk %.*s\n", (int)(p-ptr), ptr);
			return -1;
		}
		ptrlen -= p-ptr+1;
		ptr = p+1;
	}

	off = jsonwalk2(root, off, ptr, ptrlen);

	return off;
}
