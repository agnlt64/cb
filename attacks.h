#pragma once

#include "board.h"

bool is_square_attacked(board_t* board, square_t sq, color_t color);
bool can_knight_attack(board_t* board, square_t sq, color_t color);
bool can_king_attack(board_t* board, square_t sq, color_t color);
bool can_pawn_attack(board_t* board, square_t sq, color_t color);
bool can_diag_attack(board_t* board, square_t sq, color_t color);
bool can_orth_attack(board_t* board, square_t sq, color_t color);