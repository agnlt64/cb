#include <math.h>
#include <stdlib.h>
#include <assert.h>

#include "evaluation.h"
#include "piece.h"
#include "pieces_tables.h"
#include "precomputed_move_data.h"

// https://github.com/SebLague/Chess-Coding-Adventure/blob/Chess-V2-UCI/Chess-Coding-Adventure/src/Core/Evaluation/Evaluation.cs

int evaluation_data_sum(evaluation_data_t* data)
{
    return data->material_score + data->mop_up_score + data->pawn_score + data->pawn_shield_score + data->piece_square_score;
}

int count_material(board_t* board, color_t color)
{
    int total = 0;

    for (int i = 0; i < FILES * RANKS; i++)
    {
        piece_t p = board->squares[i];
        if (piece_color(p) != color) continue;

        total += piece_value(p);
    }

    return total;
}

void new_material_info(material_info_t* info, int num_pawns, int num_knights, int num_bishops, int num_rooks, int num_queens)
{
    static const int queen_endgame_weight = 45;
    static const int rook_endgame_weight = 20;
    static const int bishop_endgame_weight = 10;
    static const int knight_endgame_weight = 10;

    info->num_majors = num_rooks + num_queens;
    info->num_minors = num_bishops + num_knights;
    info->num_pawns = num_pawns;
    info->num_knights = num_knights;
    info->num_bishops = num_bishops;
    info->num_rooks = num_rooks;
    info->num_queens = num_queens;

    int material_score = 0;
    material_score += num_pawns * piece_value(PAWN);
    material_score += num_knights * piece_value(KNIGHT);
    material_score += num_bishops * piece_value(BISHOP);
    material_score += num_rooks * piece_value(ROOK);
    material_score += num_queens * piece_value(QUEEN);

    info->material_score = material_score;

    int endgame_start_weight = 2 * rook_endgame_weight + 2 * bishop_endgame_weight + 2 * knight_endgame_weight + queen_endgame_weight;
    int endgame_weight_sum = num_queens * queen_endgame_weight + num_rooks * rook_endgame_weight + num_bishops * bishop_endgame_weight + num_knights * knight_endgame_weight;
    info->endgame_t = 1 - fminf(1, (float)endgame_weight_sum / (float)(endgame_start_weight));
}

int mop_up(board_t* board, color_t turn, material_info_t my_material, material_info_t enemy_material)
{
    if (my_material.material_score > enemy_material.material_score + piece_value(PAWN) * 2 && enemy_material.endgame_t > 0)
    {
        int score = 0;
        int friendly_king = FIND_KING(board, turn);
        int opp_king = FIND_KING(board, turn == WHITE ? BLACK : WHITE);
        assert(friendly_king >= 0 && opp_king >= 0 && "unreachable i think");
        score += (14 - orthogonal_distance[friendly_king][opp_king]) * 4;
        score += centre_manhattan_distance[opp_king] * 10;
        return (int)(score * enemy_material.endgame_t);
    }
    return 0;
}

int eval_piece_tables(board_t* board, color_t turn, float endgame_t)
{
    int value = 0;

    for (int sq = 0; sq < 64; sq++)
    {
        piece_t p = board->squares[sq];
        if (piece_type(p) == NO_PIECE || piece_color(p) != turn) continue;
        // handle every piece except king and pawns
        value += piece_square_value(p, sq);

        int idx = turn == WHITE ? sq : flip_square_idx(sq);
        
        if (piece_type(p) == PAWN)
        {
            int pawn_early = pawn_early_table[idx];
            int pawn_late = pawn_late_table[idx];
            value += (int)(pawn_early * (1 - endgame_t));
            value += (int)(pawn_late * endgame_t);
        }
        else if (piece_type(p) == KING)
        {
            int king_early = king_start_table[idx];
            int king_late = king_end_table[idx];
            value += (int)(king_early * (1 - endgame_t));
            value += (int)(king_late * endgame_t);
        }
    }

    return value;
}

void get_material_info(material_info_t* info, board_t* board, color_t color)
{
    int num_pawns = 0;
    int num_knights = 0;
    int num_bishops = 0;
    int num_rooks = 0;
    int num_queens = 0;

    for (int sq = 0; sq < 64; sq++)
    {
        piece_t p = board->squares[sq];
        if (piece_color(p) != color) continue;

        switch (piece_type(p))
        {
            case PAWN: num_pawns++; break;
            case KNIGHT: num_knights++; break;
            case BISHOP: num_bishops++; break;
            case ROOK: num_rooks++; break;
            case QUEEN: num_queens++; break;
            default:
                break;
        }
    }

    new_material_info(info, num_pawns, num_knights, num_bishops, num_rooks, num_queens);
}

int evaluate(board_t* board)
{

    evaluation_data_t white_eval = {0};
    evaluation_data_t black_eval = {0};

    material_info_t white_material = {0};
    material_info_t black_material = {0};

    get_material_info(&white_material, board, WHITE);
    get_material_info(&black_material, board, BLACK);

    white_eval.material_score = white_material.material_score;
    black_eval.material_score = black_material.material_score;

    white_eval.piece_square_score = eval_piece_tables(board, WHITE, black_material.endgame_t);
    black_eval.piece_square_score = eval_piece_tables(board, BLACK, white_material.endgame_t);

    white_eval.mop_up_score = mop_up(board, WHITE, white_material, black_material);
    black_eval.mop_up_score = mop_up(board, BLACK, black_material, white_material);

    int sign = board->turn == WHITE ? 1 : -1;
    int eval = evaluation_data_sum(&white_eval) - evaluation_data_sum(&black_eval);
    return sign * eval;
}
