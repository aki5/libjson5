
CFLAGS=-I. -O2 -W -Wall

tests/json_test: tests/json_test.o json.o
	$(CC) -o $@ tests/json_test.o json.o

test: tests/json_test
	tests/json_test -s tests/test_schema.json tests/test_file.json

clean:
	rm -f tests/json_test tests/*.o *.o
