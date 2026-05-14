CC=gcc
LDFLAGS=-lm
DEPS_C=board.c piece.c square.c move.c zobrist.c tt.c
DEPS_H=board.h piece.h square.h move.h zobrist.h tt.h
DEPS=$(DEPS_C)
MT_OBJ=bin/mt19937-64.o
UCI=./bin/uci
UCI_DEBUG=$(UCI)_debug
UCI_RELEASE=$(UCI)_o3_release

.PHONY: all run clean

all: uci uci_debug tests

$(MT_OBJ): mt19937-64.c mt19937-64.h
	$(CC) -c mt19937-64.c -o $(MT_OBJ)

tests: tests.c $(DEPS) $(DEPS_H) $(MT_OBJ)
	$(CC) tests.c $(DEPS) $(MT_OBJ) -o ./bin/tests
	./bin/tests

run: uci
	$(UCI)

uci: main.c $(DEPS) $(DEPS_H) $(MT_OBJ)
	$(CC) $(LDFLAGS) main.c $(DEPS) $(MT_OBJ) -o $(UCI)

uci_debug: main.c $(DEPS) $(DEPS_H) $(MT_OBJ)
	$(CC) $(LDFLAGS) -DUCI_DEBUG main.c $(DEPS) $(MT_OBJ) -o $(UCI_DEBUG)

release: main.c $(DEPS) $(DEPS_H) $(MT_OBJ)
	$(CC) $(LDFLAGS) -O3 main.c $(DEPS) $(MT_OBJ) -o $(UCI_RELEASE)

install: release
	cp $(UCI_RELEASE) ~/dev/en_croissant_engines/main

clean:
	rm bin/*