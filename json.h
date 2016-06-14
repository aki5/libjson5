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
	JsonInteger = '1',
	JsonBoolean = 't',
	JsonNull = 'n',
	JsonReference = '#', // a resolved "$ref" object, 'off' field for this AST type holds the destination
};

int jsoncheck(JsonRoot *docroot, int docoff, JsonRoot *scmroot, int scmoff);
char *jsoncstr(JsonRoot *root, int off);
void jsonfree(JsonRoot *root);
int jsonindex(JsonRoot *root, int off, int index);
int jsonparse(JsonRoot *root, char *buf, int len);
int jsonrefs(JsonRoot *docroot);
void jsonsetname(char *filename);
int jsonwalk(JsonRoot *root, int off, char *name);
int jsonwalk2(JsonRoot *root, int off, char *name, int namelen);
int jsonptr(JsonRoot *root, int off, char *ptr, int ptrlen);
