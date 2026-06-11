#pragma once

#include "board.h"

typedef struct evaluation_data {
    int material_score;
    int piece_square_score;
    int pawn_score;
    int rook_score;
    int pawn_shield_score;
    int mop_up_score;
} evaluation_data_t;

typedef struct material_info {
    int material_score;
    int num_pawns;
    int num_majors;
    int num_minors;
    int num_knights;
    int num_bishops;
    int num_queens;
    int num_rooks;

    float endgame_t;
} material_info_t;

int evaluate(board_t* board);
