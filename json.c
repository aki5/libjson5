#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include "json.h"

#define nelem(x) (sizeof(x)/sizeof(x[0]))

static __thread char *jsonfile = "";
static __thread int jsonline;

static int
isterminal(int c)
{
	switch(c){
	case JsonArray:
	case JsonObject:
	case JsonNumber:
	case JsonString:
	case JsonSymbol:
	case JsonInteger:
		return 1;
	}
	return 0;
}

static int
isnumber(int c)
{
	switch(c){
	case '0': case '1': case '2': case '3': case '4':
	case '5': case '6': case '7': case '8': case '9':
/**/
	case 'e': case 'E':
	case '+': case '-':
	case '.':
		return 1;
	}
	return 0;
}

static int
iswhite(int c)
{
	switch(c){
	case '\t': case '\n': case '\v': case '\f': case '\r': case ' ':
		return 1;
	}
	return 0;
}

static int
isbreak(int c)
{
	switch(c){
	case '{': case '}':
	case '[': case ']':
	//case '(': case ')':
	case ':': case ',':
	case '+': case '-':
	case '.':
	case '"':

	case '\t': case '\n': case '\v': case '\f': case '\r': case ' ':
		return 1;
	}
	return 0;
}

static int
issymbol(char *buf, int len)
{
	if(len == 4){
		if(memcmp(buf, "true", 4) == 0)
			return 1;
		if(memcmp(buf, "null", 4) == 0)
			return 1;
	} else if(len == 5){
		if(memcmp(buf, "false", 5) == 0)
			return 1;
	}
	return 0;
}

// notice that all the switch-case business and string scanning looks for ascii values < 128,
// hence there is no need to decode utf8 explicitly, it just works.
static int
jsonlex(char *buf, int *offp, int *lenp, char **tokp)
{
	char *str;
	int len, qch;
	int isinteger;

	str = buf + *offp;
	len = *lenp;

again:
	while(len > 0 && iswhite(*str)){
		if(*str == '\n')
			jsonline++;
		str++;
		len--;
	}
	*tokp = str;
	if(len > 0){
		// a number is an integer unless it has '.' or 'E' in it.
		isinteger = 1;
		switch(*str){

		// c and c++ style comments, not standard either
		case '/':
			if(len > 1 && str[1] == '/'){
				while(len > 1 && str[1] != '\n'){
					str++;
					len--;
				}
				// advance to newline
				if(len > 0){
					str++;
					len--;
				}
				// skip newline
				if(len > 0 && *str == '\n'){
					jsonline++;
					str++;
					len--;
				}
				goto again;
			} else if(len > 1 && str[1] == '*'){
				while(len > 1 && !(str[0] == '*' && str[1] == '/')){
					if(str[1] == '\n')
						jsonline++;
					str++;
					len--;
				}
				// skip '*'
				if(len > 0){
					str++;
					len--;
				}
				// skip '/'
				if(len > 0){
					str++;
					len--;
				}
				goto again;
			}
			goto caseself;

		// straight forward return self.
		case '{': case '}':
		case '[': case ']':
		//case '(': case ')':
		case ':':
		case ',':
		caseself:
			*offp = str+1-buf;
			*lenp = len-1;
			return *str;

		// it may be number, look at the next character and decide.
		case '.':
			isinteger = 0;
		case '+': case '-':
			if(len < 2)
				goto caseself;
			if(str[1] < '0' || str[1] > '9')
				goto caseself;
			str++;
			len--;
			/* fall through */

		// the number matching algorithm is a bit fast and loose, but it does the business.
		case '0': case '1': case '2': case '3': case '4':
		case '5': case '6': case '7': case '8': case '9':
			while(len > 1){
				if(!isnumber(str[1]))
					break;
				if(str[1] == '.' || str[1] == 'e' || str[1] == 'E')
					isinteger = 0;
				str++;
				len--;
			}
			*offp = str+1-buf;
			*lenp = len-1;
			return isinteger ? JsonInteger : JsonNumber;

		// nonstandard single quote strings..
		case '\'':
			qch = '\'';
			goto casestr;

		// string constant, detect and skip escapes but don't interpret them
		case '"':
			qch = '"';
		casestr:
			while(len > 1 && str[1] != qch){
				if(len > 1 && str[1] == '\\'){
					switch(str[2]){
					default:
						str++;
						len--;
						break;
					case 'u':
						str += len < 5 ? len : 5;
						len -= len < 5 ? len : 5;
						break;
					}
				}
				str++;
				len--;
			}
			if(len > 1 && str[1] == qch){
				str++;
				len--;
			}
			*offp = str+1-buf;
			*lenp = len-1;
			return JsonString;

		// symbol is any string of nonbreak characters not starting with a number
		default:
			while(len > 1 && !isbreak(str[1])){
				str++;
				len--;
			}
			*offp = str+1-buf;
			*lenp = len-1;
			if(issymbol(*tokp, str-*tokp+1))
				return JsonSymbol;
			return -1; // not a symbol, means error!
		}
	}
	// set them here too, so that we don't re-read the last character indefinitely
	*offp = str+1-buf;
	*lenp = len-1;
	return -1;
}

static int jsonany(JsonAst *ast, int *astoff, int jscap, char *buf, int *offp, int *lenp);

static int
jsonarray(JsonAst *ast, int *astoff, int jscap, char *buf, int *offp, int *lenp)
{
	int patch, lt;
	int ret;

	// array begin
	patch = *astoff;
	if(patch < jscap){
		ast[patch].type = '[';
		ast[patch].off = -1;
		ast[patch].len = -1;
	}
	*astoff = patch+1;

	ret = '[';
	for(;;){
		lt = jsonany(ast, astoff, jscap, buf, offp, lenp);
nocomma:
		if(lt == -1){
			ret = -1;
			break;
		}
		if(lt == ']')
			break;
		if(!isterminal(lt))
			fprintf(stderr, "%s:%d: jsonarray: got '%c', expecting object or terminal\n", jsonfile, jsonline, lt);

		lt = jsonany(ast, astoff, jscap, buf, offp, lenp);
		if(lt == -1){
			ret = -1;
			break;
		}
		if(lt == ']')
			break;
		if(lt != ','){
			fprintf(stderr, "%s:%d: jsonarray: got '%c', expecting ','\n", jsonfile, jsonline, lt);
			goto nocomma;
		}
	}
	if(patch < jscap)
		ast[patch].next = *astoff + 1;

	// array end
	patch = *astoff;
	if(patch < jscap){
		ast[patch].type = ']';
		ast[patch].off = -1;
		ast[patch].len = -1;
		ast[patch].next = patch+1;
	}
	*astoff = patch+1;
	return ret;
}


static int
jsonobject(JsonAst *ast, int *astoff, int jscap, char *buf, int *offp, int *lenp)
{
	int patch, lt;
	int ret;

	// object begin
	patch = *astoff;
	if(patch < jscap){
		ast[patch].type = '{';
		ast[patch].off = -1;
		ast[patch].len = -1;
	}
	*astoff = patch+1;

	ret = '{';
	for(;;){
		lt = jsonany(ast, astoff, jscap, buf, offp, lenp);
nocomma:
		if(lt == -1){
			ret = -1;
			break;
		}
		if(lt == '}')
			break;
		if(lt != JsonString)
			fprintf(stderr, "%s:%d: jsonobject: got '%c', expecting key ('\"')\n", jsonfile, jsonline, lt);

		lt = jsonany(ast, astoff, jscap, buf, offp, lenp);
		if(lt == -1){
			ret = -1;
			break;
		}
		if(lt != ':')
			fprintf(stderr, "%s:%d: jsonobject: got '%c', expecting colon (':')\n", jsonfile, jsonline, lt);

		lt = jsonany(ast, astoff, jscap, buf, offp, lenp);
		if(lt == -1){
			ret = -1;
			break;
		}
		if(!isterminal(lt))
			fprintf(stderr, "%s:%d: jsonobject: got '%c', expecting object or terminal\n", jsonfile, jsonline, lt);

		lt = jsonany(ast, astoff, jscap, buf, offp, lenp);
		if(lt == -1){
			ret = -1;
			break;
		}
		if(lt == '}')
			break;
		if(lt != ','){
			fprintf(stderr, "%s:%d: jsonobject: got '%c', expecting ','\n", jsonfile, jsonline, lt);
			goto nocomma;
		}

	}
	if(patch < jscap)
		ast[patch].next = *astoff + 1;

	// object end
	patch = *astoff;
	if(patch < jscap){
		ast[patch].type = '}';
		ast[patch].off = -1;
		ast[patch].len = -1;
		ast[patch].next = patch+1;
	}
	*astoff = patch+1;
	return ret;
}

void
jsonsetname(char *filename)
{
	jsonline = 1;
	jsonfile = filename;
}

static int
jsonany(JsonAst *ast, int *astoff, int jscap, char *buf, int *offp, int *lenp)
{
	char *tok;
	int patch, lt;

	switch(lt = jsonlex(buf, offp, lenp, &tok)){
	case -1:
		return -1;
	case '[':
		return jsonarray(ast, astoff, jscap, buf, offp, lenp);
	case '{':
		return jsonobject(ast, astoff, jscap, buf, offp, lenp);
	case ']': case '}': case ':': case ',':
		return lt;
	case JsonNumber:
	case JsonInteger:
	case JsonString:
	case JsonSymbol:
		patch = *astoff;
		if(patch < jscap){
			ast[patch].type = lt;
			ast[patch].off = tok-buf;
			ast[patch].len = *offp - (tok-buf);
			ast[patch].next = patch+1;
		}
		*astoff = patch+1;
		return lt;
	default:
		fprintf(stderr, "%s:%d: jsonany: unexpected token: '", jsonfile, jsonline);
		fwrite(tok, *offp - (tok-buf), 1, stderr);
		fprintf(stderr, "'\n");
		return -1;
	}
}

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

int
jsonparse(JsonRoot *root, char *buf, int len)
{
	JsonAst *ast;
	int off, buflen, jsoff, jscap;

	ast = root->ast.buf;
	jscap = root->ast.cap;
	off = 0;
	buflen = len;
	jsoff = 0;

	jsonline = 1;
	if(jsonany(ast, &jsoff, jscap, buf, &off, &buflen) == -1)
		return -1;

	if(jscap < jsoff){
		jscap = jsoff;
		jsonline = 1;
		if((ast = realloc(ast, jscap * sizeof ast[0])) == NULL){
			fprintf(stderr, "jsonroot: cannot malloc ast\n");
			return -1;
		}

		jsoff = 0;
		off = 0;
		buflen = len;
		if(jsonany(ast, &jsoff, jscap, buf, &off, &buflen) == -1)
			return -1;
	}

	root->ast.buf = ast;
	root->ast.len = jsoff;
	root->ast.cap = jscap;

	root->str.buf = buf;
	root->str.len = len;
	root->str.cap = len;

	return 0;
}

void
jsonfree(JsonRoot *root)
{
	if(root->ast.buf != NULL)
		free(root->ast.buf);
	memset(root, 0, sizeof root[0]);
}

int
jsonwalk2(JsonRoot *root, int off, char *name, int keylen)
{
	JsonAst *ast;
	char *buf;
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
jsonwalk(JsonRoot *root, int off, char *name)
{
	return jsonwalk2(root, off, name, strlen(name));
}

int
jsonindex(JsonRoot *root, int off, int index)
{
	JsonAst *ast;
	char *buf;
	int i, j;

	if(off == -1)
		return -1;

	ast = root->ast.buf;
	buf = root->str.buf;

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

// walk a json-ptr
int
jsonptr(JsonRoot *root, int off, char *ptr, int ptrlen)
{
	char *p;

	while((p = memchr(ptr, '/', ptrlen)) != NULL){
//		fprintf(stderr, "jsonptr: walk %.*s\n", (int)(p-ptr), ptr);
		off = jsonwalk2(root, off, ptr, p-ptr);
		if(off == -1){
			fprintf(stderr, "jsonptr: could not walk %.*s\n", (int)(p-ptr), ptr);
			return -1;
		}
		ptrlen -= p-ptr+1;
		ptr = p+1;
	}

//	fprintf(stderr, "jsonptr: last walk %.*s\n", ptrlen, ptr);
	off = jsonwalk2(root, off, ptr, ptrlen);

	return off;
}

int
jsonrefs(JsonRoot *docroot)
{
	JsonAst *ast;
	int off, off2;

	ast = docroot->ast.buf;
	for(off = 0; off < docroot->ast.len; off++){
		if(ast[off].type == '{'){
			off2 = jsonwalk(docroot, off, "$ref");
			if(off2 != -1){
				char *ref;
				int reflen;

				ref = docroot->str.buf+ast[off2].off+1;
				reflen = ast[off2].len-2;
				if(ref[0] != '#' || ref[1] != '/'){
					fprintf(stderr, "jsonrefs: only local, absolute references are supported\n");
					return -1;
				}

				// rewrite the JsonObject to a JsonReference.
				ast[off].type = JsonReference;
				ast[off].off = jsonptr(docroot, 0, ref+2, reflen-2);
			}
		}
	}
	return 0;
}

static int
scmtype(JsonRoot *scmroot, int off)
{
	JsonAst *ast;
	char *buf;
	int len;

	if(off == -1)
		return -1;

	ast = scmroot->ast.buf + off;
	if(ast->type != JsonString || ast->len < 2){
		fprintf(stderr, "scmtype: value is not string\n");
		return -1;
	}

	buf = scmroot->str.buf + ast->off + 1;
	len = ast->len - 2;
	if(len == 7){
		if(memcmp(buf, "integer", 7) == 0)
			return JsonInteger;
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
	fprintf(stderr, "scmtype: value is not string\n");
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
	if(docast[docoff].type != scmtyp && !(scmtyp == JsonNumber && docast[docoff].type == JsonInteger)){
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
			fprintf(stderr, "jsoncheck: schema has no properties field\n");
			return -1;
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
