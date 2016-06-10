#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <fcntl.h>
#include <errno.h>
#include <string.h>
#include <sys/stat.h>
#include "json.h"

size_t
filesize(char *path)
{
	struct stat stdoc;
	if(stat(path, &stdoc) == -1){
		fprintf(stderr, "stat '%s': %s\n", path, strerror(errno));
		return -1;
	}
	return stdoc.st_size;
}

size_t
writefile(char *path, void *data, size_t len)
{
	size_t nwr;
	int fd;
	if((fd = open(path, O_WRONLY)) == -1){
		fprintf(stderr, "open '%s': %s\n", path, strerror(errno));
		return -1;
	}
	nwr = write(fd, data, len);
	if(close(fd) == -1){
		fprintf(stderr, "close '%s': %s\n", path, strerror(errno));
		return -1;
	}
	return nwr;
}

size_t
readfile(char *path, void *data, size_t len)
{
	size_t nrd;
	int fd;
	if((fd = open(path, O_RDONLY)) == -1){
		fprintf(stderr, "open '%s': %s\n", path, strerror(errno));
		return -1;
	}
	nrd = read(fd, data, len);
	if(close(fd) == -1){
		fprintf(stderr, "close '%s': %s\n", path, strerror(errno));
		return -1;
	}
	return nrd;
}

JsonRoot scmroot;
char *scm;
int scmlen;
int scmcap;

JsonRoot docroot;
char *doc;
int doclen;
int doccap;

int
main(int argc, char *argv[])
{
	char *path;
	int opt;
	while((opt = getopt(argc, argv, "s:")) != -1){
		switch(opt){
		case 's':
			scmcap = filesize(optarg);
			if(scmcap == -1)
				exit(1);
			scm = malloc(scmcap);
			scmlen = readfile(optarg, scm, scmcap);
			if(scmlen < scmcap){
				fprintf(stderr, "error reading %s\n", optarg);
				exit(1);
			}
			if(jsonparse(&scmroot, scm, scmlen) == -1){
				fprintf(stderr, "error parsing %s\n", optarg);
				exit(1);
			}
			break;
		default:
		caseusage:
			fprintf(stderr, "usage: %s [-s path/to/schema.json] file.json\n", argv[0]);
			exit(1);
		}
	}
	if(optind >= argc)
		goto caseusage;

	path = argv[optind];
	doccap = filesize(path);
	doc = malloc(doccap);
	doclen = readfile(path, doc, doccap);
	if(doclen < doccap){
		fprintf(stderr, "error reading %s\n", path);
		exit(1);
	}

	if(jsonparse(&docroot, doc, doclen) == -1){
		fprintf(stderr, "error parsing %s\n", path);
		exit(1);
	}

	if(scm != NULL){
		if(jsonrefs(&scmroot) == -1){
			fprintf(stderr, "jsonrefs: fail for schema\n");
			exit(1);
		}
		if(jsoncheck(&docroot, 0, &scmroot, 0) == -1){
			fprintf(stderr, "type checking failed for '%s'\n", path);
			exit(1);
		}
	}

	return 0;
}

