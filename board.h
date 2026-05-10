#pragma once

#include <stdbool.h>

#include "piece.h"
#include "move.h"
#include "square.h"

#define DEFAULT_FEN "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1"

#define RANKS 8
#define FILES 8

typedef struct board_history {
    int castling;
    int ep_square_idx;
    int halfmove;
} board_history_t;

typedef struct board {
    int squares[FILES * RANKS];
    color_t turn;
    int castling; // 4 bits, KQkq
    int ep_square_idx;
    int halfmove;
    int fullmove;
    board_history_t history[512];
    int history_top;
} board_t;

void board_init(board_t* board);
void board_print(board_t* board);
piece_t board_at(board_t* board, square_t sq);
void board_from_fen(board_t* board, const char* fen);

int board_gen_moves(board_t* board, move_t* moves);
int gen_king_moves(board_t* board, int sq, move_t* moves);
int gen_knight_moves(board_t* board, int sq, move_t* moves);
int gen_diag_moves(board_t* board, int sq, move_t* moves);
int gen_orth_moves(board_t* board, int sq, move_t* moves);
int gen_pawn_moves(board_t* board, int sq, move_t* moves);

void board_make_move(board_t* board, move_t move);
void board_unmake_move(board_t* board, move_t* move);

bool is_square_attacked(board_t* board, square_t sq, color_t color);
bool can_knight_attack(board_t* board, square_t sq, color_t color);
bool can_king_attack(board_t* board, square_t sq, color_t color);
bool can_pawn_attack(board_t* board, square_t sq, color_t color);
bool can_diag_attack(board_t* board, square_t sq, color_t color);
bool can_orth_attack(board_t* board, square_t sq, color_t color);
