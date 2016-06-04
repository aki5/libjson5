typedef struct JsonAst JsonAst;
typedef struct JsonRoot JsonRoot;

struct JsonRoot {
	struct {
		JsonAst *buf;
		int len;
		int cap;
	} ast;
	struct {
		char *buf;
		int len;
		int cap;
	} str;
};

struct JsonAst {
	int type;	// type of json element (JsonObject, JsonArray, JsonNumber, JsonString, JsonSymbol)
	int next;	// offset of next json element in JsonAst array
	int off;	// offset of token
	int len;	// length of token
};

// decided to make all of these in-band, because they can be.
enum {
	JsonArray = '[',
	JsonObject = '{',
	JsonString = '"',
	JsonNumber = '0',
	JsonSymbol = 'a',
};

//int jsonparse(JsonAst *ast, int *astoff, int jscap, char *buf, int *offp, int *lenp);
void jsonsetname(char *filename);

int jsonwalk(JsonRoot *root, int off, char *name);
int jsonparse(JsonRoot *root, char *buf, int len);

char *jsoncstr(JsonRoot *root, int off);
