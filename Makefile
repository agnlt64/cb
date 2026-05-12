CC=gcc
DEPS=board.c piece.c square.c move.c zobrist.c
MT_OBJ=bin/mt19937-64.o

.PHONY: all clean

all: run chess tests

$(MT_OBJ): mt19937-64.c mt19937-64.h
	$(CC) -c mt19937-64.c -o $(MT_OBJ)

run: chess
	./bin/main

tests: tests.c $(DEPS) $(MT_OBJ)
	$(CC) tests.c $(DEPS) $(MT_OBJ) -o ./bin/tests
	./bin/tests

chess: main.c $(DEPS) $(MT_OBJ)
	$(CC) -DNO_UCI main.c $(DEPS) $(MT_OBJ) -o bin/main

install:
	$(CC) main.c $(DEPS) $(MT_OBJ) -o bin/main
	cp bin/main ~/dev/en_croissant_engines

clean:
	rm bin/*