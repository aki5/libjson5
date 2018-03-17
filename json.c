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
	case JsonBoolean:
	case JsonNull:
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
jsonsymbol(const char *buf, int len)
{
	if(len == 4){
		if(memcmp(buf, "true", 4) == 0)
			return JsonBoolean;
		if(memcmp(buf, "null", 4) == 0)
			return JsonNull;
	} else if(len == 5){
		if(memcmp(buf, "false", 5) == 0)
			return JsonBoolean;
	}
	return -1;
}

// notice that all the switch-case business and string scanning looks for ascii values < 128,
// hence there is no need to decode utf8 explicitly, it just works.
static int
jsonlex(const char *buf, int *offp, int *lenp, const char **tokp)
{
	const char *str;
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
			return jsonsymbol(*tokp, str-*tokp+1);
		}
	}
	// set them here too, so that we don't re-read the last character indefinitely
	*offp = str+1-buf;
	*lenp = len-1;
	return -1;
}

static int jsonany(JsonAst *ast, int *astoff, int jscap, const char *buf, int *offp, int *lenp);

static int
jsonarray(JsonAst *ast, int *astoff, int jscap, const char *buf, int *offp, int *lenp)
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
jsonobject(JsonAst *ast, int *astoff, int jscap, const char *buf, int *offp, int *lenp)
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
jsonany(JsonAst *ast, int *astoff, int jscap, const char *buf, int *offp, int *lenp)
{
	const char *tok;
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
	case JsonBoolean:
	case JsonNull:
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

int
jsonparse(JsonRoot *root, const char *buf, int len)
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
