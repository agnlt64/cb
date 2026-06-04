CC=gcc
LDFLAGS=-lm
SRC=board.c piece.c square.c move.c zobrist.c tt.c  \
	mt19937-64.c evaluation.c pieces_tables.c       \
	precomputed_move_data.c precomputed_eval_data.c \
	debug.c uci.c killer.c search.c
HEAD=board.h piece.h square.h move.h zobrist.h tt.h  \
	 mt19937-64.h evaluation.h pieces_tables.h       \
	 precomputed_move_data.h precomputed_eval_data.h \
	 debug.h uci.h killer.h search.h

OBJ_DIR=bin/obj
OBJ_DEBUG=$(patsubst %.c, $(OBJ_DIR)/debug/%.o, $(SRC))
OBJ_RELEASE=$(patsubst %.c, $(OBJ_DIR)/release/%.o, $(SRC))

UCI=./bin/uci
UCI_TESTS=$(UCI)_tests
UCI_DEBUG=$(UCI)_debug
UCI_RELEASE=$(UCI)_o3_release

CFLAGS_DEBUG=-DUCI_DEBUG -ggdb
CFLAGS_RELEASE=-O3

.PHONY: all run clean debug release tests install install_debug

all: debug tests

$(OBJ_DIR)/debug $(OBJ_DIR)/release:
	mkdir -p $@

$(OBJ_DIR)/debug/%.o: %.c $(HEAD) | $(OBJ_DIR)/debug
	$(CC) $(CFLAGS_DEBUG) -c $< -o $@

$(OBJ_DIR)/release/%.o: %.c $(HEAD) | $(OBJ_DIR)/release
	$(CC) $(CFLAGS_RELEASE) -c $< -o $@

tests: tests.c $(OBJ_DEBUG)
	$(CC) $(CFLAGS_DEBUG) tests.c $(OBJ_DEBUG) -o $(UCI_TESTS)

debug: main.c $(OBJ_DEBUG)
	$(CC) $(LDFLAGS) $(CFLAGS_DEBUG) main.c $(OBJ_DEBUG) -o $(UCI_DEBUG)

release: main.c $(OBJ_RELEASE)
	$(CC) $(LDFLAGS) $(CFLAGS_RELEASE) main.c $(OBJ_RELEASE) -o $(UCI_RELEASE)

install: release
	cp $(UCI_RELEASE) ~/dev/en_croissant_engines/main

install_debug: debug
	cp $(UCI_DEBUG) ~/dev/en_croissant_engines/uci_debug

clean:
	-rm -rf $(OBJ_DIR)
	-rm -f $(UCI_DEBUG)
	-rm -f $(UCI_RELEASE)
	-rm -f $(UCI_TESTS)