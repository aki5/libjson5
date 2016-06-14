#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include "json.h"

int
jsonindex(JsonRoot *root, int off, int index)
{
	JsonAst *ast;
	int i, j;

	if(off == -1)
		return -1;

	ast = root->ast.buf;

	if(ast[off].type != '['){
		fprintf(stderr, "jsonindex: called on a non-array '%c'\n", ast[off].type);
		return -1;
	}

	// skip elements until we find the right one.
	i = off+1;
	for(j = 0; j < index; j++){
		if(ast[i].type == ']')
			break;
		// next array element
		i = ast[i].next;
	}
	if(j != index){
		fprintf(stderr, "jsonindex: index %d out of bounds, max %d\n", index, j);
		return -1;
	}
	return i;
}
