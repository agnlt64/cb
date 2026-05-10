CC=gcc

.PHONY: all clean

all: chess, tests

tests: tests.c board.c piece.c square.c move.c
	$(CC) tests.c board.c piece.c square.c move.c -o ./bin/tests
	./bin/tests

chess:
	$(CC) main.c -o bin/main
	./bin/main

clean:
	rm bin/*