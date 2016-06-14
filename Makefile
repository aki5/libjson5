
CFLAGS=-I. -Os -fomit-frame-pointer -W -Wall

LIBJSON_OFILES=\
	json.o\
	jsoncheck.o\
	jsoncstr.o\
	jsonindex.o\
	jsonptr.o\
	jsonrefs.o\
	jsonwalk.o\

libjson.a: $(LIBJSON_OFILES)
	$(AR) r $@ $(LIBJSON_OFILES)

tests/json_test: tests/json_test.o libjson.a
	$(CC) -o $@ tests/json_test.o libjson.a

tests/randjson: tests/randjson.o
	$(CC) -o $@ tests/randjson.o

test: tests/json_test tests/randjson
	tests/json_test -s tests/test_schema2.json tests/test_file.json
	tests/json_test -s tests/test_schema.json tests/test_file.json
	for i in `seq 1 1000`; do tests/randjson 2> tests/rand-schema.json > tests/rand.json && tests/json_test -s tests/rand-schema.json tests/rand.json; done
	rm tests/rand.json tests/rand-schema.json

clean:
	rm -f libjson.a tests/json_test tests/randjson tests/rand.json tests/rand-schema.json tests/*.o *.o
