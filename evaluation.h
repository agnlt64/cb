#pragma once

#include "board.h"

typedef struct evaluation_data {
    int material_score;
    int piece_square_score;
    int pawn_score;
    int pawn_shield_score; // TODO
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

int evaluation_data_sum(evaluation_data_t* data);

void new_material_info(material_info_t* info, int num_pawns, int num_knights, int num_bishops, int num_queens, int num_rooks);
void get_material_info(material_info_t* info, board_t* board, color_t color);

int eval_piece_tables(board_t* board, color_t turn, float endgame_t);