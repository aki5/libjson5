
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

## JSON References

A JSON reference is a JSON object that contains a "$ref" property.
when the reference behaves as if the reference object was replaced
with JSON data in the referred location. For example

```
{ "$ref": "#/foo/bar" }
```

Only references within the same document can be interpreted by standalone libjson5, due to lack of interest in choosing a specific http access library or an i/o framework.

## JSON Schema

A JSON schema is a JSON object describing types and properties of
another JSON object. The type checker in libjson5 requires all properties and items to be declared in the schema, and checks for their types. 

In addition to basic type checking, the type checker checks for presence of "required" fields if specified in the schema. Other features of the json-schema are not implemented at the moment.

An example containing just about everything supported in libjson5 is below

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