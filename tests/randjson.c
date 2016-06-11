
#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>

FILE *jsonfp;
FILE *schfp;
char *key = "child";
char alpha[] = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ";

enum {
	Number = 0,
	Integer,
	String,
	Null,
	Bool,
//	Array,
	Object,
	NumTypes,
};

char *
value(void)
{
	size_t i, len;
	static char buf[256];
	len = (lrand48()%16) * (lrand48()%16);
	if(len > sizeof buf-1)
		len = sizeof buf-1;
	if(len > 0)
		len = lrand48()%len;
	if(len < 5)
		len = 5;
	for(i = 0; i < len; i++)
		buf[i] = alpha[lrand48() % (sizeof alpha-1)];
	buf[i] = '\0';
	return buf;
}

char *
symbolgen(void)
{
	char *syms[] = {"true", "false", "null"};
	int rnd;
	rnd = lrand48()%3;
	return syms[rnd];
}

void
valuegen(int typ)
{
	static int depth;
	int i, rnd, fix;
	depth++;
	if(typ == -1)
		rnd = lrand48()%NumTypes;
	else
		rnd = typ;
	if(depth > 30)
		rnd %= Bool+1;
	switch(rnd){
	default:
		abort();
	case Number: // number
		fprintf(jsonfp, "%g", drand48());
		if(typ == -1)fprintf(schfp, "{\"type\": \"number\"}");
		break;
	case Integer: // integer
		fix = -(lrand48()%1);
		fprintf(jsonfp, "%ld", fix*lrand48());
		fprintf(schfp, "{\"type\": \"integer\"}");
		break;
	case String: // string
		fprintf(jsonfp, "\"%s\"", value());
		fprintf(schfp, "{\"type\": \"string\"}");
		break;
	case Null: // symbol
		fprintf(jsonfp, "null");
		fprintf(schfp, "{\"type\": \"object\"}");
		break;
	case Bool: // symbol
		fix = lrand48()%2;
		if(fix == 1)
			fprintf(jsonfp, "true");
		else
			fprintf(jsonfp, "false");
		fprintf(schfp, "{\"type\": \"boolean\"}");
		break;
/*
	case Array: // array
		rnd = lrand48()%10;
		fix = lrand48()%NumTypes;
		fprintf(schfp, "{");
		fprintf(schfp, "\"type\": \"array\",");
		fprintf(schfp, "\"items\":{");
		fprintf(jsonfp, "[");
		for(i = 0; i < rnd; i++){
			if(i != 0)
				fprintf(jsonfp, ",");
			valuegen(fix);
		}
		fprintf(jsonfp, "]");
		fprintf(schfp, "}");
		fprintf(schfp, "}");
		break;
*/
	case Object: // object
		rnd = lrand48()%10;
		fprintf(schfp, "{");
		fprintf(schfp, "\"type\": \"object\",");
		fprintf(schfp, "\"properties\":{");
		fprintf(jsonfp, "{");
		for(i = 0; i < rnd; i++){
			if(i != 0){
				fprintf(jsonfp, ",");
				fprintf(schfp, ",");
			}
			char *key = value();
			fprintf(jsonfp, "\"%s\":", key);
			fprintf(schfp, "\"%s\":", key);
			valuegen(-1);
		}
		fprintf(jsonfp, "}");
		fprintf(schfp, "}");
		fprintf(schfp, "}");
		break;
	}
	depth--;
}

int
main(int argc, char *argv[])
{
	struct timeval tv;
	int i, numobjs;

	gettimeofday(&tv, NULL);
	srand48(tv.tv_sec ^ tv.tv_usec);

	numobjs = 1;
	if(argc > 1)
		numobjs = strtol(argv[1], NULL, 10);
	jsonfp = stdout;
	schfp = stderr;
	for(i = 0; i < numobjs; i++){
		valuegen(Object);
		fprintf(jsonfp, "\n");
		fprintf(schfp, "\n");
	}
	return 0;
}
