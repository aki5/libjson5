
#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>

char *key = "child";
char alpha[] = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ";

enum {
	Number = 0,
	Integer,
	String,
	Null,
	Bool,
	Array,
	Object,
	NumTypes,
};

char *
value(void)
{
	int i, len;
	static char buf[256];
	len = (lrand48()%16) * (lrand48()%16);
	if(len > sizeof buf-1)
		len = sizeof buf-1;
	if(len > 0)
		len = lrand48()%len;
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
valuegen(int rnd)
{
	static int depth;
	int i, fix;
	depth++;
	if(rnd == -1)
		rnd = lrand48()%NumTypes;
	if(depth > 30)
		rnd %= Array;
	switch(rnd){
	default:
		abort();
	case Number: // number
		printf("%g", drand48());
		break;
	case Integer: // integer
		fix = -(lrand48()%1);
		printf("%ld", fix*lrand48());
		break;
	case String: // string
		printf("\"%s\"", value());
		break;
	case Null: // symbol
		printf("null");
		break;
	case Bool: // symbol
		fix = lrand48()%2;
		if(fix == 1)
			printf("true");
		else
			printf("false");
		break;
	case Array: // array
		rnd = lrand48()%10;
		fix = lrand48()%NumTypes;
		printf("[");
		for(i = 0; i < rnd; i++){
			if(i != 0)
				printf(",");
			valuegen(fix);
		}
		printf("]");
		break;
	case Object: // object
		rnd = lrand48()%10;
		printf("{");
		for(i = 0; i < rnd; i++){
			if(i != 0)
				printf(",");
			printf("\"%s\":", value());
			valuegen(-1);
		}
		printf("}");
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
	for(i = 0; i < numobjs; i++){
		valuegen(Object);
		printf("\n");
	}
	return 0;
}
