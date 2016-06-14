#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include "json.h"

// this isn't exactly my finest hour...
char *
jsoncstr(JsonRoot *root, int i)
{
	JsonAst *ast;
	char *buf, *str;

	ast = root->ast.buf;
	if(ast[i].type != JsonString){
		fprintf(stderr, "jsoncstring: element is '%c', want string\n", ast[i].type);
		return NULL;
	}
	if(ast[i].len < 2){
		fprintf(stderr, "jsoncstring: element too short (%d) to be a offd json string\n", ast[i].len);
		return NULL;
	}
	if((str = malloc(ast[i].len-1)) == NULL){
		fprintf(stderr, "jsoncstr: malloc failed\n");
		return NULL;
	}

	buf = root->str.buf;
	memcpy(str, buf + ast[i].off + 1, ast[i].len-2);
	str[ast[i].len-2] = '\0';

	return str;
}
