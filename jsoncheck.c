#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include "json.h"

static int
scmtype(JsonRoot *scmroot, int off)
{
	JsonAst *ast;
	const char *buf;
	int len;

	if(off == -1)
		return -1;

	ast = scmroot->ast.buf + off;
	buf = scmroot->str.buf + ast->off;
	len = ast->len;
	if(ast->type != JsonString || len < 2){
		fprintf(stderr, "scmtype: value '%.*s' is not a valid type\n", len > 0 ? len : 0, buf);
		return -1;
	}

	buf++;
	len -= 2;
	if(len == 7){
		if(memcmp(buf, "integer", 7) == 0)
			return JsonInteger;
		if(memcmp(buf, "boolean", 7) == 0)
			return JsonBoolean;
	} else if(len == 6){
		if(memcmp(buf, "string", 6) == 0)
			return JsonString;
		if(memcmp(buf, "object", 6) == 0)
			return JsonObject;
		if(memcmp(buf, "number", 6) == 0)
			return JsonNumber;
	} else if(len == 5){
		if(memcmp(buf, "array", 5) == 0)
			return JsonArray;
	}
	fprintf(stderr, "scmtype: value '%.*s' is not a valid type\n", len, buf);
	return -1;
}

int
jsoncheck(JsonRoot *docroot, int docoff, JsonRoot *scmroot, int scmoff)
{
	JsonAst *docast, *scmast;
	int scmtyp, scmoff2;

	docast = docroot->ast.buf;
	scmast = scmroot->ast.buf;

	scmoff2 = jsonwalk(scmroot, scmoff, "type");
	if(scmoff2 == -1){
		fprintf(stderr, "jsoncheck: schema: no type property\n");
		return -1;
	}

	// this is the actual type checking code, complex cases end here through recursion.
	scmtyp = scmtype(scmroot, scmoff2);
	if(docast[docoff].type != scmtyp
	&& !(scmtyp == JsonNumber && docast[docoff].type == JsonInteger) // integer in doc is a valid number too
	&& !(scmtyp == JsonObject && docast[docoff].type == JsonNull) // null in doc is a valid object
	){
		fprintf(stderr, "jsoncheck: doctype %c scmtype %c\n", docast[docoff].type, scmtyp);
		return -1;
	}

	if(scmtyp == JsonArray){
		// walk schema to items,
		scmoff = jsonwalk(scmroot, scmoff, "items");
		if(scmoff == -1)
			return -1;
		docoff++;
		// check every array item against schema item
		while(docoff < docroot->ast.len){
			if(docast[docoff].type == ']')
				break;
			if(jsoncheck(docroot, docoff, scmroot, scmoff) == -1)
				return -1;
			docoff = docast[docoff].next;
		}
		if(docoff == docast->len)
			return -1;
	} else if(scmtyp == JsonObject){

		scmoff2 = jsonwalk(scmroot, scmoff, "required");
		if(scmoff2 != -1){
			if(scmast[scmoff2].type != JsonArray){
				fprintf(stderr, "jsoncheck: schema has non-array required field\n");
				return -1;
			}
			scmoff2++;
			while(scmoff2 < scmroot->ast.len){
				int docoff2;
				if(scmast[scmoff2].type == ']')
					break;
				docoff2 = jsonwalk2(docroot, docoff, scmroot->str.buf+scmast[scmoff2].off+1, scmast[scmoff2].len-2);
				if(docoff2 == -1){
					fprintf(stderr, "jsoncheck: required field '%.*s' missing\n", scmast[scmoff2].len, scmroot->str.buf+scmast[scmoff2].off);
				}
				scmoff2 = scmast[scmoff2].next;
			}
		}

		scmoff2 = jsonwalk(scmroot, scmoff, "properties");
		if(scmoff2 == -1){
			return 0; // no properties in schema means it can be any object..
			//fprintf(stderr, "jsoncheck: schema has no properties field\n");
			//return -1;
		}

		// to first key of the object.
		docoff++;
		scmoff = scmoff2;
		// check every doc property against corresponding schema property
		while(docoff < docroot->ast.len){
			if(docast[docoff].type == '}')
				break;
			if(docast[docoff].type != JsonString || docast[docoff].len < 2)
				return -1;
			scmoff2 = jsonwalk2(scmroot, scmoff, docroot->str.buf+docast[docoff].off+1, docast[docoff].len-2);
			if(scmoff2 == -1)
				return -1;

			docoff = docast[docoff].next;
			if(jsoncheck(docroot, docoff, scmroot, scmoff2) == -1)
				return -1;
			docoff = docast[docoff].next;
		}
		if(docoff == docast->len)
			return -1;
	}
	return 0;
}
