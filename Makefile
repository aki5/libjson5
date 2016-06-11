
CFLAGS=-I. -O2 -W -Wall

tests/json_test: tests/json_test.o json.o
	$(CC) -o $@ tests/json_test.o json.o

tests/randjson: tests/randjson.o
	$(CC) -o $@ tests/randjson.o

test: tests/json_test tests/randjson
	tests/json_test -s tests/test_schema2.json tests/test_file.json
	tests/json_test -s tests/test_schema.json tests/test_file.json
	for i in `seq 1 100`; do tests/randjson 2> tests/rand-schema.json > tests/rand.json && tests/json_test -s tests/rand-schema.json tests/rand.json; done
	rm tests/rand.json tests/rand-schema.json

clean:
	rm -f tests/json_test tests/randjson tests/rand.json tests/rand-schema.json tests/*.o *.o
