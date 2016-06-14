#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include "json.h"

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
