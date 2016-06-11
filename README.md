
[![Build Status](https://travis-ci.org/aki5/libjson5.svg?branch=master)](https://travis-ci.org/aki5/libjson5)

# Libjson5 - a tiny JSON parser with type checking

This is the fifth JSON parser library I've written in C. The previous versions have been lost as closed source, so I'm making this one open. It is intended to be part of a bridge between some of my hobby obsessions and web browsers.

A distinguishing feature of libjson5 is that it supports type checking against a partially supported JSON schema.

Using a schema can give higher confidence that data stored in a key-value store satisfies some basic structural requirements.

The recognized language is standard JSON, with support for the following extras

* JSON pointers,
* JSON references, and
* type checking a JSON document against a given JSON schema

## Data structures and API

Libjson5 is a little unusual in the sense that it does not try to convert numbers into doubles, nor does it define a tree of structs or implement accelerated data structures for property or array element access. Very high parsing performance can be achieved this way.

Instead of returning a pointer based tree of structs, libjson5 constructs a flat array-mapped syntax tree containing offset-length pairs, which refer to the input string. A few convenient accessors are provided for common cases, to abstract away from the underlying data structure.

### JSON parsing API

The `jsonparse` function expects to get JSON data in a char buffer, and a length argument telling how many bytes are valid. `Jsonparse` will not create copies of the input buffer. The AST will refer to the input buffer by offsets.

`Jsonsetname` can be used to set a file name to be used in diagnostic printf statements inside `jsonparse`.

When the AST is no longer needed, it can be freed using `jsonfree`. `Jsonfree` frees any arrays allocated for the AST, but it does not attempt to free the input buffer. If the input buffer was acquired via malloc, the user needs to free it separately.

```
int jsonparse(JsonRoot *root, char *buf, int len);
void jsonsetname(char *filename);
void jsonfree(JsonRoot *root);
```

### JSON access API

`Jsonwalk` returns the AST offset corresponding to the JSON value for the given `key` in the JSON (sub-)object at AST offset `off`. As usual, `off` of `0` refers to the top-level JSON object.

`Jsonwalk2` is just like `jsonwalk`, but instead of taking a `'\0'`-terminated key, it takes a length value instead. This is most useful when dealing with read-only buffers that lack `'\0'`-terminators, like memory mapped JSON-files etc.

`Jsonindex` does for JSON-arrays what `jsonwalk` does for objects: it starts at a JSON array at AST position `off` and returns an AST offset to the `index`-th element of that array.

`Jsoncstr` returns a malloc'd `'\0'`-terminated string for a JSON-string terminal in the AST position `off`.

```
int jsonwalk(JsonRoot *root, int off, char *name);
int jsonwalk2(JsonRoot *root, int off, char *name, int namelen);
int jsonindex(JsonRoot *root, int off, int index);
char *jsoncstr(JsonRoot *root, int off);
```

### JSON checking API

A JSON reference is a JSON object that contains a "$ref" property.
when the reference behaves as if the reference object was replaced
with JSON data in the referred location. For example

```
{ "$ref": "#/definitions/intarray" }
```

Only references within the same document are understood by standalone libjson5, but JSON schemas often contain JSON references to the same document, which need to be evaluated by calling `jsonrefs` before type checking can be performed.

`Jsoncheck` takes a document and a corresponding schema as parameters. In addition, it will take two offset parameters to the AST, indicating where in the AST should the type checking begin, separately for the document and the schema. `0` refers to the root of an AST, so the full document can be checked against the full schema by passing both offsets as `0`.

```
int jsonrefs(JsonRoot *doc);
int jsoncheck(JsonRoot *doc, int docoff, JsonRoot *scmroot, int scmoff);
```

## About JSON Schema

Once everything is parsed and the JSON references are evaluated, the document can be type checked against a schema.

```
if(jsoncheck(&doc, 0, &schema, 0) == -1){
	fprintf(stderr, "type checking failed for doc\n");
	exit(1);
}
```

A JSON schema is a JSON object describing types and properties of
another JSON object. The type checker in libjson5 requires all properties and items to be declared in the schema, and checks for their types. 

In addition to basic type checking, the type checker checks for presence of "required" fields if specified in the schema. Other features of the json-schema are not implemented at the moment.

An example schema containing just about everything supported by libjson5 is below

```
{
	"$schema": "http://json-schema.org/draft-04/schema#",
	"type": "object",
	"properties": {
		"id": { "type": "integer" },
		"name": { "type": "string" },
		"price": { "type": "number" },
		"location": { "$ref": "#/definitions/location" },
		"counts": { "$ref": "#/definitions/intarray" }
	},
	"required": [ "id", "name", "price" ],
	"definitions": {
		"location": {
			"type": "object",
			"properties": {
				"latitude": { "type": "number" },
				"longitude": { "type": "number" }
			}
		},
		"intarray": {
			"type": "array",
			"items": { "type": "integer" }
		}
	}
}
```