#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include "json.h"

int
jsonwalk2(JsonRoot *root, int off, const char *name, int keylen)
{
	JsonAst *ast;
	const char *buf;
	int i;

	if(off == -1)
		return -1;

	ast = root->ast.buf;
	buf = root->str.buf;

//	if(ast[off].type == JsonReference)
//		off = ast[i].off;

	if(ast[off].type != '{'){
		fprintf(stderr, "jsonwalk: called on a non-object '%c'\n", ast[off].type);
		return -1;
	}

	i = off+1;
	for(;;){
		if(ast[i].type == '}')
			break;
		if(ast[i].type != JsonString){
			fprintf(stderr, "jsonwalk: bad type '%c', expecting string for map key", ast[i].type);
			fwrite(buf+ast[i].off, ast[i].len, 1, stderr);
			fprintf(stderr, "\n");
			return -1;
		}
		if(keylen == ast[i].len-2 && !memcmp(buf + ast[i].off+1, name, keylen)){
			int next;

			next = ast[i].next;
			// if value is a resolved reference, go to destination, which is stored
			// in the off field because 'next' chains the higher level object or array.
			if(ast[next].type == JsonReference)
				next = ast[next].off;
			// don't loop since that can get stuck in a cycle
			if(ast[next].type == JsonReference){
				fprintf(stderr, "jsonwalk: cascading reference.\n");
				return -1;
			}
			return next;
		}

		i = ast[i].next; // to value
		i = ast[i].next; // to next key
	}
	return -1; // not found.
}

int
jsonwalk(JsonRoot *root, int off, const char *name)
{
	return jsonwalk2(root, off, name, strlen(name));
}
