CC=gcc
LDFLAGS=-lm
SRC=board.c piece.c square.c move.c zobrist.c tt.c mt19937-64.c evaluation.c pieces_tables.c precomputed_move_data.c
HEAD=board.h piece.h square.h move.h zobrist.h tt.h mt19937-64.h evaluation.h pieces_tables.h precomputed_move_data.h

OBJ_DIR=bin/obj
OBJ=$(patsubst %.c, $(OBJ_DIR)/%.o, $(SRC))

UCI=./bin/uci
UCI_TESTS=$(UCI)_tests
UCI_DEBUG=$(UCI)_debug
UCI_RELEASE=$(UCI)_o3_release

.PHONY: all run clean

all: debug tests

$(OBJ_DIR):
	mkdir -p $(OBJ_DIR)

$(OBJ_DIR)/%.o: %.c $(HEAD) | $(OBJ_DIR)
	$(CC) -c $< -o $@

tests: tests.c $(OBJ)
	$(CC) tests.c $(OBJ) -o $(UCI_TESTS)

debug: main.c $(OBJ)
	$(CC) $(LDFLAGS) -DUCI_DEBUG main.c $(OBJ) -o $(UCI_DEBUG)

release: main.c $(OBJ)
	$(CC) $(LDFLAGS) -O3 main.c $(OBJ) -o $(UCI_RELEASE)

install: release
	cp $(UCI_RELEASE) ~/dev/en_croissant_engines/main

install_debug: debug
	cp $(UCI_DEBUG) ~/dev/en_croissant_engines/uci_debug

clean:
	-rm -rf $(OBJ_DIR)
	-rm -f $(UCI_DEBUG)
	-rm -f $(UCI_RELEASE)
	-rm -f $(UCI_TESTS)
	-rm -f $(OBJ)