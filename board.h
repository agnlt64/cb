#pragma once

#include <stdbool.h>

#include "piece.h"
#include "move.h"
#include "square.h"
#include "zobrist.h"
#include "killer.h"

#define DEFAULT_FEN "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1"

#define RANKS 8
#define FILES 8

#define MAX_DEPTH 20

#define COLOR_IDX(color) ((color) == WHITE ? 0 : 1)

// assume that king_sq is never empty
// it should never be, because king_sq is
// populated with from_fen
#define FIND_KING(board, turn) (board)->king_sq[COLOR_IDX(turn)]

typedef struct board_history {
    int castling;
    int ep_square_idx;
    int halfmove;
    uint64_t hash;
} board_history_t;

typedef struct board {
    int squares[FILES * RANKS];
    int king_sq[2];
    color_t turn;
    int castling; // 4 bits, KQkq
    int ep_square_idx;
    int halfmove;
    int fullmove;
    board_history_t history[2048];
    int history_top;
    zobrist_t zobrist;
    uint64_t hash;
} board_t;

void board_init(board_t* board);
void board_print(board_t* board);
piece_t board_at(board_t* board, square_t sq);
void board_from_fen(board_t* board, const char* fen);
bool board_in_check(board_t* board);
void board_flip_turn(board_t* board);
int board_perft(board_t* board, int depth, bool verbose);

void board_make_move(board_t* board, move_t move);
void board_unmake_move(board_t* board, move_t move);

uint64_t zobrist_from_board(board_t* board);

void order_moves(board_t* board, move_t* moves, int moves_size, killer_t* killers, int depth, bool q_search, move_t pv_move, int history[64][64]);
int find_lva(square_t* squares, int to, color_t side);