#pragma once

#include "board.h"

int gen_capture_moves(board_t* board, move_t* moves);
int gen_legal_moves(board_t* board, move_t* moves, move_t to_explore);
int gen_pseudo_legal_moves(board_t* board, move_t* moves);
int gen_king_moves(board_t* board, int sq, move_t* moves);
int gen_knight_moves(board_t* board, int sq, move_t* moves);
int gen_diag_moves(board_t* board, int sq, move_t* moves);
int gen_orth_moves(board_t* board, int sq, move_t* moves);
int gen_pawn_moves(board_t* board, int sq, move_t* moves);