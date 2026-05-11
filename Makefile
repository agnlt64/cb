CC=gcc
DEPS=board.c piece.c square.c move.c

.PHONY: all clean

all: run chess tests

run: chess
	./bin/main

tests: tests.c $(DEPS)
	$(CC) tests.c $(DEPS) -o ./bin/tests
	./bin/tests

chess: main.c $(DEPS)
	$(CC) main.c  $(DEPS) -o bin/main

clean:
	rm bin/*